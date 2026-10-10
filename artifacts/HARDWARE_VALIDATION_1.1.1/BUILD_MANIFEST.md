# Firmware 1.1.1 build manifest

- Firmware source commit: `9f021696cfb76d05e293ae6ca2f70545178653de`
- Embedded `build_git_id`: `9f021696cfb7`
- Target: `esp32-dev`
- PlatformIO Core: 6.2.0
- Platform: Espressif 32 7.0.1
- Framework package: `framework-arduinoespressif32`
  `3.20017.241212+sha.dcc1105b`
- Toolchain: `toolchain-xtensa-esp32` 8.4.0+2021r2-patch5
- Clean command: `platformio run -e esp32-dev -t clean`, followed by
  `platformio run -e esp32-dev`
- Clean build result: SUCCESS
- Linked static RAM: 83,160 / 327,680 bytes (25.4%)
- Linked flash: 1,997,081 / 3,145,728 bytes (63.5%)
- Binary: `firmware-1.1.1-9f021696cfb7.bin`
- Binary bytes: 2,003,664
- Binary SHA-256:
  `AC323E354801857AE0E15F55CF09B3AC7D2DEBE1A48291D1F752EEF2B8DFF84E`
- Validation prerelease:
  https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/tuner-1.1.1-validation-9f02169
- Binary download:
  https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/download/tuner-1.1.1-validation-9f02169/firmware-1.1.1-9f021696cfb7.bin
- ELF string inspection found the embedded build ID exactly once.
- Existing deterministic host suite: PASS, 249 checks (`-Wall -Wextra
  -Werror`).
- Static repair assertions: PASS (UTF-8 HTML/HTTP declarations and entities,
  no executable `end(true)` call, restartable stop present, LOW pressed state,
  plain GPIO34 input, boot-release gate present).
- Hardware flash/run: NOT PERFORMED / PENDING OWNER TEST.

The dependency graph resolved exactly to GxEPD2 1.6.9, Adafruit GFX 1.12.6,
Adafruit BusIO 1.17.4, Bounce2 2.72.0, ArduinoJson 7.4.3, ESP32-A2DP
1.8.11 at `3245602afc494f9e62160a0cfb2af864af45a37f`, ESP8266Audio
2.2.0 at `058e131b26e459b9aadcb589a50f07877f1a09fd`, and the framework's
HTTPClient/WiFi/WiFiClientSecure/Wire/SPI/WebServer/DNSServer/EEPROM/
Preferences libraries at 2.0.0. No dependency update was requested or made.
