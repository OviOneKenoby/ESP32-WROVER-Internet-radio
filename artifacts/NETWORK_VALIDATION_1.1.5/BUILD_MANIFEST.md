# 1.1.5 allocation attribution build

- Executable commit: `f33ef76cfc2906d8f2e947d8c8e1e894a66c6de9`
- Firmware: `1.1.5-validation`; build_git_id: `f33ef76cfc29`.
- Clean PlatformIO build: PASS, 239.66 seconds; RAM 83,960/327,680;
  linked flash 2,014,157/3,145,728 bytes.
- Binary: `firmware-1.1.5-validation-f33ef76cfc29.bin`, 2,020,736 bytes.
- Host checks: existing 266, writer 2,024, allocation records 9: all PASS.
- PlatformIO 6.2.0, espressif32 7.0.1, Arduino framework
  `3.20017.241212+sha.dcc1105b`, IDF headers 4.4.7,
  Xtensa `8.4.0+2021r2-patch5`. Dependencies/configuration unchanged.
- Hardware NOT RUN; incident OPEN. Attribution instrumentation is not a fix.
- Matching ELF is supplied compressed; extract before decoding caller PCs.
- Prerelease: https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/tuner-1.1.5-alloc-attribution-f33ef76
- PR #5 remains open and unmerged. No hardware flash or automatic recovery.

See SHA256SUMS.txt for binary, ELF and archive hashes. Later documentation
commits do not change this executable. Keep the current failed device unchanged
until the wait-only snapshot is saved.
