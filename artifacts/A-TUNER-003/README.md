# A-TUNER-003 identified owner-regression artifact

This is a **non-production test artifact**, not a firmware release or hardware
acceptance claim.

- Reviewed source commit: `8fac1da6ccd749a47843d99dc746fc3d6e4bf19b`
- Ordinary integration merge: `d1e51765179f99567df3b5d18a451152adac514c`
- Reviewed and merged Git tree: `01a23382bf54be3e1a04fa992984e62c4570be8d`
- Embedded `/api/diagnostics` build ID: `8fac1da6ccd7`
- Artifact: `firmware.bin`, 1,999,408 bytes
- SHA-256: `AF066BA215F863A7D2583A6313ACEE00020CCFAD4F8F8C3168DEDA76BCD6F5D4`
- Download: https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/download/a-tuner-003-regression-8fac1da/firmware.bin
- Prerelease page: https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/a-tuner-003-regression-8fac1da

The artifact was recovered from the original isolated clean build at
`C:\Users\RYZEN\AppData\Local\Temp\a-tuner-002-build-8fac1da`. Its bytes match
the A-TUNER-002 recorded hash exactly; it was not rebuilt or relabeled.

## Pinned build identity

- Environment: `esp32-dev`, PlatformIO Core 6.2.0
- Platform: `espressif32` 7.0.1
- Framework package: `framework-arduinoespressif32` 3.20017.241212+sha.dcc1105b
- Toolchain: Xtensa 8.4.0+2021r2-patch5
- ESP32-A2DP: `3245602afc494f9e62160a0cfb2af864af45a37f`
- ESP8266Audio: `058e131b26e459b9aadcb589a50f07877f1a09fd`
- GxEPD2 1.6.9, Adafruit GFX 1.12.6, Adafruit BusIO 1.17.4,
  Bounce2 2.72.0 and ArduinoJson 7.4.3
- Linked RAM: 83,056 / 327,680 bytes (25.3%)
- Linked flash: 1,992,833 / 3,145,728 bytes (63.4%)

## Hardware/configuration identity

The reviewed configuration remains `esp32dev`, classic ESP32-WROVER with
PSRAM flags, 240 MHz CPU, 80 MHz flash, DIO and the framework `huge_app.csv`
4 MB layout. Application offset is `0x10000`; NVS is separate at `0x9000`.
The established GPIO configuration remains unchanged: e-paper CS 5, CLK 18,
MOSI 23, RES 19, DC 21, BUSY 4; I2S BCK 26, WS 25, DIN 33; buttons 34/35/39;
encoder CLK 13, DT 32, switch 14.

See [OWNER_REGRESSION_CHECKLIST.md](OWNER_REGRESSION_CHECKLIST.md) before any
owner-controlled flash. Do not use an erase command and do not assume a COM
port. The executor did not upload, reset or access hardware.
