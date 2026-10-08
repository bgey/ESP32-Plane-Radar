## Which file do I need?

Pick the files for **your board**. They are not interchangeable.

| Board | Display | Files |
|-------|---------|-------|
| **s3mini** | Wemos S3 Mini + 4" 480x320 ST7796S touch display | `*-s3mini-full.bin`, `*-s3mini-ota.bin` |
| **supermini** | ESP32-C3 Super Mini + 1.28" round GC9A01 | `*-supermini-full.bin`, `*-supermini-ota.bin` |

- `-full.bin`: first install or recovery. Flash at offset `0x0` over USB (for example with [esptool-js](https://espressif.github.io/esptool-js/)). This erases saved settings (Wi-Fi, saved places, touch calibration).
- `-ota.bin`: later updates, uploaded on the device's web page under **Firmware update**. Keeps your settings.
- `.sha256`: checksum of the file with the same name.
