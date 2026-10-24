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

Focused pre-build checks:

- TLS allocation selection: 10/10 PASS
- TLS source policy: 10/10 PASS
- BTX integration policy: 21/21 PASS
- Classic Bluetooth memory policy: 10/10 PASS
- D-009 integration policy: 12/12 PASS
- MP3 instrumentation policy: 21/21 PASS

Production build, ELF verification, size/hash/RAM budget and hardware proof are
pending.

## Hardware validation gate

No flash is authorized. A later application-only test must preserve rollback
images, verify exact readback, then test cold-boot Radio Browser HTTPS,
Bluetooth A2DP, direct Bluetooth-to-Radio Browse Countries, HTTPS and HTTP MP3,
web/RTCP coexistence, allocation-failure counters, and repeatable Bluetooth
restart. It must confirm that TLS no longer requests unavailable internal
record buffers and that no reset, audio regression, or network-security change
occurs.
