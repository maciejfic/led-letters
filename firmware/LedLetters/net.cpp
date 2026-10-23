#include "net.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "config.h"
#include "settings.h"
#include "effects.h"
#include "layout.h"
#include "web_ui.h"

// NET_IDLE: no hotspot, no connection (Wi-Fi not set up, or saved network unreachable)
enum NetState { NET_CONNECTING, NET_CONNECTED, NET_AP, NET_IDLE };

static const uint32_t AP_TIMEOUT_MS = 10UL * 60 * 1000;   // hotspot turns off if nobody joins
static const uint32_t RETRY_MS = 2UL * 60 * 1000;         // retry the saved network
static const uint32_t JOIN_TIMEOUT_MS = 20000;            // trying a new network from the setup hotspot
static const uint32_t JOIN_GRACE_MS = 2UL * 60 * 1000;    // hotspot stays up this long after a successful join

// Joining a network from the setup hotspot: the hotspot stays up while the sign tries, so the
// phone can show whether it worked and the sign's new address before the hotspot goes away.
enum JoinState { JOIN_IDLE, JOIN_TRYING, JOIN_OK, JOIN_FAIL };
static JoinState joinState = JOIN_IDLE;
static uint32_t joinSince = 0;

static WebServer server(80);
static DNSServer dns;
static NetState state = NET_CONNECTING;
static uint32_t stateSince = 0, lastRetry = 0, restartAt = 0;
static bool mdnsStarted = false;
static String wifiSsid, wifiPass;

// ---------- Wi-Fi credentials ----------

static void loadWifi() {
  Preferences p;
  p.begin("wifi", true);
  wifiSsid = p.getString("ssid", "");
  wifiPass = p.getString("pass", "");
  p.end();
}

static void saveWifi(const String& ssid, const String& pass) {
  Preferences p;
  p.begin("wifi", false);
  p.putString("ssid", ssid);
  p.putString("pass", pass);
  p.end();
}

void netForgetWifi() {
  saveWifi("", "");
  restartAt = millis() + 500;
}

// ---------- mDNS (http://HOSTNAME.local) ----------

static void startMDNS() {
  if (mdnsStarted) return;
  if (MDNS.begin(HOSTNAME)) MDNS.addService("http", "tcp", 80);
  mdnsStarted = true;
}

// ---------- connection handling ----------

static void beginSTA() {
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  // Many ESP32-C3 "SuperMini" boards can't connect at full TX power
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
}

static void startAP() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  dns.start(53, "*", WiFi.softAPIP());
  state = NET_AP;
  stateSince = lastRetry = millis();
  startMDNS();
  Serial.printf("Setup hotspot \"%s\" at http://%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

static void onConnected() {
  state = NET_CONNECTED;
  startMDNS();
  Serial.printf("Connected to %s, http://%s or http://%s.local\n",
                wifiSsid.c_str(), WiFi.localIP().toString().c_str(), HOSTNAME);
}

// ---------- JSON helpers ----------

static String colorHex(const CRGB& c) {
  char b[8];
  snprintf(b, sizeof(b), "#%02x%02x%02x", c.r, c.g, c.b);
  return b;
}

static CRGB parseHex(String s) {
  s.replace("#", "");
  uint32_t v = strtoul(s.c_str(), nullptr, 16);
  return CRGB((v >> 16) & 255, (v >> 8) & 255, v & 255);
}

static String jsonStr(const String& s) {
  String o = "\"";
  for (char c : s) {
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) o += ' ';
    else o += c;
  }
  return o + "\"";
}

