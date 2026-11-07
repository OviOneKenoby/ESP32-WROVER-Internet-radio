# 1.1.12 release image - input hardening

Firmware **1.1.12 / 684abe7bee3a** (app image only, no .elf).
Source commit: 684abe7bee3a63944fbe2d07d8eeaade051e26c5.
Explicit environment esp32-lwip-psram. Arduino-ESP32 2.0.17 / ESP-IDF 4.4.7,
espressif32@7.0.1, framework 3.20017.241212+sha.dcc1105b, Xtensa
8.4.0+2021r2-patch5, esptool 4.11.0.

Build SUCCESS, RAM 81816 B, flash 2205757 B. The code is the same as the
hardware-checked build 074f328758c3 (tasks 045 and 046) plus licence comments
in four vendored files and documentation; the build id and the path strings
differ. This file does not claim a hardware test of this exact image.

- BIN 2212336 B: firmware-1.1.12-684abe7bee3a.bin.
  SHA256 76C4D9230BA5764C1BDB9C161DA6776E7DAFCFEE09058A647BB3D1AC55A3615B.

## Build

Run from a clean checkout of the commit above:

    PLATFORMIO_BUILD_FLAGS="-ffile-prefix-map=<platformio-home>=/pio -ffile-prefix-map=<repository-root>=/src" pio run -e esp32-lwip-psram

The two prefix maps replace the build machine's PlatformIO folder and the
repository folder in the compiled-in file names; no project file is edited.
The image contains no user-profile path of the build machine.

## Flash

The image is the application only. The device must already have a bootloader
and partition table (flash them once from your own build). Then:

    esptool.py --chip esp32 --port <PORT> --baud 115200 --before default_reset --after hard_reset write_flash --flash_mode keep --flash_freq keep --flash_size keep 0x10000 firmware-1.1.12-684abe7bee3a.bin

## Licences

The image is built from this repository (GNU General Public License,
version 3, see LICENSE) and third-party components listed in
THIRD_PARTY_NOTICES.md.
