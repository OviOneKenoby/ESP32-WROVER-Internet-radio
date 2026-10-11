# 1.1.7 TX refusal evidence build

- Firmware 1.1.7-validation; embedded build_git_id **9cf48e9022c0** verified
  in binary bytes. Executable commit `9cf48e9022c0a9e8b88943c073bb29f447bfa80a`.
- PR/main base `112d68a49a053d302a87fc5658d497617eedda88`; PR #5 unmerged.
- Clean PASS 1.16s; final PlatformIO build PASS 232.00s.
- RAM 84,440/327,680 bytes; flash 2,017,317/3,145,728.
  Static RAM increase versus 1.1.6: **296 bytes**. Additional bounded critical
  sections/header inspection add timing overhead; use this build's baseline.
- Bin 2,023,888 bytes; exact ELF 29,484,404 bytes; ELF zip 11,027,424 bytes.
  SHA256SUMS.txt gives identities of all three, including extracted ELF.
- Host checks PASS: 266 correctness +2024 bounded writer +10 failure storage
  +67 refusal evidence = **2367**. Tests cover truncated/fragmented headers,
  ledger duplicates/overflow/unmatched frees and bounded record retention.
- Final ELF verification PASS: all five wrappers IRAM and call originals once,
  Wi-Fi TX path and ref/free/RX registration routed through wrappers,
  wifiLowLevelInit routed through unchanged-config init wrapper.
- PlatformIO Core 6.2.0, espressif32 7.0.1, framework
  3.20017.241212+sha.dcc1105b (Arduino2.0.17/IDF4.4.7 headers),
  Xtensa8.4.0+2021r2-patch5; existing dependency warnings retained.
- Pinned dependencies, qio_qspi SDK, partitions/GPIO/audio lifecycle/AAC patch
  unchanged. New fifth linker wrap captures esp_wifi_init scalar fields;
  no framework or closed-driver patch, TX sizing/policy/retry modification.
  Effective initialization remains dynamic TX32/cache TX4/static TX0.
- Diagnostic wrappers use fixed DRAM, bounded operations, cache-off guards,
  no diagnostic allocation/logging and no lock across original calls. All
  added inline parser/ledger helpers resolve inside IRAM wrappers; the scalar
  allocation-counter getter is IRAM. ROM memcpy/memset remain compiler helpers.
- The event ledger is not an atomic private pool inventory. Callback ordering,
  overflow/unmatched-free flags and handle reuse limit inference. Global
  allocation-counter deltas include concurrent tasks.
- Optional JTAG script generated from exact final ELF; structural pattern
  verification and full branch/ABI disassembly review PASS; see BRANCH_REVIEW.md.
  JTAG execution NOT RUN. Requires compatible manual hardware/SMP setup; halts
  perturb timing. Firmware itself does not capture private branch PCs/cache
  descriptor counts. No unsafe fallback private patch/list walk.
- Hardware NOT RUN. Saved 1.1.6 captures are the existing evidence, not a
  validation of this build. Incident OPEN; no fix/leak/private queue proof.
- Publication/download verification pending; recorded after release upload.
- Prerelease: https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/tuner-1.1.7-tx-refusal-9cf48e9

See README.md for the short new test. No automatic flash/reconnect/reboot,
cache resize, gzip fix claim or merge. Executable files were committed before
the final clean build and unchanged during it; later commits document artifacts.
