#include "effects.h"
#include "config.h"
#include "layout.h"
#include "settings.h"

static CRGB leds[MAX_LEDS];   // what goes out to the strip
// Layer 0 is the normal effect buffer; layers 1.. are used by the Mixed mode (one per symbol)
static CRGB layers[MAX_SYMBOLS + 1][MAX_LEDS];
static CRGB* fx = layers[0];          // effects draw here
static CRGB from[MAX_LEDS];   // snapshot for cross-fading between modes

static uint32_t T = 0;              // effect time in ms, scaled by speed
static uint64_t clockAcc = 0;       // T * 50
static uint32_t lastFrame = 0;
static uint32_t transitionStart = 0;
static bool inTransition = false;
static uint32_t modeSince = 0;
static uint8_t curBrightness = 0;

static const uint16_t TRANSITION_MS = 700;

// ---------- helpers ----------

// inoise8 mostly returns 60-190; stretch it to the full range
static uint8_t stretch(uint8_t n) {
  int v = ((int)n - 64) * 2;
  return constrain(v, 0, 255);
}

static CRGB scaled(CRGB c, uint8_t b) {
  c.nscale8_video(b);
  return c;
}

static uint8_t pulse(int32_t x, int32_t len) {
  if (x < 0 || x >= len) return 0;
  uint32_t f = 255 - x * 255 / len;
  return f * f / 255;
}

// ---------- effects ----------

static void fxSolid() { fill_solid(fx, numLeds, S.color1); }

static void fxRainbow() {
  for (uint16_t i = 0; i < numLeds; i++)
    fx[i] = CHSV((uint8_t)(T / 16) - (uint8_t)(i * 256 / numLeds), 255, 255);
}

static void fxRainbowSweep() {
  for (uint16_t i = 0; i < numLeds; i++) fx[i] = CHSV((uint8_t)(T / 16) - ledX[i], 255, 255);
}

static void fxRainbowRise() {
  for (uint16_t i = 0; i < numLeds; i++) fx[i] = CHSV((uint8_t)(T / 16) + ledY[i], 255, 255);
}

static void fxLetterColors() {
  uint8_t base = T / 32;
  for (uint16_t i = 0; i < numLeds; i++) fx[i] = CHSV(base + ledSymbol[i] * 256 / numSymbols, 230, 255);
}

static void fxGradient() {
  for (uint16_t i = 0; i < numLeds; i++)
    fx[i] = blend(S.color1, S.color2, sin8(ledX[i] / 2 + T / 16));
}

static void fxHeartbeat() {
  int32_t p = T % 1100;
  uint16_t h = pulse(p, 350) + pulse(p - 280, 350) * 3 / 4;
  if (h > 255) h = 255;
  CRGB heart = scaled(S.color2, 40 + scale8(h, 215));
  CRGB base = scaled(S.color1, 90);
  for (uint16_t i = 0; i < numLeds; i++) fx[i] = symbolPulses[ledSymbol[i]] ? heart : base;
}

static void fxFire() {
  for (uint16_t i = 0; i < numLeds; i++) {
    uint8_t n = stretch(inoise8(ledX[i] * 3, ledY[i] * 3 + T / 2, T / 6));
    uint8_t heat = scale8(n, 70 + ledY[i] * 2 / 3);   // hotter at the bottom
    fx[i] = ColorFromPalette(HeatColors_p, scale8(heat, 240));
  }
}

static void fxTestLetters() {
  // every glyph its own colour, to check the LED counts after setup
  for (uint16_t i = 0; i < numLeds; i++) fx[i] = CHSV(ledSymbol[i] * 256 / numSymbols, 255, 255);
}

static void fxMixed();

// ---------- mode table ----------

struct Mode { const char* name; void (*fn)(); bool test; };

