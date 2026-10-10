# Firmware 1.1.2 Wi-Fi validation build manifest

- Firmware source commit: `7cf087f8c68606b9de6fc38ea400c9bf54b1b5d5`
- Embedded `build_git_id`: `7cf087f8c686`
- Documentation branch head: recorded by the PR; later documentation commits
  do not alter this identified binary.
- Target: `esp32-dev`
- PlatformIO Core: 6.2.0
- Platform: Espressif 32 7.0.1
- Framework package: `framework-arduinoespressif32`
  `3.20017.241212+sha.dcc1105b`
- Toolchain: `toolchain-xtensa-esp32` 8.4.0+2021r2-patch5
- Clean commands: `platformio run -e esp32-dev -t clean`, followed by
  `platformio run -e esp32-dev`
- Clean build: PASS
- Linked static RAM: 83,288 / 327,680 bytes (25.4%)
- Linked flash: 2,004,813 / 3,145,728 bytes (63.7%)
- Binary: `firmware-1.1.2-7cf087f8c686.bin`
- Binary bytes: 2,011,392
- Binary SHA-256:
  `847C9FE99256F6A63EB19C27543399A1BAC46979D4658951E806D56B15E094EA`
- Validation prerelease:
  https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/tuner-1.1.2-wifi-validation-7cf087f
- Binary download:
  https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/download/tuner-1.1.2-wifi-validation-7cf087f/firmware-1.1.2-7cf087f8c686.bin
- Uploaded binary verification: PASS; downloaded bytes and SHA-256 matched.
- ELF string inspection: embedded build ID matched `7cf087f8c686`.
- Host suite: PASS, 264 checks with `-Wall -Wextra -Werror`.
- Hardware flash/run of 1.1.2: NOT PERFORMED / PENDING OWNER TEST.

Exact resolved dependency graph: GxEPD2 1.6.9, Adafruit GFX 1.12.6,
Adafruit BusIO 1.17.4, Bounce2 2.72.0, ArduinoJson 7.4.3, ESP32-A2DP
1.8.11 at `3245602afc494f9e62160a0cfb2af864af45a37f`, ESP8266Audio 2.2.0 at
`058e131b26e459b9aadcb589a50f07877f1a09fd`, plus framework HTTPClient,
WiFi, WiFiClientSecure, Wire, SPI, WebServer, DNSServer, EEPROM and Preferences
at 2.0.0. No dependency was changed or upgraded.
