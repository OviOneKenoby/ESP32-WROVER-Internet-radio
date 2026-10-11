# 1.1.6: TX NO_MEM persists after DMA heap recovery

**Incident OPEN; network FAIL reproduced. TX conversion consumer demonstrated;
later rejection branch, pool occupancy and persistence mechanism unproven.**

Raw supplied files are preserved as OWNER_REFRESH_RAW.txt and
OWNER_WAIT_SNAPSHOTS_RAW.txt. Owner reports failure on the third refresh, not
the first HTTP request. Browser generated seven recorded responses:
root, other (handler not found), stations, root, status, stations, root.
The first root already records four allocation failures, without an immediate
reported persistent outage. Do not equate an allocation failure with inevitable
permanent loss, or three refreshes with three HTTP responses.

## Baseline and progression

Baseline uptime 370622, version 1.1.6-validation/build 18553c4a207e: supplied
Bluetooth baseline reports SBC playing/BT active, no HTTP
responses/failures; INTERNAL 11312/5876, DMA 10280/4596. Wi-Fi connected,
BSSID E8:48:B8:66:61:DF channel 7; processed events 3, no disconnect/lost-IP.
Baseline tx calls/accepted 316/316, nomem 0; refs/frees 316/316; RX 1426.

| Snapshot uptime | TX calls | accepted | nomem | refs/frees | RX accepted | alloc failures | DMA free/largest |
|---|---:|---:|---:|---|---:|---:|---|
| 370622 baseline | 316 | 316 | 0 | 316/316 | 1426 | 0 | 10280/4596 |
| 437186 | 416 | 413 | 3 | 413/409 | 1706 | 16 | 6616/2932 |
| 496512 | 457 | 413 | 44 | 413/409 | 1961 | 16 | 7640/5876 |
| 521553 | 480 | 413 | 67 | 413/409 | 2062 | 16 | 7640/5876 |

Later rejection deltas 41 then 23 exactly match additional returned TX calls.
All return 257/ESP_ERR_NO_MEM; other_error stays zero. Last ref release tick
remains 427825. Last TX return ticks 433631/496504/521588; last RX ticks
437208/496499/521588. Return/RX observations advance, so this is not a total
freeze of the main loop or all driver ingress. RX accepted denotes netif
submission, not packet type or successful processing of ICMP specifically.

Root metrics retained at response count 7: build/header/body 0/9/5 ms,
3617/3617 body socket accepted, header 100, calls 2, no waits/error/overflow.
Allocation interval 11/11/15; final failure count 16. DMA start/built/end
7488/5620,8392/5620,4000/1524. These are historical request measurements.
All subsequent snapshots retain response count 7 and events 3/no disconnect,
GOT_IP at 12469. Audio reports playing; separate postincident audible
confirmation is not supplied. Main/web polling continues.

Snapshot fields are sampled sequentially with serial output between them.
Later DMA free may exceed earlier INTERNAL free and ticks may exceed header
uptime; this is not an atomic heap/packet snapshot and does not prove corruption.
DMA recovery shows current capacity, not capacity at every individual refusal.

## Exact ELF trace decoding

ELF SHA256 checked: DADF06344EB5C91F650543CDDDDD6EBFB03AF63ACF4C02A691C9ABCD11F98910.
No alternate rebuild used. All retained records are wifi/core0, heap_caps_malloc,
1622 B, INTERNAL|DMA|8BIT; only first four and latest retained. Eleven records
are omitted, so do not assert identical sizes/paths for every intermediate error.

| PC | Function |
|---|---|
| 40084449 | heap_caps_alloc_failed (heap_caps.c:89) |
| 40084585 | heap_caps_malloc (heap_caps.c:198) |
| 400842dd | malloc_internal_wrapper (esp_adapter.c:536) |
| 40240b59 | esf_buf_alloc_dynamic |
| 40241324 | esf_buf_alloc |
| 4020c69d | ic_ebuf_alloc |
| 4023d39f | ieee80211_output_process |
| 4023ed05 | ppTask (records 1..4) |
| 4023dbed | pp_process_hmac_waiting_txq (record 16) |
| 4020fd91 | ppProcTxDone (record 16) |
| 4023ed0e | ppTask (record 16) |

Records 1..4 ticks 418703/418704/418704/418705 confirm TX cache conversion,
not RX allocation. Record16 tick427892 confirms a conversion retry reached
from TX-done processing. Therefore the initial hypothesis of no retry ever
occurring is too strong. Later retry absence/progress is still not measured.
This trace does not itself prove radio success or absence of later completions.