static const Mode MODES[] = {
  {"Solid",          fxSolid,        false},
  {"Rainbow",        fxRainbow,      false},
  {"Rainbow sweep",  fxRainbowSweep, false},
  {"Rainbow rise",   fxRainbowRise,  false},
  {"Glyph colors",   fxLetterColors, false},
  {"Gradient wave",  fxGradient,     false},
  {"Heartbeat",      fxHeartbeat,    false},
  {"Fire",           fxFire,         false},
  {"Mixed",          fxMixed,        false},
  {"Test: glyphs",   fxTestLetters,  true},
};
static const uint8_t MODE_COUNT = sizeof(MODES) / sizeof(MODES[0]);

// Mixed: every symbol runs its own effect (S.symbolMode), each in its own layer
static const uint8_t MIXED_MODE = MODE_COUNT - 2;   // "Mixed" is second to last

static void fxMixed() {
  CRGB* out = fx;
  for (uint8_t L = 0; L < numSymbols; L++) {
    uint8_t m = S.symbolMode[L];
    if (m >= MODE_COUNT || m == MIXED_MODE) m = 1;
    fx = layers[L + 1];
    MODES[m].fn();
    memcpy(out + symbolStart[L], fx + symbolStart[L], symbolLeds[L] * sizeof(CRGB));
  }
  fx = out;
}

uint8_t mixedMode() { return MIXED_MODE; }

uint8_t modeCount() { return MODE_COUNT; }
const char* modeName(uint8_t m) { return m < MODE_COUNT ? MODES[m].name : ""; }
bool modeIsTest(uint8_t m) { return m < MODE_COUNT && MODES[m].test; }

void setMode(uint8_t m) {
  if (m >= MODE_COUNT) m = 0;
  if (m == S.mode) return;
  memcpy(from, leds, sizeof(leds));
  memset(layers, 0, sizeof(layers));
  S.mode = m;
  clockAcc = 0;
  T = 0;
  transitionStart = millis();
  inTransition = true;
  modeSince = millis();
}

void nextMode() {
  uint8_t m = S.mode;
  do { m = (m + 1) % MODE_COUNT; } while (MODES[m].test);
  setMode(m);
}

// ---------- engine ----------

void effectsInit() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, numLeds).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, S.maxMilliamps);
  FastLED.setBrightness(0);
  FastLED.clear(true);
  if (S.mode >= MODE_COUNT) S.mode = 1;
  lastFrame = modeSince = millis();
}

void effectsLoop() {
  uint32_t now = millis();
  if (now - lastFrame < 16) return;   // ~60 fps
  uint32_t dt = min<uint32_t>(now - lastFrame, 100);
  lastFrame = now;

  clockAcc += (uint64_t)dt * S.speed;
  T = clockAcc / 50;

  if (S.on && S.autoCycle && !modeIsTest(S.mode) && now - modeSince > S.autoCycle * 1000UL) {
    nextMode();
    settingsChanged();
  }

  MODES[S.mode].fn();

  if (inTransition) {
    uint32_t e = now - transitionStart;
    if (e >= TRANSITION_MS) {
      inTransition = false;
      memcpy(leds, fx, sizeof(leds));
    } else {
      uint8_t a = ease8InOutQuad(e * 255 / TRANSITION_MS);
      for (uint16_t i = 0; i < numLeds; i++) leds[i] = blend(from[i], fx[i], a);
    }
  } else {
    memcpy(leds, fx, sizeof(leds));
  }

  // smooth power on/off and brightness changes
  uint8_t target = S.on ? S.brightness : 0;
  if (curBrightness < target) curBrightness = min<int>(target, curBrightness + 4);
  else if (curBrightness > target) curBrightness = max<int>(target, curBrightness - 4);

  FastLED.setMaxPowerInVoltsAndMilliamps(5, S.maxMilliamps);
  FastLED.setBrightness(curBrightness);
  FastLED.show();
}

void flashAll(CRGB color, uint8_t times) {
  FastLED.setBrightness(80);
  for (uint8_t t = 0; t < times; t++) {
    fill_solid(leds, numLeds, color);
    FastLED.show();
    delay(150);
    FastLED.clear(true);
    delay(150);
  }
  FastLED.setBrightness(curBrightness);
}
