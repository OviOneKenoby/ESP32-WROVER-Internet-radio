# TUNER BTX TLS internal-RAM remediation

## Baseline

- Starting commit: `7af6aced423fab1d77a911aeeaa7e64e8922f431`
- Branch: `tuner/btx-tls-psram-remediation`
- PlatformIO environment: `esp32-lwip-psram`
- Platform: `espressif32@7.0.1`
- Framework package: Arduino-ESP32 `3.20017.241212+sha.dcc1105b`
  (Arduino core 2.0.17, ESP-IDF 4.4.7 libraries)
- Diagnostic rollback: 2,075,824 bytes,
  `621db1555f782c405bb1b31f8acebdbff4d584683d970220a4c8ef990c2714bc`
- Original BTX rollback: 2,072,960 bytes,
  `67fbeb56082fffddb6104f5ed948395bc862262cf5bcb10602292b08cb3ad590`

## Verified allocation

The matching diagnostic ELF resolves the failed call as:

`heap_caps_calloc -> esp_mbedtls_mem_calloc -> mbedtls_calloc ->
mbedtls_ssl_setup -> start_ssl_client -> WiFiClientSecure::connect ->
HTTPClient::connect -> HTTPClient::sendRequest`

The pinned SDK configuration selects `CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC=y`,
disables external/default/custom mbedTLS allocation, sets the maximum TLS
content length to 16,384 bytes, and does not enable asymmetric lengths.
`esp_mbedtls_mem_calloc()` is therefore already compiled to request
`MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT`.

Both `MBEDTLS_SSL_IN_BUFFER_LEN` and `MBEDTLS_SSL_OUT_BUFFER_LEN` resolve to
16,717 bytes: 16,384 content + 13 internal record header + 320 maximum payload
overhead. These are ordinary CPU-accessed byte buffers. They are not DMA or
ISR buffers and require only normal allocator alignment. The retained hardware
record confirms task context, ISR=false, and cache enabled. Their lifetime is
the owning SSL context; the framework frees them with `heap_caps_free`, which
accepts internal and PSRAM pointers.

## Alternatives

### A. Framework `CONFIG_MBEDTLS_EXTERNAL_MEM_ALLOC`

Supported by ESP-IDF source, but not selectable by an application-only macro:
the Arduino framework mbedTLS library and `esp_mbedtls_mem_calloc` are
precompiled. A framework rebuild would be required. The resolved implementation
moves all mbedTLS allocations to PSRAM without internal fallback. This is
broader than the demonstrated defect and increases compatibility and physical
memory-confidentiality risk. Rejected.

### B. Exact record-buffer allocator wrapper

Selected. GNU ld redirects the precompiled allocator reference to an
application wrapper. Compile-time assertions bind the policy to the resolved
input/output sizes. Only exact 16,717-byte requests prefer
`MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT`; allocation failure falls back to the
original internal allocator. This should save 33,434 bytes of contiguous
internal allocation per SSL context while retaining all smaller crypto objects
internally.

PSRAM is cache-backed and unsuitable while cache is disabled, but the SSL
record path executes in task context with cache enabled and performs no DMA.
The wrapper is stateless and uses ESP-IDF's thread-safe heap allocator. Network
TLS semantics and cryptography do not change. Record plaintext can reside in
external RAM, so physical-memory confidentiality is weaker; this is the same
class of tradeoff documented by Espressif for external mbedTLS allocation.

### C. Smaller TLS record buffers

Requires rebuilding the precompiled mbedTLS library. Peers may legally send
larger records unless maximum-fragment-length behavior is negotiated and
honored, so a local reduction can cause HTTPS interoperability failures.
Rejected for this isolated remediation.

### D. Reduce unrelated internal allocations

Helix already places its three largest structures in PSRAM. Releasing retained
Classic Bluetooth memory conflicts with restartable A2DP, and rearranging web,
audio, or RTCP allocations does not guarantee one 16,717-byte contiguous
block. Rejected as broader and less deterministic.

## Source change

- `platformio.ini`: link-time wrapper flag.
- `src/tls_record_buffer_psram.h`: checked exact-size selection.
- `src/tls_record_buffer_psram.cpp`: PSRAM-first record allocation and original
  allocator fallback.
- `tests/test_tls_record_buffer_psram.cpp`: deterministic size/overflow tests.
- `tests/test_tls_record_buffer_psram_policy.py`: source and security-policy
  checks.
- `tools/verify_tls_record_buffer_wrapper_elf.py`: confirms the linked mbedTLS
  allocator pointer targets the wrapper and retains the original fallback.

## Verification

Focused checks:

- TLS allocation selection: 10/10 PASS
- TLS source policy: 10/10 PASS
- BTX integration policy: 21/21 PASS
- Classic Bluetooth memory policy: 10/10 PASS
- D-009 integration policy: 12/12 PASS
- MP3 instrumentation policy: 21/21 PASS

The two new TLS tests were also rerun under C++11 after correcting a
target-language-only `constexpr` incompatibility; both remained 10/10 PASS.
The clean `esp32-lwip-psram` production build passed at firmware source commit
`cbcd90cb23a1db428fc8654beef5e28288680cbc` with embedded build ID
`cbcd90cb23a1`.

