#pragma once
#include <stdint.h>

#define FW_VERSION    "2.0.0"

// Hardware (constants, not macros: FastLED uses names like DATA_PIN internally)
constexpr uint8_t LED_PIN    = 4;
constexpr uint8_t BUTTON_PIN = 9;     // on-board BOOT button
// LED chip and colour order are set in effectsInit() (WS2812B, GRB)

// Sign limits
constexpr uint8_t  MAX_GLYPHS      = 6;     // more would overload the power supply
constexpr uint8_t  WARN_GLYPHS     = 4;     // above this the web app warns about the power supply
constexpr uint8_t  MIN_GLYPH_LEDS  = 2;
constexpr uint8_t  MAX_GLYPH_LEDS  = 60;
constexpr uint16_t MAX_LEDS        = MAX_GLYPHS * MAX_GLYPH_LEDS;
constexpr uint16_t UNCONFIGURED_LEDS = 150; // lit as a plain strip until the glyphs are set up

// Network
#define HOSTNAME      "led-letters"         // http://led-letters.local
#define AP_SSID       "LED-Letters-Setup"   // open hotspot used for setup

// Defaults
constexpr uint16_t DEFAULT_MAX_MA = 500;  // total current limit for the LEDs (mA)
constexpr uint16_t SAFE_MA        = 500;  // above this the web app warns about risking damage
