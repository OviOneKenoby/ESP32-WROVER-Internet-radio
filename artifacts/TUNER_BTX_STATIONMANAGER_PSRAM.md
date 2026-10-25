# TUNER BTX StationManager PSRAM candidate

## Identity

- Branch: `tuner/btx-tls-psram-remediation`
- Installed baseline implementation: `71a7ee9c59f78a6629b7d27c5b986bde9b1e8898`
- Installed baseline BIN: 2,071,584 bytes; SHA-256
  `247f7a097c76b664072c6d8a38aaf96e7f967b6bd818924b357b3eaadb8c3103`
- Implementation commit: `dbf9896876a7fe8c9b421bb1608dc9c2782e9cfe`
- Environment: `esp32-lwip-psram`, `espressif32@7.0.1`, board `esp32dev`
- Framework: Arduino-ESP32 2.0.17 / ESP-IDF 4.4.7
- Toolchain: `toolchain-xtensa-esp32 8.4.0+2021r2-patch5`
- ESP32-A2DP: `3245602afc494f9e62160a0cfb2af864af45a37f`
- ESP8266Audio: `058e131b26e459b9aadcb589a50f07877f1a09fd`
- ArduinoJson: 7.4.3; GxEPD2: 1.6.9

## Design and lifetime

`RadioStation` remains a 324-byte trivially copyable and trivially
destructible record. `StationManager::begin()` requests exactly 30 records
(9,720 bytes) in one `MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT` allocation. Saved,
recent and favorite views are fixed offsets into that block. Every record is
placement-constructed before the pointer is published. The global constructor
only initializes the pointer and counters; `setup()` calls `begin()` before
the first persistence load or record access. Existing callers execute in the
main-loop-owned UI/web/RTCP paths, so this change introduces no new task,
callback or synchronization ownership.

The allocation remains valid for the manager lifetime. The destructor ends
all record lifetimes and releases the same block with `heap_caps_free`. The
production application does not normally tear down globals, but the paired
lifetime is deterministic in host tests.

Allocation failure has no internal fallback. The pointer remains null, all
record access/mutation fails closed, station and list loading is skipped, and
NVS is left untouched. The existing serial error channel reports the failure.

## Persistence compatibility

The existing NVS namespaces (`stations`, `recent`, `favorites`), `count` key,
indexed `n%d`/`u%d`/`c%d` keys, capacities, string sizes and codec values are
unchanged. A host Preferences model saved data through one manager lifetime
and reloaded the same counts, values, order, favorites and recent items in a
new lifetime. No migration or erase is required.

## Verification

- StationManager behavior: 62/62 PASS.
- StationManager source policy: 16/16 PASS.
- Core correctness/navigation: 266 PASS.
- Bounded web writer: 2,024 PASS.
- Diagnostics PSRAM allocator lifecycle: PASS.
- MP3 memory diagnostics: 13/13 PASS.
- BTX: 46/46 PASS.
- D-009: 23/23 PASS.
- RTCP diagnostics: 11/11 PASS.
- TLS record-buffer model: 10/10 PASS.
- CCM wire/protocol: 24/24 PASS.
- Relevant D-009, BTX, diagnostics, display-service, discovery, latency,
  Bluetooth-memory, MP3, TLS, Wi-Fi-probe and web policy suites: PASS.
- Clean `pio run -e esp32-lwip-psram`: PASS.
- No dedicated Radio Browser host test exists; browser source was unchanged
  and compiled in the clean production build.
- TLS and lwIP PSRAM wrappers and the diagnostics PSRAM allocator remain
  present in the linked ELF.

## Measured memory and artifacts

| Measurement | Installed baseline | Candidate | Delta |
| --- | ---: | ---: | ---: |
| `.dram0.data` | 30,088 | 30,088 | 0 |
| `.dram0.bss` | 58,704 | 48,984 | -9,720 |
| Total static internal DRAM | 88,792 | 79,072 | -9,720 |
| `stationManager` BSS symbol | 9,732 | 8 | -9,724 |
| BIN | 2,071,584 | 2,072,416 | +832 |
| ELF | 30,314,124 | 30,319,124 | +5,000 |
| 3 MiB slot headroom | 1,074,144 | 1,073,312 | -832 |

Candidate BIN SHA-256:
`6f02cdfd1576831a5d4a40f54eff88dcc3960fcdf72545d0909b0525cbceabf6`

Candidate ELF SHA-256:
`e547145b6977f7332decd1bafb33e1cd272de0087be3cfc3fec419e944ac5b65`

The linker proves static recovery only. Runtime INTERNAL/DMA free space and
largest contiguous blocks must be measured on hardware.

## Minimum hardware validation

1. Verify candidate, TUNER identity, partition and rollback; application-only
   flash at `0x10000`, then exact-span readback.
2. Confirm the PSRAM storage-ready boot message, retained saved stations,
   Favorites and Recent ordering, and zero allocation failures.
3. Exercise station add/remove through the web manager, favorite add/remove,
   recent-list update, fixed-list previous/next and wrap; reboot and confirm
   persistence without NVS migration or loss.
4. Capture idle and Bluetooth INTERNAL/DMA free/largest values, then transition
   Bluetooth to the verified Radio ABF HTTPS MP3 stream.
5. During stable playback, verify HTTP diagnostics, station CRUD and RTCP
   responsiveness with zero new allocation failures/TX refusals.
6. Stop playback and capture recovery. Compare real largest-block values with
   the installed `71a7ee9c59f7` evidence; do not infer them from static savings.

Hardware status: NOT INSTALLED / NOT HARDWARE VALIDATED.