static String stateJson() {
  bool ap = state == NET_AP;
  String j;
  j.reserve(700);
  j += "{\"on\":";      j += S.on ? "true" : "false";
  j += ",\"mode\":";    j += S.mode;
  j += ",\"bri\":";     j += S.brightness;
  j += ",\"speed\":";   j += S.speed;
  j += ",\"c1\":";      j += jsonStr(colorHex(S.color1));
  j += ",\"c2\":";      j += jsonStr(colorHex(S.color2));
  j += ",\"auto\":";    j += S.autoCycle;
  j += ",\"maxma\":";   j += S.maxMilliamps;
  j += ",\"safeMa\":";  j += SAFE_MA;
  j += ",\"sym\":[";
  for (uint8_t L = 0; L < numSymbols; L++) { if (L) j += ","; j += jsonStr(layoutConfigured() ? glyphLabel(symbolGlyph[L]) : "Strip"); }
  j += "],\"lm\":[";
  for (uint8_t L = 0; L < numSymbols; L++) { if (L) j += ","; j += S.symbolMode[L]; }
  j += "]";
  j += ",\"mixed\":";   j += mixedMode();
  j += ",\"num\":";     j += numLeds;
  j += ",\"setup\":";   j += layoutConfigured() ? "true" : "false";
  j += ",\"maxG\":";    j += MAX_GLYPHS;
  j += ",\"warnG\":";   j += WARN_GLYPHS;
  j += ",\"minL\":";    j += MIN_GLYPH_LEDS;
  j += ",\"maxL\":";    j += MAX_GLYPH_LEDS;
  j += ",\"sign\":";    j += jsonStr(layoutSpec());
  j += ",\"ap\":";      j += ap ? "true" : "false";
  j += ",\"ssid\":";    j += jsonStr(wifiSsid);
  j += ",\"ip\":";      j += jsonStr(ap ? WiFi.softAPIP().toString() : WiFi.localIP().toString());
  j += ",\"host\":";    j += jsonStr(HOSTNAME);
  j += ",\"apSsid\":";  j += jsonStr(AP_SSID);
  j += ",\"rssi\":";    j += ap ? 0 : WiFi.RSSI();
  j += ",\"fw\":";      j += jsonStr(FW_VERSION);
  j += ",\"up\":";      j += millis() / 1000;
  j += ",\"heap\":";    j += ESP.getFreeHeap();
  j += "}";
  return j;
}

// ---------- HTTP handlers ----------

static void handleSet() {
  if (server.hasArg("on"))     S.on = server.arg("on").toInt() != 0;
  if (server.hasArg("mode"))   { S.on = true; setMode(server.arg("mode").toInt()); }
  if (server.hasArg("next"))   { S.on = true; nextMode(); }
  if (server.hasArg("bri"))    S.brightness = constrain(server.arg("bri").toInt(), 1, 255);
  if (server.hasArg("speed"))  S.speed = constrain(server.arg("speed").toInt(), 1, 255);
  if (server.hasArg("c1"))     S.color1 = parseHex(server.arg("c1"));
  if (server.hasArg("c2"))     S.color2 = parseHex(server.arg("c2"));
  if (server.hasArg("auto"))   S.autoCycle = constrain(server.arg("auto").toInt(), 0, 3600);
  if (server.hasArg("maxma"))  S.maxMilliamps = constrain(server.arg("maxma").toInt(), 100, 10000);
  for (uint8_t L = 0; L < numSymbols; L++) {
    String k = "lm" + String(L);
    if (server.hasArg(k)) {
      uint8_t m = server.arg(k).toInt();
      if (m < modeCount() && m != mixedMode()) S.symbolMode[L] = m;
      S.on = true;
      setMode(mixedMode());
    }
  }
  settingsChanged();
  server.send(200, "application/json", stateJson());
}

static void handleModes() {
  String j = "[";
  for (uint8_t m = 0; m < modeCount(); m++) {
    if (m) j += ",";
    j += "{\"n\":" + jsonStr(modeName(m)) + ",\"t\":" + (modeIsTest(m) ? "1" : "0") + "}";
  }
  server.send(200, "application/json", j + "]");
}

