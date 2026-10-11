# 1.1.6 network progress instrumentation

**Diagnostic candidate; hardware NOT RUN; incident OPEN; PR unmerged.**

Extends failure traces from six to twelve bounded frames, preserving first four
failures and latest. A trace can still end early due to stack validation,
optimization, ISR/cache-off context. Decode only with the published matching ELF.

Extract `firmware-1.1.6-18553c4a207e.elf.zip`, then run:

```text
xtensa-esp32-elf-addr2line -aipfC -e firmware-1.1.6-18553c4a207e.elf 0xPC1 0xPC2 ...
```

`x` now prints NET_PROGRESS: Wi-Fi by-reference transmit calls/results,
registered netstack pbuf ref/free calls, and netif receive calls/results.
Counters are saturating, copied under a short critical section and formatted
outside it. Last activity ticks use pinned FreeRTOS 1000 Hz; cache-off tick
samples are skipped and counted. No packet pointers/data are retained.
Calls are counted after the original returns; these are completed boundary
invocations, not entry counts or proof that an in-flight call is absent.

## Safe boundaries and limits

Linker wrappers observe existing esp_wifi_internal_tx_by_ref,
esp_netif_netstack_buf_ref/free and esp_netif_receive. Their exact prototypes
are declared in the pinned SDK. Each wrapper calls its original exactly once
with unchanged arguments, returns the original result and does not change
pbuf ownership. No callback is replaced. Wrapper bookkeeping is fixed DRAM,
IRAM code with no allocation/logging, and never holds a diagnostic lock while
calling the original. ELF call sites and callback registration must be verified
before publication. These are diagnostic link flags, not a dependency upgrade.

There is no verified public observation hook for private ESF conversion/retry
entry and success in this fixed framework. ic_ebuf_alloc has no public prototype;
private queue access/ABI interception is deliberately not used. The private
single TX-done callback setter has no getter/chaining contract to prove safe
preservation of existing driver callbacks. It is not registered here.

Consequently these counters are **not** direct conversion/retry counts or radio
ACK/completion counts. Failed conversion attempts can be identified only when
they emit allocation failures and the longer trace confirms the consumer.
Successful conversions without failure are not counted individually. Netstack
free is observable driver release of a pbuf reference, not proof of successful
radio transmission or TCP ACK. Ref/free totals denote callback invocations,
not a count of unique packets; use deltas and do not infer exact queue depth.
Transmit acceptance means driver/API acceptance, not delivery. RX acceptance
means submission at the netif boundary, not application consumption.

Instrumentation adds static RAM and timing overhead. Record its own baseline;
do not compare its memory margin as if identical to 1.1.5. HTTP response content,
send/retry policy, dependencies, pins, audio and BT lifecycle are unchanged.
No gzip, automatic reconnect/reboot, retry trigger, flash or merge.

## Short hardware test (about one minute after trigger)

1. Install manually only after preserving any desired evidence from the old
   failing boot. Verify firmware/build_git_id from manifest. Start continuous
   ping and a PC packet capture filtered for tuner IP, ICMP and TCP port 80.
2. Enter Bluetooth; confirm audible audio and successful ping. Before any HTTP,
   capture serial `x`: require web_response_count=0 and alloc_failures=0.
   Save DMA heap and NET_PROGRESS baseline. If either is nonzero, label it and
   restart the controlled test manually if desired; do not call it no-HTTP.
3. Request root once with the same browser. Record whether page loads, ping
   stops, audio continues, WEB_RESPONSE and ALLOC_FAILURE/NET_PROGRESS lines.
   Capture `x` immediately, then after 5, 30 and 60 seconds, with no other
   HTTP requests or restart. Keep continuous ping running throughout.
   Record actual web_response_count: browser-generated auxiliary requests or
   retries must be reported, not silently treated as a single-response test.
4. Decode every retained PC with matching ELF using addr2line -aipfC.
   Look for the caller above ic_ebuf_alloc, especially output_process versus
   RX/management consumers. Return full snapshots and capture, not last fields only.

Interpretation brings new evidence:

- Repeated matching failure traces while DMA space is insufficient support
  continuing allocation pressure/repeated failed work, not a one-off event.
- DMA recovery above requested size, no new failures, RX still advancing,
  and stalled TX acceptance/ref-release activity support lack of TX progress;
  they do not alone prove private queue/event starvation.
- Advancing netstack releases and successful ICMP replies demonstrate recovery;
  advancing releases without replies rules out total reference-release stasis,
  but does not establish radio success.
- Flat RX and TX leaves direction unresolved: AP-side capture is the alternative
  for whether requests arrive over air and replies leave. PC capture alone
  cannot prove tuner receipt; it can show requests, replies and TCP retransmissions.

Do not declare gzip or the incident resolved based on a pressure-reducing test.
