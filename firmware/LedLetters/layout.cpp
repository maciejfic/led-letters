#include "layout.h"
#include <math.h>
#include "glyph_data.h"

static const float GAP_MM = 20;   // space between glyphs on the sign

uint16_t numLeds = 0;
uint8_t numSymbols = 0, numSegments = 0;
uint8_t symbolGlyph[MAX_SYMBOLS];
uint16_t symbolStart[MAX_SYMBOLS], symbolLeds[MAX_SYMBOLS];
bool symbolPulses[MAX_SYMBOLS];
uint16_t segStart[MAX_SEGMENTS];
uint8_t ledX[MAX_LEDS], ledY[MAX_LEDS], ledR[MAX_LEDS], ledSeg[MAX_LEDS], ledSymbol[MAX_LEDS];

static float rawX[MAX_LEDS], rawY[MAX_LEDS];
static bool configured = false;

static float aspect = 1;

bool layoutConfigured() { return configured; }
float layoutAspect() { return aspect; }

uint8_t glyphCount() { return GLYPH_COUNT; }
const char* glyphName(uint8_t g) { return g < GLYPH_COUNT ? GLYPHS[g].name : ""; }
const char* glyphLabel(uint8_t g) { return g < GLYPH_COUNT ? GLYPHS[g].label : ""; }
uint8_t glyphLeds(uint8_t g) { return g < GLYPH_COUNT ? GLYPHS[g].leds : 0; }

static int findGlyph(const String& name) {
  for (uint8_t g = 0; g < GLYPH_COUNT; g++)
    if (name.equalsIgnoreCase(GLYPHS[g].name) || name == GLYPHS[g].label) return g;
  return -1;
}

// Parses "Name:leds,Name:leds,...". Returns the number of items, or -1 if invalid.
static int parseSpec(const String& spec, uint8_t* glyph, uint16_t* leds) {
  int n = 0, pos = 0;
  uint32_t total = 0;
  String s = spec;
  s.trim();
  if (s.isEmpty()) return -1;
  while (pos <= (int)s.length()) {
    int comma = s.indexOf(',', pos);
    if (comma < 0) comma = s.length();
    String item = s.substring(pos, comma);
    item.trim();
    pos = comma + 1;
    if (item.isEmpty()) continue;
    if (n >= MAX_GLYPHS) return -1;
    int colon = item.indexOf(':');
    String name = colon < 0 ? item : item.substring(0, colon);
    name.trim();
    int g = findGlyph(name);
    if (g < 0) return -1;
    int count = colon < 0 ? GLYPHS[g].leds : item.substring(colon + 1).toInt();
    if (count < MIN_GLYPH_LEDS || count > MAX_GLYPH_LEDS) return -1;
    glyph[n] = g;
    leds[n] = count;
    total += count;
    n++;
  }
  if (n == 0 || total > MAX_LEDS) return -1;
  return n;
}

uint16_t layoutValidate(const String& spec) {
  uint8_t g[MAX_SYMBOLS];
  uint16_t l[MAX_SYMBOLS];
  int n = parseSpec(spec, g, l);
  if (n < 0) return 0;
  uint16_t total = 0;
  for (int i = 0; i < n; i++) total += l[i];
  return total;
}

String layoutSpec() {
  String s;
  if (!configured) return s;
  for (uint8_t i = 0; i < numSymbols; i++) {
    if (i) s += ",";
    s += GLYPHS[symbolGlyph[i]].name;
    s += ":";
    s += symbolLeds[i];
  }
  return s;
}

// Position of the point at distance d along a polyline
static void pointAlong(const int16_t (*p)[2], uint8_t n, float d, float& x, float& y) {
  for (uint8_t i = 0; i + 1 < n; i++) {
    float dx = p[i + 1][0] - p[i][0], dy = p[i + 1][1] - p[i][1];
    float len = sqrtf(dx * dx + dy * dy);
    if (d <= len || i + 2 == n) {
      float t = len > 0 ? fminf(d / len, 1.0f) : 0;
      x = p[i][0] + dx * t;
      y = p[i][1] + dy * t;
      return;
    }
    d -= len;
  }
  x = p[0][0];
  y = p[0][1];
}

static float polyLength(const int16_t (*p)[2], uint8_t n) {
  float total = 0;
  for (uint8_t i = 0; i + 1 < n; i++) total += hypotf(p[i + 1][0] - p[i][0], p[i + 1][1] - p[i][1]);
  return total;
}

