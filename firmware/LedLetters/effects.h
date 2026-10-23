#pragma once
#include <FastLED.h>

void effectsInit();
void effectsLoop();

uint8_t modeCount();
const char* modeName(uint8_t m);
bool modeIsTest(uint8_t m);
uint8_t mixedMode();          // index of the per-letter Mixed mode
void setMode(uint8_t m);
void nextMode();

// Direct LED feedback (blocking), used for button actions
void flashAll(CRGB color, uint8_t times);
