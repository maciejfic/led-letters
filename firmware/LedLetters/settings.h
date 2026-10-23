#pragma once
#include <FastLED.h>
#include "config.h"
#include "layout.h"

struct Settings {
  bool on = true;
  uint8_t mode = 1;
  uint8_t brightness = 120;
  uint8_t speed = 50;                      // 50 = normal
  CRGB color1 = CRGB(255, 120, 40);        // warm white
  CRGB color2 = CRGB(255, 0, 60);          // pink-red
  uint16_t autoCycle = 0;                  // seconds, 0 = off
  uint16_t maxMilliamps = DEFAULT_MAX_MA;
  String sign = "";                        // glyphs in chain order (see layout.h), empty = not set up
  // Mixed mode: effect per glyph (Rainbow sweep, Fire, Rainbow rise, ...)
  uint8_t symbolMode[MAX_SYMBOLS] = {2, 7, 3, 2, 7, 3};
};

extern Settings S;

void settingsLoad();
void settingsChanged();   // schedules a (debounced) save to flash
void settingsSaveNow();
void settingsLoop();