The production application is 2,075,920 bytes, SHA-256
`7e25aaf27e6c78ad75eb563bc58f17d798d9a691f7284cb88da8b99d8b3b0976`.
The ELF is 30,379,460 bytes, SHA-256
`aa1e8f0f2104e922446606317abc78c8c90e061e061487d44bf9011f5e654264`.
The 3 MiB application partition retains 1,069,808 bytes. Linked static RAM is
90,264 bytes, unchanged from the diagnostic build. The application grows by
96 bytes relative to the installed diagnostic image.

Post-link inspection passed: `mbedtls_calloc_func` points to
`__wrap_esp_mbedtls_mem_calloc`, and the original
`esp_mbedtls_mem_calloc` remains linked for fallback. The build used the
expected pinned platform, framework, and library revisions. The candidate and
ELF are preserved privately; neither is published by this report.

## Original hardware validation gate (completed)

The software-only checkpoint did not authorize a flash. A later, separately
authorized application-only test preserved rollback images, verified exact
readback, then tested cold-boot Radio Browser HTTPS, Bluetooth A2DP,
direct Bluetooth-to-Radio Browse Countries, HTTPS and HTTP MP3, web/RTCP
coexistence, allocation-failure counters, and repeatable Bluetooth restart.
The resulting evidence is recorded below.

If that test fails and rollback is separately authorized, restore only the
verified 2,072,960-byte BTX application at `0x10000`, then read back exactly
2,072,960 bytes and require SHA-256
`67fbeb56082fffddb6104f5ed948395bc862262cf5bcb10602292b08cb3ad590`.
Bootloader, partition table, NVS, pairing, credentials, and every other flash
region must remain untouched.

## Hardware checkpoint

The candidate was installed application-only and independently read back as
2,075,920 bytes with the expected SHA-256. Build ID `cbcd90cb23a1` booted
normally with Wi-Fi, PSRAM, web diagnostics, and the RTCP listener available.

Cold-boot Radio Browser retrieval, A2DP audio and metadata, direct
Bluetooth-to-Radio MP3 transition, and post-Bluetooth Radio Browser retrieval
all passed. During active Bluetooth the largest internal block was 6,388
bytes; immediately before the post-Bluetooth HTTPS retrieval it was 7,924
bytes, and the subsequent MP3 pre-construction checkpoint recorded 12,788
bytes. The 16,717-byte internal fallback could not fit under these observed
limits. The HTTPS request completed and both allocation-failure counters
remained zero. Together with the verified ELF allocator binding, this verifies
the TLS record-buffer PSRAM path using the existing bounded instrumentation.

At the latest stable MP3 checkpoint, internal heap was 14,888 bytes free with
a 3,316-byte largest block, DMA heap was 16,576 bytes free with an 8,180-byte
largest block, and PSRAM free was 4,144,135 bytes. All eight Helix allocations
succeeded, Wi-Fi recorded no disconnect, web diagnostics returned HTTP 200,
the RTCP listener accepted a bounded connection, and no unexpected reset was
observed.

Bluetooth restartability subsequently passed with A2DP audio and metadata
restored. West City Radio and 80s80s provided stable owner-observed HTTP MP3
playback; web diagnostics returned HTTP 200 and the RTCP listener remained
reachable during West City playback.

The saved Sports Radio Brila FM HTTPS MP3 attempt stopped immediately. The
immediate snapshot retained zero allocation failures and zero internal-cap
failures, completed generator, Helix, and I2S initialization, but no stable
playback phase. Wi-Fi and RTCP remained healthy and no reset occurred. This is
not evidence of the prior TLS record-buffer defect. Its precise cause is
unproven because the exact stream error was not retained; endpoint availability,
content/codec mismatch, or a later source/decoder failure remain possible.

The owner then independently verified the exact Radio ABF endpoint as HTTP
200 and `audio/mpeg` from Windows. Because the firmware exposes no transient
direct-URL path and Radio Browser resolves Radio ABF to a different AAC URL,
explicit owner authorization allowed one temporary saved station containing
the exact HTTPS MP3 URL. It was selected physically and produced clear, stable
audio. The retained session reached stable playback with all eight Helix
allocations successful, zero allocation/internal-cap failures, Wi-Fi and RTCP
healthy, web HTTP 200, and no reset. The temporary station was removed and the
saved-station/Favorites JSON matched the pre-test state.

Resource headroom remains a blocker: during stable Radio ABF playback the
snapshot recorded 5,524 internal bytes free with a 1,652-byte largest block,
and only 2,132 DMA-capable bytes free with a 236-byte largest block. PSRAM had
4,105,271 bytes free. The stable-playback phase itself retained only a
1,332-byte largest internal/DMA block. The stop condition was applied after
cleanup.

TLS PSRAM placement, Radio Browser HTTPS, HTTP MP3, and HTTPS MP3 are hardware
PASS. Overall status remains PARTIAL PASS due to dangerously low contiguous
internal/DMA headroom; REMOTE BTX installation is not approved by this
checkpoint.
