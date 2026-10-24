# TUNER BTX Wi-Fi DMA production candidate

## Identity

- Source commit: `0e30003fcbd6284d74eb1184aac8390b70477232`
- Environment: `esp32-lwip-psram`
- Platform: `espressif32@7.0.1`
- Framework: Arduino-ESP32 2.0.17 / ESP-IDF 4.4.7
- Application size: 2,071,424 bytes
- Application SHA-256:
  `2970cc09d62431e5385bcdb74bbba3702398a775c9e4f652ed3092a67e54f083`
- ELF size: 30,311,348 bytes
- ELF SHA-256:
  `0525e2b41d383b3eae2a3d73d153de0d66a8f2b0f09363a44c7b14fbf47f6f2e`
- Application partition: `0x300000`; headroom: 1,074,304 bytes
- Hardware status: not installed; validation pending

The private candidate is preserved outside the repository. The previously
installed 2,075,920-byte TLS-remediation application with SHA-256
`7e25aaf27e6c78ad75eb563bc58f17d798d9a691f7284cb88da8b99d8b3b0976`
and all earlier rollback images remain unchanged.

## Correction

The production environment no longer inherits `TUNER_UART_TX_PROBE`. Wi-Fi
initialization therefore uses the original SDK OS-adapter table without the
probe's `_malloc_internal`, `_wifi_malloc`, or mutex-unlock interception. The
diagnostic source remains compile-time isolated in the explicit
`esp32-uart-probe` environment.

The normal network wrappers, TLS record-buffer PSRAM allocator, lwIP PSRAM
policy, Bluetooth restartability, audio configuration, RTCP/BTX behavior,
I2S buffers, audio task stack, Helix allocation policy, and global allocator
policy are unchanged.

## Offline verification

- Python source/policy suites: 11/11 PASS.
- Native C++ model suites: 12/12 PASS.
- Clean production build: PASS.
- Linked static DRAM: 88,792 bytes versus 90,264-byte baseline, -1,472 bytes.
- Application binary: -4,496 bytes versus the installed TLS candidate.
- `SoftwareTxProbe` symbols in the final ELF: zero.
- Generic `heap_caps_register_failed_alloc_callback` path: retained.
- TLS allocator ELF binding: PASS; wrapper active and original allocator
  retained for fallback.
- lwIP PSRAM wrapper: retained.

The measured static reduction predicts up to 1,472 additional bytes in the
shared internal/DMA-capable heap after static initialization. Runtime free
heap and largest contiguous block remain hardware measurements and are not
claimed by this offline result.

## Diagnostic coverage change

The serial `x` snapshot continues to expose generic bounded allocation-failure
counts and records, including requested size, capability mask, task, timestamp,
and bounded PCs. It no longer emits `SW_TX`, `SW_CONVERT`, `SW_MALLOC`, or
`SW_TRACE`, so it cannot directly classify a failed Wi-Fi allocation as
dynamic conversion followed by driver requeue. A zero generic failure-count
delta is valid evidence of no observed heap-capability allocation failures; the
absence of probe-specific fields alone is not.

## Bounded hardware-validation plan

After separate owner authorization:

1. Verify TUNER identity, candidate/rollback hashes and partition boundaries;
   flash only the application at `0x10000`, then require an exact-length
   readback hash match.
2. Verify build ID, retained configuration, Wi-Fi, web manager, RTCP/BTX and
   PSRAM availability. Capture one idle `x` baseline.
3. Exercise restartable Bluetooth A2DP once, then switch directly to Radio.
4. Play the already-validated Radio ABF HTTPS MP3 endpoint for a bounded five
   minutes while checking audio, HTTP and RTCP coexistence. Capture `x` once
   during stable playback and once after stopping.
5. Compare generic allocation-failure count/records and internal/DMA free and
   largest-block values. Require no new allocation failure, no reset, stable
   audio, responsive HTTP/RTCP, and normal teardown recovery.
6. Run one bounded MP3, AAC/AAC+ and Bluetooth restartability smoke check to
   cover the unchanged audio paths before accepting the candidate.

If a generic failure is recorded, preserve its size, capabilities, task,
timestamp and PCs and stop; the production build cannot supply the removed
probe's dynamic/requeue classification without a separate diagnostic build.
