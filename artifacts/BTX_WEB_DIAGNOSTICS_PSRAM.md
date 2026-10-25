# TUNER BTX web diagnostics PSRAM candidate

## Identity

- Source baseline: `0e30003fcbd6284d74eb1184aac8390b70477232`
- Implementation commit: `71a7ee9c59f78a6629b7d27c5b986bde9b1e8898`
- Environment: `esp32-lwip-psram`
- Platform: `espressif32@7.0.1`
- Framework: Arduino-ESP32 2.0.17 / ESP-IDF 4.4.7
- ArduinoJson: 7.4.3
- Application: 2,071,584 bytes
- Application SHA-256:
  `247f7a097c76b664072c6d8a38aaf96e7f967b6bd818924b357b3eaadb8c3103`
- ELF: 30,314,124 bytes
- ELF SHA-256:
  `8c73414f47354dde846e6c3e5ecd47baa12735f32f56741425fc0980619d5b2c`
- Application partition: `0x300000`; headroom: 1,074,144 bytes
- Hardware status: not installed; validation pending

The installed production rollback remains the 2,071,424-byte application from
`0e30003fcbd6284d74eb1184aac8390b70477232`, SHA-256
`2970cc09d62431e5385bcdb74bbba3702398a775c9e4f652ed3092a67e54f083`.
No rollback artifact was modified.

## Implemented change

`src/diagnostics_psram_allocator.h` implements the ArduinoJson 7.4.3
`Allocator` interface with `heap_caps_malloc` and `heap_caps_realloc` using
exactly `MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT`, plus `heap_caps_free`.
`WebPortal::handleDiagnostics()` is the only consumer. Its static allocator
outlives each local document. No INTERNAL fallback exists. A failed allocation
sets ArduinoJson overflow and follows the endpoint's existing bounded HTTP 503
path.

No JSON key, type, hierarchy or counter meaning changed. `sendJson()` still
measures the complete document, sets a fixed Content-Length and serializes
through the 512-byte bounded stack writer. `/api/status`, `/api/stations`,
station/configuration handlers, authentication, RTCP, TLS and audio behavior
are untouched.

## Offline verification

- Diagnostics allocator lifecycle and failure behavior: PASS.
- Web diagnostics allocation/schema source policy: 8/8 PASS.
- Bounded web writer: 2,024 checks PASS.
- Allocation failure records: 10 checks PASS.
- MP3 memory diagnostics: 13/13 PASS.
- Bluetooth Classic memory policy: 10/10 PASS.
- BTX integration policy: 21/21 PASS; BTX model: 46/46 PASS.
- D-009 model: 23/23 PASS.
- RTCP diagnostics: 11 checks PASS; RTCP policy: 14 checks PASS.
- TLS PSRAM policy: 10/10 PASS; TLS allocation model: 10/10 PASS.
- Production Wi-Fi probe-removal policy: 9/9 PASS.
- Clean production build: PASS.

Post-link verification retained the mbedTLS record-buffer PSRAM wrapper, lwIP
PSRAM wrapper and generic allocation-failure callback. No `SoftwareTxProbe`
symbol is present. Dependency pins did not change.

Linked static DRAM is 88,792 bytes (30,088 data + 58,704 BSS), identical to
the installed baseline. The allocator object is a 4-byte BSS symbol, absorbed
by existing alignment. The BIN grew 160 bytes and the ELF grew 2,776 bytes.
Prior request measurements support an estimated 1.0-1.6 KiB transient INTERNAL
saving during diagnostics construction, paid as transient PSRAM consumption.
The runtime largest DMA-block change is unknown until hardware validation.

## Web portal memory audit

