# 1.1.7 owner evidence: conversion failure followed by TX refusal

Incident OPEN; PR unmerged. Owner reports that the first root request stopped
ping and the page did not load. No new firmware or behavior change is proposed
in this evidence update. This snapshot does not include the pre-HTTP baseline
or a later wait series; do not invent either. Root's own interval starts with
allocation-failure count0 and ends4. Hardware observation is network FAIL,
with Bluetooth reported playing; audible quality was not explicitly stated
for this particular run.

## Exact ELF decode

Firmware1.1.7-validation/build9cf48e9022c0. Downloaded published ELF SHA256
22D30785E7C25F3587A04ECFBE73610A438D7D78AB843D1773DE167EBC75D999 verified.
Pinned xtensa-esp32-elf-addr2line -pfiaC produced:

| PC | Symbol | Source detail |
|---|---|---|
| 40084865 | heap_caps_alloc_failed | heap_caps.c:89 |
| 400849a1 | heap_caps_malloc | heap_caps.c:198 |
| 400846f9 | malloc_internal_wrapper | esp_adapter.c:536 |
| 40240c59 | esf_buf_alloc_dynamic | closed library, no line info |
| 40241424 | esf_buf_alloc | closed library, no line info |
| 4020c79d | ic_ebuf_alloc | closed library, no line info |
| 4023d4a3 | ieee80211_output_process | closed library, no line info |
| 4023ee05 | ppTask | closed library, no line info |

Caller-to-allocator order: ppTask -> ieee80211_output_process -> ic_ebuf_alloc
-> esf_buf_alloc -> esf_buf_alloc_dynamic -> malloc_internal_wrapper
-> heap_caps_malloc -> failure callback. All four captured failures share this
path, taskwifi/core0/nonISR/cache-enabled, size1622/caps0x80c
(INTERNAL|DMA|8BIT); none omitted. The four ticks are
201730,201731,201731,201732. No TX-done retry frame is present in these records;
the earlier 1.1.6 retry evidence remains separate.

Exact ELF output-process at 4023d4a3 passes type1 to ic_ebuf_alloc for dynamic
conversion. NULL branches at 4023d4aa to 4023d4c9; the existing code requeues
the cache entry at the pending-list head (4023d4d6..4023d4e2), unlocks and
returns0x101 at 4023d4f4/4023d4f7. Successful conversion instead recycles
the cache entry at 4023d4b6. This is the statically verified response to NULL;
the callback frames capture the failed allocation inside that response path,
not the subsequent executed PCs or the reason that later progress stops.

## Correlation at snapshot uptime208720

- TX_CONFIG captured1/init_calls1/result0/type1/dynamic32/cache4/static0,
  features0x3: initialization values are now observed at the actual API,
  including cache support bit0x2; not a live descriptor inventory.
- TX155 = accepted124 + NO_MEM31, other_error0. Ref124/free120; last free
  tick201727. Ledger overflow0/unmatched_free0, four DISTINCT opaque handles
  with refs1 each:
  3ffff410@201730, 3f8041d8@201730, 3f8047e8@201731, 3fffd140@201732.
  Their first retained events overlap the allocation-failure window. This
  excludes the explanation that the observed difference is four refs of one
  handle. It does not pair each failure with a particular handle: counts/ticks
  and allocation records have no common packet ID; two failures share201731.
  Ledger is callback-event bookkeeping, subject to ordering/identity reuse,
  not private descriptor ownership or actual pbuf refcount readback.
- All31 refusals classify as TCP80(29) or echo replies(2). Retained examples:

