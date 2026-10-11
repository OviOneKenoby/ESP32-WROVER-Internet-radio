# TUNER//01 1.1.4 validation build manifest

- Firmware: `1.1.4-validation`
- Executable commit: `e135a1c0aace945c9b30c4be2b41da086462d8e2`
- Embedded build_git_id: `e135a1c0aace` (also verified in binary bytes)
- Main/PR base: `112d68a49a053d302a87fc5658d497617eedda88`
- PR #5 predecessor head: `459103b89320159897645c491fecf2ee5d74eb0d`
- PR: https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/pull/5
- Binary: `firmware-1.1.4-validation-e135a1c0aace.bin`
- Size: **2,019,072 bytes**
- SHA-256: `5CEC87A6FC902411037DCC9CEB045110B7AFEF3AF8588A2613F7BB36B71AB503`
- Prerelease: https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/tuner-1.1.4-network-validation-e135a1c
- Download: https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/download/tuner-1.1.4-network-validation-e135a1c/firmware-1.1.4-validation-e135a1c0aace.bin
- Publication/download verification: **PASS**. Published prerelease asset was
  downloaded again through GitHub CLI; 2,019,072 bytes and the SHA-256 above
  matched the local clean-build binary. Previous 1.1.2/1.1.3 assets were not
  overwritten.

## Build and tests

- Clean `platformio run -e esp32-dev -t clean`, then
  `platformio run -e esp32-dev`: **PASS**, 221.94 seconds.
- PlatformIO Core 6.2.0; platform espressif32 7.0.1.
- Framework package `3.20017.241212+sha.dcc1105b` (Arduino-ESP32 2.0.17).
- Xtensa toolchain `8.4.0+2021r2-patch5`; esptool 4.11.0.
- Static RAM: **83,504 / 327,680 bytes (25.5%)**.
- Linked flash: **2,012,501 / 3,145,728 bytes (64.0%)**.
- Existing host suite: **266 checks PASS**.
- New writer suite: **2,024 checks PASS**, including pinned ArduinoJson
  exact UTF-8 output/Content-Length, partial sends, retry deadline, no deadline
  renewal on progress, shared header/body deadline, wraparound, terminal
  errors, zero sends, buffering and accepted-versus-sent semantics.
- Host compiler: portable w64devkit 2.4.0, GCC 15.2.0;
  `-std=c++17 -Wall -Wextra -Werror`.
- Existing upstream AudioTools/ADC deprecation warnings remain in firmware
  dependency builds; no dependency changes were made to suppress them.
- Hardware at publication: NOT RUN. Subsequent owner testing: **PARTIAL;
  root-refresh network outage reproduced; incident OPEN**. See
  `OWNER_REPORT_2026-10-11.md` and `ALLOCATION_ANALYSIS.md`. No Codex flash.

Host commands (put the portable compiler's bin directory on PATH):

```powershell
g++ -std=c++17 -Wall -Wextra -Werror -Isrc tests/test_correctness_guards.cpp -o .pio/correctness_tests.exe
.pio/correctness_tests.exe
g++ -std=c++17 -Wall -Wextra -Werror -Isrc -I.pio/libdeps/esp32-dev/ArduinoJson/src tests/test_bounded_web_writer.cpp -o .pio/web_writer_tests.exe
.pio/web_writer_tests.exe
```

Pinned libraries: GxEPD2 1.6.9, Adafruit GFX 1.12.6, Adafruit BusIO 1.17.4,
Bounce2 2.72, ArduinoJson 7.4.3,
ESP32-A2DP `3245602afc494f9e62160a0cfb2af864af45a37f`,
ESP8266Audio `058e131b26e459b9aadcb589a50f07877f1a09fd`.
The existing AAC prebuild patch remains unchanged. `platformio.ini`, GPIO,
audio/BT teardown, partitions and other pinned files are unchanged relative
to PR predecessor. The configured memory_type selects the existing prebuilt
`esp32/qio_qspi` SDK and its effective sdkconfig; no sdkconfig rebuild/override.

Executable files were committed and unchanged during the final clean build.
Later commits contain documentation/manifest updates only. The release tag
targets the executable commit, so its code archive may precede this manifest;
this manifest is also attached as a prerelease asset.