void layoutInit(const String& spec) {
  uint16_t want[MAX_SYMBOLS];
  int n = parseSpec(spec, symbolGlyph, want);
  numLeds = 0;
  numSegments = 0;
  configured = n > 0;

  if (!configured) {
    // Not set up yet: one straight strip, so effects still show something
    if (!spec.isEmpty()) Serial.printf("Invalid sign \"%s\"\n", spec.c_str());
    numSymbols = 1;
    numSegments = 1;
    symbolGlyph[0] = 0;
    symbolStart[0] = 0;
    symbolLeds[0] = numLeds = UNCONFIGURED_LEDS;
    symbolPulses[0] = false;
    segStart[0] = 0;
    for (uint16_t i = 0; i < numLeds; i++) {
      ledX[i] = i * 255 / (numLeds - 1);
      ledY[i] = 128;
      ledR[i] = abs((int)ledX[i] - 128) * 2;
      ledSeg[i] = ledSymbol[i] = 0;
    }
    aspect = 24;
    Serial.println("Sign not set up yet");
    return;
  }
  numSymbols = n;
  float originX = 0;

  for (uint8_t s = 0; s < numSymbols; s++) {
    const GlyphDef& g = GLYPHS[symbolGlyph[s]];
    symbolStart[s] = numLeds;
    symbolLeds[s] = want[s];
    symbolPulses[s] = g.pulses;

    // Spread the real LED count over the glyph's segments in proportion to their design counts
    uint16_t designTotal = 0;
    for (uint8_t k = 0; k < g.numSegs; k++) designTotal += GLYPH_SEGS[g.firstSeg + k].leds;
    uint16_t given = 0;
    for (uint8_t k = 0; k < g.numSegs; k++) {
      const GlyphSeg& seg = GLYPH_SEGS[g.firstSeg + k];
      uint32_t acc = 0;
      for (uint8_t j = 0; j <= k; j++) acc += GLYPH_SEGS[g.firstSeg + j].leds;
      uint16_t upTo = designTotal ? (acc * want[s] + designTotal / 2) / designTotal : want[s];
      uint16_t count = upTo - given;
      given = upTo;
      if (count == 0) continue;

      if (numSegments < MAX_SEGMENTS) segStart[numSegments] = numLeds;
      const int16_t (*p)[2] = &GLYPH_PTS[seg.firstPt];
      float len = polyLength(p, seg.numPts);
      for (uint16_t i = 0; i < count && numLeds < MAX_LEDS; i++, numLeds++) {
        float x, y;
        if (seg.numPts == 1) { x = p[0][0]; y = p[0][1]; }
        else pointAlong(p, seg.numPts, count == 1 ? len / 2 : len * i / (count - 1), x, y);
        rawX[numLeds] = originX + x;
        rawY[numLeds] = y;
        ledSeg[numLeds] = numSegments < MAX_SEGMENTS ? numSegments : MAX_SEGMENTS - 1;
        ledSymbol[numLeds] = s;
      }
      if (numSegments < MAX_SEGMENTS) numSegments++;
    }
    originX += g.widthMm + GAP_MM;
  }

  float minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
  for (uint16_t i = 0; i < numLeds; i++) {
    minX = fminf(minX, rawX[i]); maxX = fmaxf(maxX, rawX[i]);
    minY = fminf(minY, rawY[i]); maxY = fmaxf(maxY, rawY[i]);
  }
  aspect = (maxX - minX) / fmaxf(maxY - minY, 1);
  float cx = (minX + maxX) / 2, cy = (minY + maxY) / 2, maxR = 1;
  for (uint16_t i = 0; i < numLeds; i++) maxR = fmaxf(maxR, hypotf(rawX[i] - cx, rawY[i] - cy));
  for (uint16_t i = 0; i < numLeds; i++) {
    ledX[i] = (rawX[i] - minX) * 255 / fmaxf(maxX - minX, 1);
    ledY[i] = (maxY - rawY[i]) * 255 / fmaxf(maxY - minY, 1);   // glyph y is up, ledY 0 = top
    ledR[i] = hypotf(rawX[i] - cx, rawY[i] - cy) * 255 / maxR;
  }

  Serial.printf("Sign %s: %u LEDs, %u segments\n", layoutSpec().c_str(), numLeds, numSegments);
}
