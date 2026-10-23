#include "settings.h"
#include <Preferences.h>

Settings S;

static Preferences prefs;
static uint32_t dirtyAt = 0;

static uint32_t toU32(const CRGB& c) { return ((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b; }
static CRGB fromU32(uint32_t v) { return CRGB((v >> 16) & 255, (v >> 8) & 255, v & 255); }

void settingsLoad() {
  prefs.begin("mk", true);
  S.on = prefs.getBool("on", S.on);
  S.mode = prefs.getUChar("md", S.mode);
  S.brightness = prefs.getUChar("bri", S.brightness);
  S.speed = prefs.getUChar("spd", S.speed);
  S.color1 = fromU32(prefs.getULong("c1", toU32(S.color1)));
  S.color2 = fromU32(prefs.getULong("c2", toU32(S.color2)));
  S.autoCycle = prefs.getUShort("auto", S.autoCycle);
  S.maxMilliamps = prefs.getUShort("ma", S.maxMilliamps);
  S.sign = prefs.getString("sign", S.sign);
  prefs.getBytes("sm", S.symbolMode, sizeof(S.symbolMode));
  prefs.end();
}

void settingsSaveNow() {
  prefs.begin("mk", false);
  prefs.putBool("on", S.on);
  prefs.putUChar("md", S.mode);
  prefs.putUChar("bri", S.brightness);
  prefs.putUChar("spd", S.speed);
  prefs.putULong("c1", toU32(S.color1));
  prefs.putULong("c2", toU32(S.color2));
  prefs.putUShort("auto", S.autoCycle);
  prefs.putUShort("ma", S.maxMilliamps);
  prefs.putString("sign", S.sign);
  prefs.putBytes("sm", S.symbolMode, sizeof(S.symbolMode));
  prefs.end();
}

void settingsChanged() { dirtyAt = millis() | 1; }

void settingsLoop() {
  if (dirtyAt && millis() - dirtyAt > 3000) {
    settingsSaveNow();
    dirtyAt = 0;
  }
}