## NO_MEM without heap allocation: verified branches in this ELF

1. API esp_wifi_internal_tx_by_ref at 401ffb7c forwards to
   ieee80211_output_do at 401ffa30; it is not the asynchronous conversion
   callback returning its error directly to the original submitter.
2. Output-do calls ieee80211_alloc_tx_buf at 401ffb15. NULL leads to unlock,
   then literal 0x101 at 401ffb24 and return 401ffb27. No heap allocator is
   called between that NULL and the API return.
3. With cache enabled and a pbuf argument, alloc_tx_buf passes type9 to
   ic_ebuf_alloc at 401fe3a5. A pbuf reference is acquired only on success
   (401fe3b9..401fe3be). Ref count stays flat when this step is rejected.
4. The esf_buf_alloc type9 jump-table entry at 3f44262c is 40241269: the
   static descriptor-list branch. It takes the global lock and reads the
   free-list head at 4024128d. If NULL, 40241299..402412ba only increments
   failure statistics, unlocks and returns NULL. **No heap call, no failed
   heap-allocation callback.** This is a concrete explanation for rising
   API nomem while alloc_failures stays constant, independent of DMA free.
5. Setup calls config_get_wifi_cache_tx_buffer_num then esf_buf_setup_static
   with type9 (relocatable libpp esf_buf_setup offsets 211..229). Thus type9
   is a preallocated cache resource, not an attempt to allocate 1622 B anew
   for each API submission.
6. Other non-heap rejection paths exist: output-do contains an AP-side
   node/admission gate at 401ffac4..401ffaf0 which also reaches 401ffb24.
   It requires interface1/AP and specific node/counter conditions; current
   STA evidence does not establish its execution. Dynamic ESF paths also
   check descriptor quotas at 40240aec and can return NULL before malloc.
   Invalid type/default and empty static pools similarly bypass heap hooks.
   These are separate candidate conditions, not all demonstrated at runtime.

The measured later API rejection is most consistent with lack of an available
cache descriptor or another pre-heap admission limit. Exact executed branch
has not been captured. Do not call it proven type9 exhaustion/queue lockup.

## Correction: effective runtime buffer configuration

Earlier reports used SDK compile defaults (static TX8/cache16) as if effective.
That was incomplete. Pinned Arduino WiFiGeneric.cpp:638 initializes
_wifiUseStaticBuffers=false; lines674..680 override WIFI_INIT_CONFIG_DEFAULT:
static TX0, dynamic TX32, tx_buf_type1, cache TX4, static RX4, dynamic RX32.
Repository has no useStaticBuffers override. Exact ELF wifiLowLevelInit at
400e58c7..400e58f2 retains this branch and passes cfg to esp_wifi_init.
Therefore the source/ELF-established initialization is **dynamic TX32/cache4**,
not the SDK defaults. This is not a readback of private live driver counters.

The persistent ref/free difference of four matches configured cache capacity
and strengthens the retained-cache-resource hypothesis. Ref/free totals are
aggregate callback invocations, not descriptor identities or an inventory;
this numerical agreement does not prove all four descriptors are occupied,
a leak, list corruption, lost work or an unbalanced ownership bug.

## Next discriminating observation; no behavior change

- Capture the actual wifi_init_config_t at the declared esp_wifi_init boundary
  if a further candidate is prepared: confirms initialization values without
  changing policy. Do not infer runtime occupancy from that configuration.
- A safe boundary with a declared prototype can count generic submit rejection,
  but cannot reveal which internal NULL/admission branch fired. The closed ESF
  ABI has no public prototype or synchronized inventory API verified here.
  Wrapping the API after return cannot reconstruct its exited inner stack.
- For direct branch/occupancy proof use a supported Espressif debug facility
  with verified semantics, or a controlled JTAG breakpoint/trace at
  401ffb24, 40241299 and conversion/recycle paths, reading arguments at the
  failure point. Debugger halts perturb timing: treat it as a separate test,
  avoid unsynchronized private-list walking and preserve this boot first.
- PC/AP capture of ongoing ping plus wait-only x snapshots can distinguish
  continuing RX submission, missing over-air replies and TCP retransmission,
  but cannot prove private descriptor ownership by itself.

No private-driver patch, pool resize, gzip, automatic reconnect/reboot, flash
or merge was performed. Evidence now distinguishes DMA allocation failures
during conversion from later API resource/admission refusals. Incident OPEN.

Primary configuration source: [Arduino-ESP32 2.0.17 WiFiGeneric.cpp](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WiFi/src/WiFiGeneric.cpp#L638).
