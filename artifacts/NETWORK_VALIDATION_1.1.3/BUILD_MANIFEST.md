# TUNER//01 1.1.3 network instrumentation build manifest

- Firmware version: `1.1.3-validation`
- Source commit: `603c4b12a56b83cae712e891c5b1f41b4c158ca9`
- Embedded `build_git_id`: `603c4b12a56b`
- Environment: `esp32-dev`
- PlatformIO Core: 6.2.0
- Platform: `espressif32@7.0.1`
- Arduino-ESP32 package: `3.20017.241212+sha.dcc1105b` (Arduino 2.0.17)
- Toolchain: `toolchain-xtensa-esp32@8.4.0+2021r2-patch5`
- Partition table: unchanged `huge_app.csv`
- Static RAM: 83,336 / 327,680 bytes (25.4%)
- Flash: 2,009,881 / 3,145,728 bytes (63.9%)
- Binary: `firmware-1.1.3-validation-603c4b12a56b.bin`
- Binary size: 2,016,464 bytes
- SHA-256: `BDB7CA191AEA282A6F7F27D0FF5B12EF04A6422547AFDC1B4514006F9BDE716F`
- Validation release: https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/tuner-1.1.3-network-validation-603c4b1
- Binary download: https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/download/tuner-1.1.3-network-validation-603c4b1/firmware-1.1.3-validation-603c4b12a56b.bin
- Post-publication verification: downloaded size and SHA-256 matched.

Pinned project libraries were unchanged:

- GxEPD2 1.6.9
- Adafruit GFX 1.12.6
- Adafruit BusIO 1.17.4
- Bounce2 2.72.0
- ArduinoJson 7.4.3
- ESP32-A2DP commit `3245602afc494f9e62160a0cfb2af864af45a37f`
- ESP8266Audio commit `058e131b26e459b9aadcb589a50f07877f1a09fd`

Build procedure:

```text
platformio run -e esp32-dev -t clean
platformio run -e esp32-dev
```

The binary was built before this documentation-only manifest commit so the
embedded ID identifies the exact executable source commit. No hardware was
flashed automatically.