| Sequence | Tick | ifx | Length | Pbuf | Kind | Allocation before/after |
|---|---:|---:|---:|---|---|---|
| 1 | 201916 | 0 | 58 | 3fffce70 | TCP80 | 4/4 |
| 2 | 202373 | 0 | 58 | 3fffce70 | TCP80 | 4/4 |
| 3 | 202748 | 0 | 74 | 3fffe090 | EchoReply | 4/4 |
| 4 | 202873 | 0 | 58 | 3fffce70 | TCP80 | 4/4 |
| 31 | 208394 | 0 | 58 | 3fffd010 | TCP80 | 4/4 |

  All retained cases have nonnull buffer/pbuf, STA ifx0, ESP_ERR_NO_MEM257.
  The allocation total is still4, so no further registered failure is shown
  before this snapshot. Unretained refusal metadata is not reconstructed;
  first4/latest storage does not provide all31 intervals. Repeated refusal
  handle does not by itself prove TCP retransmission or a TCP flags value:
  classification checks ports/protocol, not connection, sequence or flags.
- RX701 accepted701, last tick208684; cumulative echo requests102. Two TX echo
  replies were generated/classified and then explicitly refused: an output-side
  failure affects ping as well as TCP. The snapshot does NOT timestamp each
  accepted echo request, correlate request/reply IDs or prove every post-fault
  request arrived. Advancing unrelated RX alone is insufficient for that claim.
- Last free precedes retained events by3..5ms. Observation is approximately7s
  after the first retained event; refusals span201916..208394. This is short
  persistence, not a demonstrated minute-long wait for this build.
- Current DMA5840/largest2676, internal5436/largest2676. Samples are sequential,
  not atomic; largest exceeding1622 now does not describe the heap at failures
  or prove a future allocation would succeed. Do not import the recovered5876
  largest-block result from 1.1.6 into this run.
- Root3617 bytes locally accepted, two socket calls/no retries/errors, handler
  ends with four allocation failures. Socket acceptance does not prove delivery.
  Loop/association continue, no Wi-Fi disconnect/events after GOT_IP.

## What is established, and the remaining branch proof

Established: dynamic TX conversion fails in Wi-Fi during root; four distinct
netstack reference identities remain in observed callback bookkeeping; later
small STA TCP80 and echo-reply submissions receive NO_MEM while the failure
counter remains4. Init cache4 is measured, so its agreement with four retained
identities is substantially stronger than the old aggregate ref/free count.
The observed outage has a TX refusal component affecting ping, not only JSON,
HTTP construction or mere lack of RX/association. Complete causal assignment
of the persistent outage and every lost packet is not yet established.

Cache-pool refusal is the strongest supported hypothesis, not direct runtime
branch proof. Exact ELF alloc_tx_buf (401fe48c) tests cache support at401fe495;
enabled plus nonnull pbuf selects type9 at401fe49f..401fe4a5. Its reference
callback occurs only on successful descriptor allocation at401fe4be. Type9's
static free-list NULL branch returns without a heap call, explaining how
NO_MEM can rise while allocator failures do not. ifx0 rules out the AP-only
gate for the recorded cases. These observations constrain the code path but
are not a capture of the getter's live return, private head or executed branch.
Ledger entries do not establish private pool capacity/ownership directly.
Do not label this a proven leak, private-list corruption or permanently blocked
queue. Why retries/recycling cease to restore progress remains unresolved.

Remaining decisive observation: exact-ELF hardware probe at40241399 conditioned
on a3==9 must show a2==0 at the synchronized free-head failure point; pair its
event with output-do NO_MEM return401ffc27 using thread/core/call context.
That demonstrates zero availability on the relevant type9 allocation attempt,
without an unsafe list walk. A return-only hit or numerical hit-count match
is insufficient. The optional published script and BRANCH_REVIEW.md explain
SMP/halt timing limitations; JTAG has NOT been run. A supported synchronized
vendor diagnostic with equivalent semantics would also supply this evidence.

No repeat of 1.1.6 is requested. Any already collected wait snapshots for this
1.1.7 run can establish whether the same identities persist while DMA recovers;
such observations strengthen persistence evidence but cannot replace branch
capture. No cache resize, gzip fix claim, auto reconnect/reboot, flash or merge.
