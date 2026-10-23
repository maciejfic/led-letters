# Hardware

## Parts

| Part | Notes |
|---|---|
| ESP32-C3 board with USB-C | Tested with an ESP32-C3 "SuperMini" style board (USB-C, BOOT and RESET buttons). |
| WS2812B LED strip, 5 V | 10 mm wide, 60 LEDs per metre, cut into the pocket lengths of each glyph insert. |
| Wire | Three conductors between glyphs: 5 V, data, GND. |
| 5 V power supply | See [Power](#power). |
| 330 Ω resistor (optional) | In series with the data line, as close to the first LED as possible. |
| 470–1000 µF capacitor (optional) | Across 5 V and GND at the start of the chain. Protects the first LED at power-on. |

## Wiring

```
 ESP32-C3                    first glyph                     next glyph
 ┌──────────┐               ┌──────────────┐               ┌──────────────┐
 │       5V ├───────────────┤ 5V       5V  ├───────────────┤ 5V           │
 │   GPIO 4 ├──[330 Ω]──────┤ DIN     DOUT ├───────────────┤ DIN     ...  │
 │      GND ├───────────────┤ GND     GND  ├───────────────┤ GND          │
 └──────────┘               └──────────────┘               └──────────────┘
```

- **Data goes in at DIN.** The arrows printed on the strip point away from DIN.
- **Glyph 1** is the one the controller's data wire goes into. In the app you add glyphs in this
  chain order, which is also the left-to-right order of the sign.
- Every glyph is wired the same way inside: its strip pieces are joined in one chain from its entry
  hole to its exit hole.

### Pins

| Function | Default | Change in |
|---|---|---|
| LED data | GPIO 4 | `LED_PIN` in `firmware/LedLetters/config.h` |
| Button | GPIO 9 (on-board BOOT) | `BUTTON_PIN` |

Avoid GPIO 2, 8 and 9 for the LEDs (they decide how the chip boots), GPIO 18/19 (USB) and
GPIO 12–17 (flash on most modules). GPIO 0, 1, 3, 5, 6, 7 and 10 are free.

### Data signal level

The ESP32 outputs 3.3 V data; WS2812B LEDs formally want about 3.5 V at 5 V supply. With a short
wire (under about 30 cm) it works reliably. If the first LEDs flicker or show random colours, add a
74AHCT125 / SN74AHCT1G125 level shifter between GPIO 4 and DIN.

## Power

Each LED draws up to about 60 mA at full white, so a sign of 100 LEDs could draw 6 A. The firmware
never lets it get there: it limits the total current (**500 mA by default**, set in the app under
System) and scales brightness down automatically.

| Sign | Supply |
|---|---|
| 1–4 glyphs | A good 5 V USB phone charger (1 A or more) into the controller's USB-C port. |
| 5–6 glyphs | A stable 5 V supply rated for the current limit you set. Feed power into the start of the chain, and ideally also into the end. |

- **Six glyphs is the maximum** the app allows: more would overload the current the controller and
  wiring can carry.
- **Raising the limit above 500 mA** is possible in the app, but only do it if the supply, wiring and
  connectors are rated for it. The app shows a warning.
- Powered over the controller's USB-C, the LED current flows through the board. Keep the limit at
  500 mA in that case.

## Soldering the glyphs

> **Coming soon.** Step-by-step soldering of the LED strip into the glyph inserts, joining the
> glyphs and connecting the controller will be added here.

## Light sensor (planned)

Support for a GY-302 (BH1750) light sensor for automatic brightness is planned. Reserved wiring:
VCC → 3V3, GND → GND, SDA → GPIO 5, SCL → GPIO 6, ADDR unconnected.
