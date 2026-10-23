// LED letters - ESP32-C3 + WS2812B
//
// Setup:  on first start (or without a saved network) the sign opens the "LED-Letters-Setup" hotspot.
//         Join it, then set up Wi-Fi and the glyphs on the page that opens (or http://192.168.4.1).
// Web app: http://led-letters.local (or the IP printed on Serial)
//
// BOOT button: short press = next effect (or turn on)
//              hold ~1 s  = power on/off
//              hold 6 s   = forget Wi-Fi (blinks blue) and restart into the setup hotspot

#include "config.h"
#include "layout.h"
#include "settings.h"
#include "effects.h"
#include "net.h"

static struct {
  bool pressed = false;
  uint32_t downAt = 0, lastChange = 0;
  uint8_t stage = 0;   // 0 = short, 1 = long (power), 2 = very long (Wi-Fi reset)
} btn;

static void handleButton() {
  bool down = digitalRead(BUTTON_PIN) == LOW;
  uint32_t now = millis();

  if (down != btn.pressed && now - btn.lastChange > 30) {
    btn.lastChange = now;
    btn.pressed = down;
    if (down) {
      btn.downAt = now;
      btn.stage = 0;
    } else if (btn.stage == 0) {
      if (!S.on) S.on = true;
      else nextMode();
      settingsChanged();
    } else if (btn.stage == 1) {
      S.on = !S.on;
      settingsChanged();
    } else {
      netForgetWifi();
    }
  }

  if (btn.pressed) {
    uint32_t held = now - btn.downAt;
    if (btn.stage == 0 && held >= 800) btn.stage = 1;
    if (btn.stage == 1 && held >= 6000) {
      btn.stage = 2;
      flashAll(CRGB::Blue, 3);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  settingsLoad();
  layoutInit(S.sign);
  effectsInit();
  netSetup();

  Serial.printf("LED letters %s, %u LEDs, %u modes\n", FW_VERSION, numLeds, modeCount());
}

void loop() {
  handleButton();
  netLoop();
  effectsLoop();
  settingsLoop();
}