// LED positions for the live preview: x, y (0-255) and glyph index per LED
static void handleLayout() {
  String j;
  j.reserve(numLeds * 12 + 64);
  j = "{\"aspect\":" + String(layoutAspect(), 2) + ",\"x\":[";
  for (uint16_t i = 0; i < numLeds; i++) { if (i) j += ","; j += ledX[i]; }
  j += "],\"y\":[";
  for (uint16_t i = 0; i < numLeds; i++) { if (i) j += ","; j += ledY[i]; }
  j += "],\"s\":[";
  for (uint16_t i = 0; i < numLeds; i++) { if (i) j += ","; j += ledSymbol[i]; }
  j += "],\"p\":[";   // glyphs that beat in Heartbeat
  for (uint8_t L = 0; L < numSymbols; L++) { if (L) j += ","; j += symbolPulses[L] ? "1" : "0"; }
  j += "]}";
  server.send(200, "application/json", j);
}

static void handleGlyphs() {
  String j = "[";
  for (uint8_t g = 0; g < glyphCount(); g++) {
    if (g) j += ",";
    j += "{\"k\":" + jsonStr(glyphName(g)) + ",\"l\":" + jsonStr(glyphLabel(g)) + ",\"n\":" + String(glyphLeds(g)) + "}";
  }
  server.send(200, "application/json", j + "]");
}

// Changing the sign takes effect after a restart (the LED buffers are set up at boot)
static void handleSign() {
  String spec = server.arg("s");
  uint8_t items = spec.isEmpty() ? 0 : 1;
  for (char c : spec) if (c == ',') items++;
  uint16_t total = layoutValidate(spec);
  if (!total) {
    String err = items > MAX_GLYPHS
      ? "At most " + String(MAX_GLYPHS) + " glyphs: more would overload the current."
      : "Unknown glyph, or an LED count outside " + String(MIN_GLYPH_LEDS) + "-" + String(MAX_GLYPH_LEDS) + ".";
    server.send(400, "application/json", "{\"ok\":false,\"err\":" + jsonStr(err) + "}");
    return;
  }
  S.sign = spec;
  settingsSaveNow();
  server.send(200, "application/json", "{\"ok\":true,\"num\":" + String(total) + "}");
  restartAt = millis() + 800;
}

static void handleScan() {
  int n = WiFi.scanNetworks();
  String j = "[";
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i).isEmpty()) continue;
    if (j.length() > 1) j += ",";
    j += "{\"s\":" + jsonStr(WiFi.SSID(i)) + ",\"r\":" + String(WiFi.RSSI(i)) +
         ",\"l\":" + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "0" : "1") + "}";
  }
  WiFi.scanDelete();
  server.send(200, "application/json", j + "]");
}

static void handleWifiSave() {
  String ssid = server.arg("ssid");
  if (ssid.isEmpty()) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  saveWifi(ssid, server.arg("pass"));
  if (state != NET_AP) {
    // changing networks while connected: restart and join the new one
    server.send(200, "application/json", "{\"ok\":true,\"restart\":true}");
    restartAt = millis() + 1000;
    return;
  }
  wifiSsid = ssid;
  wifiPass = server.arg("pass");
  WiFi.disconnect(false);
  beginSTA();
  joinState = JOIN_TRYING;
  joinSince = lastRetry = millis();
  server.send(200, "application/json", "{\"ok\":true,\"restart\":false}");
}

static void handleWifiStatus() {
  static const char* names[] = {"idle", "trying", "ok", "fail"};
  bool up = WiFi.status() == WL_CONNECTED;
  String j = "{\"join\":\"" + String(names[joinState]) + "\",\"ssid\":" + jsonStr(wifiSsid) +
             ",\"ip\":" + jsonStr(up ? WiFi.localIP().toString() : "") + ",\"host\":" + jsonStr(HOSTNAME) + "}";
  server.send(200, "application/json", j);
}

