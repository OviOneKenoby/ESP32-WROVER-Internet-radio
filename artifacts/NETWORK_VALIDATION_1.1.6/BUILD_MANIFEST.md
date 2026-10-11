# 1.1.6 network progress instrumentation build

- Firmware: 1.1.6-validation; embedded build_git_id **18553c4a207e** verified
  in binary bytes.
- Executable commit: `18553c4a207e97d0b3bde01539b7e8166d49abc4`.
- PR/main base: `112d68a49a053d302a87fc5658d497617eedda88`; PR #5 unmerged.
- Clean PlatformIO build PASS, 222.52 seconds after clean PASS.
- RAM 84,144/327,680 bytes; flash 2,015,225/3,145,728.
- Static RAM delta versus 1.1.5: **184 bytes**. Instrumentation also adds
  bounded execution/critical-section overhead; measure its own baseline.
- Binary 2,021,808 bytes; exact ELF 29,457,432 bytes supplied in zip.
- Host checks: 266 correctness, 2024 bounded writer, 10 failure-record storage:
  **2300 PASS**. Final ELF routing verification PASS (ELF_VERIFICATION.txt).
- PlatformIO Core 6.2.0, espressif32 7.0.1, framework
  3.20017.241212+sha.dcc1105b (Arduino 2.0.17/IDF 4.4.7 headers),
  Xtensa 8.4.0+2021r2-patch5. Existing dependency deprecation warnings retained.
- Pinned packages/libraries, qio_qspi SDK, partitions, GPIO, AAC patch and
  audio/BT lifecycle unchanged. Four diagnostic --wrap flags added; no
  framework package or driver buffer/configuration change.
- TX/ref/free/RX prototypes compiled against exact SDK headers. Final ELF
  proves Wi-Fi TX routing and wrapped callback pointers in wifi_start;
  each IRAM wrapper has exactly one call to its original. No logging/allocation
  added to wrappers, no diagnostic lock held across original calls.
- Hardware at publication NOT RUN. Subsequent owner run: PARTIAL/network FAIL
  at third refresh, TX conversion attributed; see OWNER_REPORT_2026-10-11.md.
  Diagnostic candidate, not a demonstrated fix.
- Publication/download verification PASS: bin, zip and extracted exact ELF
  were downloaded from GitHub and matched all published SHA-256 entries.
  Wrapper routing verification also passed on the downloaded ELF. Release
  tag resolves to the executable commit above; older assets were preserved.
- Prerelease: https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/tuner-1.1.6-network-progress-18553c4

See SHA256SUMS.txt and README.md for exact identity, extraction/decoding and
the one-minute controlled root/ping test. Private TX conversion successes and
radio completion remain outside instrumentation coverage. Incident OPEN.
No automatic flash/reconnect/reboot, gzip fix claim or merge.

Executable files were committed before the final clean build and unchanged
during it. Later commits only update documentation and publication evidence.
