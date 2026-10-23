#pragma once
#include <Arduino.h>
#include "config.h"

// The sign is a list of glyphs in chain order (= left to right), each with its real LED count,
// e.g. "Heart:27,M:30,Amp:27,K:22". Glyph shapes come from glyph_data.h (tools/gen_glyphs.py).
// An empty spec means "not set up yet": the LEDs are treated as one plain strip.
constexpr uint8_t MAX_SYMBOLS = MAX_GLYPHS;
constexpr uint8_t MAX_SEGMENTS = 64;

// Glyph catalog
uint8_t glyphCount();
const char* glyphName(uint8_t g);     // "Heart", "M", "Amp", ...
const char* glyphLabel(uint8_t g);    // "♥", "M", "&", ...
uint8_t glyphLeds(uint8_t g);         // default LED count

// Checks a sign spec; returns the LED count, or 0 if it is invalid
uint16_t layoutValidate(const String& spec);

// Builds the layout; an empty or invalid spec gives the unconfigured strip
void layoutInit(const String& spec);
bool layoutConfigured();
float layoutAspect();                 // sign width / height, for the web app preview
String layoutSpec();                  // normalised spec of the current layout

// Current layout
extern uint16_t numLeds;
extern uint8_t numSymbols;
extern uint8_t numSegments;
extern uint8_t symbolGlyph[MAX_SYMBOLS];
extern uint16_t symbolStart[MAX_SYMBOLS];
extern uint16_t symbolLeds[MAX_SYMBOLS];
extern bool symbolPulses[MAX_SYMBOLS];
extern uint16_t segStart[MAX_SEGMENTS];   // first LED index of each segment

// Per-LED lookup tables
extern uint8_t ledX[MAX_LEDS];       // 0 = left edge of sign, 255 = right edge
extern uint8_t ledY[MAX_LEDS];       // 0 = top, 255 = bottom
extern uint8_t ledR[MAX_LEDS];       // distance from sign centre, 0-255
extern uint8_t ledSeg[MAX_LEDS];
extern uint8_t ledSymbol[MAX_LEDS];
