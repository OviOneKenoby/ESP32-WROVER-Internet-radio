# Root-refresh DMA allocation investigation

Evidence priority: the dated 1.1.4 owner report, not the previous JSON theory.
**Incident OPEN.** This records findings from 1.1.4. Subsequent 1.1.5 evidence
demonstrates wifi-task/ESF allocator attribution; persistence remains unresolved.
See [exact ELF decoding and failure paths](../NETWORK_VALIDATION_1.1.5/OWNER_REPORT_2026-10-11.md).

## Exact mask and compiled evidence

The pinned framework package is Arduino-ESP32 2.0.17,
`3.20017.241212+sha.dcc1105b`, with SDK headers identifying ESP-IDF 4.4.7.
Its `esp_heap_caps.h` defines 8BIT = 1<<2, DMA = 1<<3, INTERNAL = 1<<11.
Thus **0x80c = INTERNAL | DMA | 8BIT**, not DEFAULT or SPIRAM. External PSRAM
cannot satisfy this request on classic ESP32. An internal/8BIT largest-block
snapshot is not an allocation-time guarantee for the stricter DMA mask.

Disassembly of the actual pinned `libesp_wifi.a` and executable 1.1.4 ELF
shows `malloc_internal_wrapper` calling `heap_caps_malloc` with a literal
0x80c. In that ELF, the wrapper is at 0x40083f98; it loads its caps from
0x4008082c, whose little-endian bytes are `0c 08 00 00`, then calls
`heap_caps_malloc` at 0x40084230. These addresses apply only to e135a1c0aace.
`wifi_malloc` at 0x40083f50 instead calls ordinary malloc in this package.

The effective 32-bit `wifi_osi_funcs_t` layout was compiled with the pinned
Xtensa compiler and SDK header: `_malloc_internal` offset = 0x15c,
`_wifi_malloc` = 0x16c. Pinned `libpp.a` disassembly of
`esf_buf_alloc_dynamic` shows an indirect allocator call using 0x15c for one
class of buffer and 0x16c for another. The internal branch computes requested
size from packet length plus descriptor/headroom (`96 + headroom + length`),
not a literal constant 1622. This is a concrete driver allocation path that
can produce the observed mask, **not proof that hardware took that path**.
Do not assign the packet type, RX/TX direction or exact 1622-byte composition
without the captured caller frames. Other DMA users are not excluded by mask.

Archive SHA-256:

- libesp_wifi.a: `BB858D1FE6F7395E919806318FD5A6E80A262C40D6F448CC4D1B7CB673976C75`
- libpp.a: `A4356534D066DA8C1CE08B52E3E614EE4DA1A3E9B3B4BFC4285D083F46B75937`

Upstream source context was inspected in ESP-IDF v4.4.7 commit
`38eeba213aa695aabfd6d89aa9f5078dbe5a94c3`, with esp-lwip submodule
`a45be9e438f6cf9c54ec150581819c3b95d5af6b`. These checkouts are analysis-only,
not replacement build dependencies. `esp_adapter.c` confirms the wrapper's
mask and OS adapter assignment; the exact binary checks above avoid assuming
that a matching version tag alone proves identical prebuilt libraries.

## Root, TCP and Wi-Fi ownership

- The root has no JsonDocument. Its 3617-byte HTML resides in flash. The
  application builds a small dynamic header, then sends header and full body
  in two calls. The observed built free=9840 from start=9968 is a 128-byte
  delta, not a page-sized contiguous allocation.
- `lwip_send` uses NETCONN_COPY. `tcp_write` allocates pbufs/segments, copies
  the body and queues them on `unsent`; output advances them to `unacked`.
  They remain owned by TCP until ACK processing, purge or timeout. Success
  from the application's send means the stack accepted bytes; low-level
  packet output can encounter memory errors later and TCP can retry them.
- Effective TCP_MSS=1436, TCP_SND_BUF/TCP_WND=5760,
  TCP_OVERSIZE=MSS and default malloc's internal-preference threshold=4096.
  `MEM_LIBC_MALLOC=1`/`MEMP_MEM_MALLOC=1`; ordinary small TCP/pbuf allocations
  therefore prefer internal RAM. CONFIG_WIFI_LWIP_ALLOCATION_FROM_SPIRAM_FIRST
  is not enabled. Their normal malloc caps do not directly explain the
  explicitly requested DMA mask; they can still compete for the same RAM.