| Component | Current storage and lifetime | Safe PSRAM opportunity | Decision |
|---|---|---|---|
| Root HTML/CSS/JS (`src/web_portal.cpp`, `WEB_PAGE`, `handleRoot`) | 3,618-byte `PROGMEM` symbol in flash/DROM; served with `send_P`; no application body copy | None; copying it would waste PSRAM and internal CPU time | Keep in flash |
| Images/icons | No separate binary assets are registered | None | No change |
| Portal object (`src/web_portal.h`, `WebPortal`) | 612-byte global BSS including server, DNS server, fixed SSID/password and counters; retained for process lifetime | Framework ownership makes relocation invasive | Keep internal/static |
| Diagnostics JSON (`handleDiagnostics`) | Previously default heap, request lifetime, multiple 1,024-byte ArduinoJson pools and copied strings | Dedicated PSRAM allocator is safe; CPU serialization only, not DMA | Implemented |
| Status JSON (`handleStatus`) | Default ArduinoJson allocator, request lifetime; at least one pool when populated | Dedicated allocator is technically feasible, but savings are estimated near 1 KiB and this frequently polled endpoint needs measurement | Future low/medium priority |
| Stations JSON (`handleStations`) | Default allocator; request lifetime; station/favorite names, URLs and codec strings may require multiple pools | Dedicated allocator is technically feasible and likely the largest remaining web JSON opportunity | Future highest web priority; measure first |
| JSON response writer (`sendJson`, `BufferedWebWriter`) | Fixed 512-byte local array on the main-loop task stack; no full response buffer and no heap growth | External stack buffer would add complexity and copies for little value | Keep stack/internal |
| Static response/header path | Pinned WebServer constructs request-lifetime header `String`s; static body remains in flash | Requires framework-level changes; exact peak not measured | No-go without measurements |
| Error/success bodies (`sendJsonError`, mutation handlers) | Small request-lifetime `String` or literal responses, generally tens of bytes | Savings negligible; error reliability favors internal memory | Keep internal |
| URI and form arguments | Request-lifetime Arduino `String`s in delete, Wi-Fi, timezone and station handlers; the framework also owns request/header parsing and `RequestArgument[]` | Selective relocation would require a WebServer fork or custom parser and may create copies | No-go without measured benefit |
| Route handlers | Registered during startup and retained; bounded handler/lambda ownership in WebServer | Moving them requires framework changes | Keep internal |
| Station model (`src/stations.*`) | About 9,732 bytes of fixed global state for stations, recent items and favorites; shared by UI, NVS and RTCP, not a web temporary | Relocation could affect all consumers and offers no request-specific recovery | Out of scope; keep static |
| Radio Browser arrays (`src/browser.cpp`) | Lazy request/flow allocations: countries about 10,750 bytes, tags about 1,280 bytes, results about 6,480 bytes; released by `freeBrowseMemory()` | Allocations above the 4,096-byte preference threshold are expected to prefer PSRAM; tags likely remain internal. Actual placement was not remeasured | Preserve; separate subsystem |
| Radio Browser response and JSON | 24,576-byte `StreamString` reserve plus default ArduinoJson filter/result documents; HTTPS client objects persist through the browse flow to reduce fragmentation | JSON documents are candidates for dedicated PSRAM; stream is already expected to prefer PSRAM under the pinned policy | Future medium priority; separate TLS/browser validation required |

The WebServer is serviced synchronously, so application handlers do not
overlap one another. Their allocations can still overlap the independent audio
task, network driver and TLS activity. Network, Wi-Fi and I2S DMA buffers must
remain internal/DMA-capable. Flash-resident assets, the bounded writer, tiny
error paths and framework-owned parser/header state should not be relocated by
default.

### Ranked future opportunities (not implemented)

1. Measure and, if justified, give `/api/stations` a dedicated PSRAM JSON
   allocator. Expected benefit: several request-time KiB; risk: low/medium.
2. Measure Radio Browser filter/result documents and consider scoped PSRAM
   allocators. Expected benefit: potentially several pool KiB; risk: medium
   because the path overlaps TLS and browse-state lifetimes.
3. Measure `/api/status`; consider a dedicated allocator only if its frequent
   request-time allocation materially affects DMA headroom. Expected benefit:
   roughly one pool; risk: low but PSRAM latency becomes frequent.
4. Leave tiny error/literal/URI `String`s unchanged unless measurements prove a
   problem. Expected benefit: tens to low hundreds of bytes; poor tradeoff.
5. Do not fork WebServer parsing/header machinery without direct peak-allocation
   evidence. Risk and compatibility cost exceed the unmeasured benefit.

## Bounded hardware validation plan

1. Verify this candidate and all rollback hashes; identify the TUNER and its
   partition layout.
2. Install only the application at `0x10000`, then read back exactly 2,071,584
   bytes and require the candidate SHA-256.
3. Confirm normal boot and retained configuration; record baseline allocation
   failures and Wi-Fi TX-refusal counters.
4. Briefly verify Bluetooth A2DP plus metadata, then transition to radio.
5. Play the known HTTPS MP3 endpoint
   `https://stream.radioabf.com/abf-sd.mp3`.
6. During active audio, request `/api/diagnostics` and record INTERNAL/DMA free
   and largest-block values plus PSRAM delta.
7. Observe five bounded minutes of audio, HTTP diagnostics and RTCP
   coexistence.
8. Require continuous audio and network responsiveness with no new generic
   allocation failure, Wi-Fi TX-refusal growth, disconnect or reset.
9. Stop playback and verify memory recovery. Preserve/restore any temporary
   station and Favorite state exactly.

Stop on any candidate/readback mismatch, unexpected reset, allocation failure,
TX-refusal growth, audio failure, or HTTP/RTCP loss. Offline verification does
not establish hardware success.

## Remaining risks

- Runtime PSRAM use and INTERNAL/DMA largest-block recovery are estimates until
  measured on hardware.
- The existing 503 error construction still uses a small internal `String`;
  this is bounded and intentionally preserves established behavior.
- Other default ArduinoJson documents and framework parsing allocations remain
  internal and can still overlap audio/network activity.
- This change targets one confirmed request-time contributor; it does not prove
  that every Wi-Fi DMA starvation condition is eliminated.
