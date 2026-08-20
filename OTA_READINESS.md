# OTA readiness analysis

## Decision

OTA is **not enabled for V1**. The configured target is 4 MB and the proposed
dual-slot layout technically accommodates the current firmware, but the margin
is too small for a safe update path. The physical board's flash size also still
requires hardware confirmation through the boot log or `/api/diagnostics`.

## Current layout and firmware

The project uses Arduino-ESP32's `huge_app.csv`:

- NVS: `0x5000` (20,480 bytes)
- OTA data: `0x2000` (8,192 bytes)
- One application slot: `0x300000` (3,145,728 bytes)
- SPIFFS: `0xE0000` (917,504 bytes)
- Coredump: `0x10000` (65,536 bytes)

The clean reconstructed V1 diagnostics build produced a `firmware.bin` of
1,998,768 bytes (1,992,189 bytes reported by the linker).

## Proposed 4 MB OTA layout

`partitions/ota_4mb_proposed.csv` retains the existing 20 KiB NVS area, 8 KiB
OTA metadata area and 64 KiB coredump area. The unused filesystem allocation is
removed because this firmware does not use SPIFFS. The remaining flash is split
into two equal application slots:

- `ota_0`: `0x1F0000` (2,031,616 bytes)
- `ota_1`: `0x1F0000` (2,031,616 bytes)

Against the 1,998,768-byte binary, each slot leaves only 32,848 bytes, or
1.62%. A 10% slot margin would require a binary no larger than approximately
1,828,454 bytes, about 170 KiB smaller than the current build.

## Requirements before enabling OTA

1. Confirm the physical flash size is at least 4,194,304 bytes on the actual
   WROVER board.
2. Rebuild and measure the final release binary after all V1 changes.
3. Reduce firmware size enough to leave a practical update margin, or move to a
   hardware-verified larger-flash target and design a matching partition table.
4. Implement signed-image verification and authenticated update requests as
   specified in `WEB_SECURITY.md`.
5. Test interrupted download, invalid signature, boot failure and rollback on
   hardware before exposing any OTA endpoint.

The proposed CSV is an analysis artifact only. `platformio.ini` deliberately
continues to select `huge_app.csv`.