- WebServer drops its client after the request. The shared WiFiClient socket
  handle closes the fd when destroyed and its RX object frees the lazy
  1436-byte read buffer. Header Strings are destroyed. Closing a socket is
  not a guarantee that TCP's transmitted/unacknowledged queues are already
  purged; graceful-close TCP state can retain them awaiting ACK/FIN/timers.
- `wlanif` passes output to esp_netif, then Wi-Fi's transmit-by-reference
  path in this PSRAM build. The driver can retain a pbuf reference until its
  own completion. RX uses PBUF_REF with the L2 buffer attached (L2_TO_L3_COPY
  is not enabled); freeing the pbuf notifies Wi-Fi to free/recycle RX storage.
  Static RX/TX buffers persist for interface lifetime; dynamic buffers have
  packet/refcount/completion lifetime. No driver-private leak is demonstrated.
- Ending the root sample after client cleanup still includes concurrent Wi-Fi,
  BT and TCP/IP task activity. The 4712-byte built-to-end delta is consistent
  with outstanding network allocations, but cannot uniquely identify owners
  or prove a leak. The wait-only snapshot can show whether memory returns,
  failure counters continue increasing and ping stays absent independently.

## Attribution instrumentation (next candidate)

The allocator callback's `function_name` identifies an allocator entry point
(such as heap_caps_malloc), not its original consumer. 1.1.5 retains that name,
current core/task name/handle, tick and up to six normalized caller PCs using
the pinned non-printing backtrace helpers. First four records plus latest are
retained in fixed internal storage; omitted middle records are counted.
Names are copied immediately instead of borrowing potentially freed task data.

Safety: callback/helpers are in IRAM; records are in DRAM. No allocations,
heap queries, logging, network calls or task enumeration occur in the callback.
Task getter/name implementations were inspected: direct current-TCB access,
no allocation or scheduler list walk. ISR/cache-off paths skip task-name and
stack capture; cache-off paths do not dereference allocator text. Stack walks
are limited to six frames, validate the current SP/PC and stop on invalid/end
frames. A short port critical section protects record copying. Formatting
and addr2line decoding happen later outside the callback. DMA-specific heap
samples are taken by the main task, never by the allocator callback.

Some frames may be absent because of ISR/cache-off context, corruption or
optimized/tail-call frames. Task name denotes the execution context; only the
stack can distinguish a TCP/IP task transmitting on behalf of HTTP from an
independent Wi-Fi receive allocation. New static diagnostic storage itself
uses internal RAM; record the candidate's baseline and RAM delta. No
artificial OOM probe is run on the tuner.

## Proposed correction and boundary of evidence

The justified direction is to reduce **outstanding root/TCP memory competing
with DMA buffers**, rather than remove JSON copies or hide failure with a
reconnect/reboot. A narrowly scoped mitigation is to precompress this exact
HTML at build time and serve a flash-resident gzip representation when the
client advertises gzip support, with an uncompressed fallback and correct
Content-Encoding/Vary/Content-Length. Offline deterministic gzip (mtime=0)
is **1542 bytes versus 3617**, with exact decompression roundtrip verified.
It removes 2075 payload bytes from each supported root response and can reduce
TCP segmentation/queue pressure without runtime compression allocations or
UI changes. Actual peak RAM savings and incident prevention require testing;
TCP oversizing, headers, segmentation and driver queues prevent treating the
payload difference as an exact heap saving.

This mitigation is **proposed, not applied to the attribution candidate**, so
the next reproduction can identify the original allocation consumer. If
frames confirm a different subsystem, revise the correction accordingly.
Global allocator policy changes, arbitrary buffer resizing and internal DMA
reservation are not justified yet. No automatic reconnect/reboot is added.

The final 1.1.4 wait-only snapshot is now saved: ping remained unavailable
27 min 10.312 s after the post-refresh snapshot, despite internal free/largest
recovering from 4940/1972 to 8232/3188 and failure count staying 5. HTTP count
and Wi-Fi events remained unchanged; observation is concluded. This makes
transient pressure alone insufficient to explain recovery behavior, but does
not identify retained packet ownership or a driver-state fault. DMA-specific
availability was not sampled in 1.1.4. The proposed gzip change remains an
unproven prevention measure, not a recovery fix. A new firmware cannot add
caller attribution retroactively to that boot. On a later run, retain `x` and
decode `[ALLOC_FAILURE]` PCs with the **matching published ELF**. Do not repeat
an uninstrumented cycle or mistake five observed failures for a proven root
cause. PR #5 remains open/unmerged; hardware incident remains OPEN.
