# Flashing the firmware

Three ways, from easiest to most flexible. The controller only needs a USB-C data cable.

## 1. From the browser (no software to install)

Works in Chrome or Edge on a computer (not on phones, not in Safari or Firefox).

1. Download **`led-letters-esp32c3-full.bin`** from the latest
   [release](https://github.com/maciejfic/led-letters/releases).
2. Plug the controller into the computer.
3. Open Espressif's official web flasher: **https://espressif.github.io/esptool-js/**
4. Click **Connect** and pick the controller in the list (usually "USB JTAG/serial debug unit").
5. In *Flash Address* enter **`0x0`**, choose the downloaded `.bin` file and click **Program**.
6. When it says it is done, unplug the controller and plug it back in.

If **Connect** finds nothing or fails: hold the **BOOT** button, press and release **RESET**, release
**BOOT**, then click Connect again.

### Updating later without losing settings

`led-letters-esp32c3-full.bin` erases the saved glyphs and Wi-Fi. To update and keep them, flash
**`led-letters-esp32c3-update.bin`** at address **`0x10000`** instead.

## 2. Arduino IDE

1. Install [Arduino IDE 2](https://www.arduino.cc/en/software).
2. **Boards Manager:** install **esp32 by Espressif Systems**, version 3.3.x.
   (If it is not listed, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   under Settings → Additional boards manager URLs.)
3. **Library Manager:** install **FastLED** by Daniel Garcia, version 3.10.x.
4. Open `firmware/LedLetters/LedLetters.ino`.
5. Tools → Board: **ESP32C3 Dev Module**; Tools → **USB CDC On Boot: Enabled**; pick the port.
6. Click **Upload**.

## 3. arduino-cli

```bash
arduino-cli core install esp32:esp32@3.3.10 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "FastLED@3.10.3"
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc firmware/LedLetters
arduino-cli upload  --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc -p /dev/cu.usbmodem2101 firmware/LedLetters
```

On Windows the port is `COM3` or similar; `arduino-cli board list` shows it.

## Troubleshooting

| Problem | Fix |
|---|---|
| "Port is busy or doesn't exist" | The board's USB re-connects while resetting. Try again; if it keeps failing use the BOOT + RESET sequence above. |
| No port appears | Use a USB-C cable that carries data (some are charge-only). |
| It flashed, but nothing happens | Open the serial monitor at 115200 baud and press RESET. A healthy start prints the sign, `Setup hotspot "LED-Letters-Setup" at http://192.168.4.1` and the firmware version. |
