# 1.1.13 release image - AAC playback fixes

Firmware **1.1.13 / 7b726c2f5e33** (app image only, no .elf).
Source commit: 7b726c2f5e33 (this repository).
Explicit environment esp32-lwip-psram. Arduino-ESP32 2.0.17 / ESP-IDF 4.4.7,
espressif32@7.0.1, framework 3.20017.241212+sha.dcc1105b, Xtensa
8.4.0+2021r2-patch5, esptool 4.11.0.

Build SUCCESS from a clean checkout, RAM 81816 B, flash 2206773 B. Compiler
warnings come from third-party sources only. Host tests: 30 C++ tests, the
library buffer test (fails unpatched, passes patched) and 23 Python tests pass.

- BIN 2213344 B: firmware-1.1.13-7b726c2f5e33.bin.
  SHA256 F8E001C05EDE133FD9668251954D690386341CD26A70642BB44A55EBFA6C46CF.

## Hardware status

This exact image was flashed on one TUNER (ESP32-WROVER) on 2026-11-25 and
checked by listening: one plain-http MP3 station, one https AAC station and one
plain-http AAC station, a few minutes each, all clean, with the remote control
connected and no restart during playback. This is a short check, not a full
validation. Decode errors and delivered audio were not measured on this image,
which has no measurement code. They were measured on build e324ee4 (the same
fixes plus logging): all audio delivered, no decode errors, no underruns.
Bluetooth, pause and volume were not tested.

Start-up observations from the same session:

- After the first write the board restarted repeatedly; the ROM reported
  checksum errors while loading the bootloader, which had not been written.
- The board did not start from its main supply alone until it had been started
  from USB. Afterwards it started from the main supply 3 out of 3 times with
  this image, and 3 out of 3 times with the 1.1.12 image.
- One restart during start-up was a crash in the radio calibration step
  (`esp_efuse_mac_get_default` returned a CRC error). The board recovered by
  itself. A stored crash report from an earlier build, without these fixes,
  shows the same failure.

The causes were not established. Check the power supply and cable if a board
behaves this way.

## Build

Run from a clean checkout of the commit above:

    PLATFORMIO_BUILD_FLAGS="-ffile-prefix-map=<platformio-home>=/pio -ffile-prefix-map=<repository-root>=/src" pio run -e esp32-lwip-psram

## Flash

The image is the application only. The device must already have a bootloader
and partition table (flash them once from your own build). Then:

    esptool.py --chip esp32 --port <PORT> --baud 115200 --before default_reset --after hard_reset write_flash --flash_mode keep --flash_freq keep --flash_size keep 0x10000 firmware-1.1.13-7b726c2f5e33.bin

## Licences

The image is built from this repository (GNU General Public License,
version 3, see LICENSE) and third-party components listed in
THIRD_PARTY_NOTICES.md.