static void handleNotFound() {
  // In setup mode, send every unknown URL to the setup page (captive portal)
  if (state == NET_AP) {
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
  } else {
    server.send(404, "text/plain", "Not found");
  }
}

static void setupRoutes() {
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", INDEX_HTML); });
  server.on("/api/state", HTTP_GET, []() { server.send(200, "application/json", stateJson()); });
  server.on("/api/modes", HTTP_GET, handleModes);
  server.on("/api/set", handleSet);
  server.on("/api/glyphs", HTTP_GET, handleGlyphs);
  server.on("/api/layout", HTTP_GET, handleLayout);
  server.on("/api/sign", HTTP_POST, handleSign);
  server.on("/api/wifi/scan", HTTP_GET, handleScan);
  server.on("/api/wifi", HTTP_POST, handleWifiSave);
  server.on("/api/wifi/status", HTTP_GET, handleWifiStatus);
  server.on("/api/wifi/forget", HTTP_POST, []() {
    server.send(200, "application/json", "{\"ok\":true}");
    netForgetWifi();
  });
  server.on("/api/restart", HTTP_POST, []() {
    server.send(200, "application/json", "{\"ok\":true}");
    restartAt = millis() + 500;
  });
  server.onNotFound(handleNotFound);
}

// ---------- public ----------

static void stopAP() {
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  state = NET_IDLE;
  lastRetry = millis();
  Serial.println("Setup hotspot off (hold the button 6 s to open it again)");
}

void netSetup() {
  loadWifi();
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);

  // The network stack must be up before the web server starts
  if (wifiSsid.isEmpty()) {
    startAP();
  } else {
    WiFi.mode(WIFI_STA);
    beginSTA();
    state = NET_CONNECTING;
    stateSince = millis();
    Serial.printf("Connecting to %s...\n", wifiSsid.c_str());
  }

  setupRoutes();
  server.begin();
}

void netLoop() {
  server.handleClient();
  bool haveSaved = !wifiSsid.isEmpty();

  switch (state) {
    case NET_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) onConnected();
      else if (millis() - stateSince > 20000) {
        Serial.println("Wi-Fi connection failed, starting setup hotspot");
        startAP();
      }
      break;

    case NET_AP:
      dns.processNextRequest();
      if (joinState == JOIN_TRYING) {
        if (WiFi.status() == WL_CONNECTED) {
          joinState = JOIN_OK;
          joinSince = millis();
          Serial.printf("Joined %s, address %s\n", wifiSsid.c_str(), WiFi.localIP().toString().c_str());
        } else if (millis() - joinSince > JOIN_TIMEOUT_MS) {
          joinState = JOIN_FAIL;
          WiFi.disconnect(false);
        }
        stateSince = millis();
      } else if (joinState == JOIN_OK && millis() - joinSince < JOIN_GRACE_MS) {
        stateSince = millis();   // keep the hotspot up so the phone can read the new address
      } else if (haveSaved && WiFi.status() == WL_CONNECTED) {
        // saved network came back (or was just joined) while in setup mode
        joinState = JOIN_IDLE;
        dns.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        onConnected();
      } else if (WiFi.softAPgetStationNum() > 0) {
        stateSince = millis();   // someone is using the hotspot, keep it open
      } else if (millis() - stateSince > AP_TIMEOUT_MS) {
        stopAP();
        if (haveSaved) beginSTA();
      } else if (haveSaved && joinState != JOIN_FAIL && millis() - lastRetry > RETRY_MS) {
        lastRetry = millis();
        beginSTA();
      }
      break;

    case NET_IDLE:
      if (haveSaved) {
        if (WiFi.status() == WL_CONNECTED) onConnected();
        else if (millis() - lastRetry > RETRY_MS) {
          lastRetry = millis();
          beginSTA();
        }
      }
      break;

    case NET_CONNECTED:
      break;   // drops are handled by auto-reconnect
  }

  if (restartAt && (int32_t)(millis() - restartAt) > 0) ESP.restart();
}
