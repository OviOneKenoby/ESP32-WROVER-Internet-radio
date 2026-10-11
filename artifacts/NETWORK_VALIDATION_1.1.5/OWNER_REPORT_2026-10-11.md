# 1.1.5 / f33ef76cfc29: first root request reproduces outage

**Incident OPEN; network FAIL reproduced; task attribution demonstrated;
persistence mechanism unverified. PR remains unmerged.**

Owner reports Bluetooth baseline with functional continuous ping and audible
audio, web_response_count=0 and alloc_failures=0. The first HTTP root request
stops ping and does not load in the browser. Unlike 1.1.4, this baseline has
no recorded HTTP responses. Its full raw snapshot was not supplied here.

## Response and snapshot

| Field | Observation |
|---|---|
| Firmware/build/uptime/reset | 1.1.5-validation/f33ef76cfc29/299420/power_on |
| Root response count; build/header/body ms | 1; 1/2/10 |
| Body expected/socket accepted; header accepted | 3617/3617; 100 |
| Socket calls/waits/error; failed/overflow | 2/0/0; no/no |
| INTERNAL free/largest start/built/end | 9084/4852; 8844/4852; 4052/1652 |
| DMA free/largest start/built/end | same as INTERNAL at these phases |
| Allocation failures start/built/end | 0/0/4 |
| Current INTERNAL free/largest; allocator-region minimum | 6996/3956; 1520 |
| Current DMA free/largest; PSRAM free | 6996/2932; 4175447 |
| Audio | Bluetooth/SBC/playing/active/restartable=yes |
| Audio/main stack HWM | 7648/4868 |
| Loop count/current/last/max/completed age ms | 21317/1/11/2516/1 |
| Web handle count/last/max/age ms | 21317/1/33/0 |
| Hook result; total/internal-cap failures | 0; 5/5 |
| Wi-Fi cached/framework/association | connected/connected(3)/available, result 0 |
| BSSID/channel/RSSI | E8:48:B8:66:61:DF/7/-48 dBm |
| IP/gateway/subnet/DNS | 192.168.1.129/192.168.1.1/255.255.255.0/192.168.1.1,0.0.0.0 |
| Events processed/dropped/queued; disconnect/got-IP/lost-IP | 4/0/0; 1/1/0 |
| Last disconnect; GOT_IP | reason 39(TIMEOUT) at 11280; GOT_IP at 13494 |
| Snapshot event ages, disconnect/GOT_IP | 288140/285926 ms |

The boot TIMEOUT precedes the incident around tick 288002; do not attribute it
to root refresh. Existing auto_reconnect=yes is a reported setting, not a
new recovery action. Postincident playing state is not a separate explicit
owner confirmation of audible output. Socket acceptance is not browser receipt.

All five individual records: size 1622, caps 0x0000080c, heap_caps_malloc,
task wifi, handle 0x3ffdc564, core 0, ISR=0, cache=1, six frames, omitted=0.
Ticks for sequences 1..5: **288002,288002,288011,288013,288072**.
All PCs: **400842e5,40084421,40084179,40240acd,40241298,4020c611**.
Four failures are within the response interval, fifth later. Individual
records now establish all five sizes/caps, rather than last/largest inference.

## Exact ELF decoding

Preserved ELF SHA256 matches published/downloaded matching ELF:
`341E063BA572B96EA62E2EC1FF0EDC5573F1DDE103BE0E40930CFA86759FB7A5`.
Executable commit: f33ef76cfc2906d8f2e947d8c8e1e894a66c6de9.

```text
xtensa-esp32-elf-addr2line -aipfC -e firmware-1.1.5-f33ef76cfc29.elf 0x400842e5 0x40084421 0x40084179 0x40240acd 0x40241298 0x4020c611
```

| PC | Decoded function | Exact ELF source information |
|---|---|---|
| 0x400842e5 | heap_caps_alloc_failed | components/heap/heap_caps.c:89 |
| 0x40084421 | heap_caps_malloc | components/heap/heap_caps.c:198 |
| 0x40084179 | malloc_internal_wrapper | components/esp_wifi/esp32/esp_adapter.c:536 |
| 0x40240acd | esf_buf_alloc_dynamic | binary library; no source line |
| 0x40241298 | esf_buf_alloc | binary library; no source line |
| 0x4020c611 | ic_ebuf_alloc | binary library; no source line |

Call order is reversed. Wi-Fi ESF packet-buffer allocation is demonstrated;
JSON allocation is not the failed request. Six frames end at a shared wrapper,
before its consumer: task attribution alone does not prove TX versus RX.

## Verified binary failure paths

The following findings come from objdump of this exact ELF and relocations
of the linked libpp/libnet80211 archives. Closed-library behavior is binary
evidence, not reconstructed authoritative C source. Public IDF 4.4.7 sources
support open adapter/netif behavior; archive identities are in the 1.1.4 analysis.

- At 0x40240ab0 the dynamic allocator loads OS adapter offset 0x15c
  (malloc_internal), forming 96 + descriptor headroom + requested payload.
  Call 0x40240acd reaches wrapper INTERNAL|DMA|8BIT.
- NULL branches from 0x40240ade to 0x40240b7c. If quota was reserved, it is
  decremented, failure statistics updated and the global lock released.
  NULL returns at 0x40240bd2/0x40240bd4, propagates through esf_buf_alloc
  (0x4024129b/0x4024129e) and ic_ebuf_alloc (0x4020c614/0x4020c616).
  No unconditional retry, lock leak or panic is shown in this branch.
- Concrete candidate consumer: ieee80211_output_process (0x4023d25c).
  It removes the cache queue head under lock, then tries converting it to
  DMA ESF type 1 at 0x4023d313. NULL test is 0x4023d31a. Failure branch
  0x4023d339..0x4023d367 reinserts the original cached packet at the **head
  of s_tx_cacheq**, unlocks and returns 0x101 (ESP_ERR_NO_MEM). It does not
  recycle the original packet/pbuf or process smaller packets behind it in
  that invocation.
- On successful conversion, ieee80211_recycle_cache_eb calls the registered
  netstack free callback and recycles the old descriptor. Open netif callbacks
  map ref/free to pbuf_ref/pbuf_free; therefore deferred conversion can retain
  a pbuf independently of fd closure. Correction from later source inspection:
  SDK defaults are static TX8/cache16, but Arduino's default init overrides
  these to dynamic TX32/cache4. See the 1.1.6 owner report; no buffer change
  was applied. Cache conversion still needs a dynamic DMA packet buffer.
- ieee80211_output_init registers output_process as the net80211 TX callback.
  ppTask dispatch at 0x4023ec79 calls it, then jumps at 0x4023ec7c to the event
  loop at 0x4023ed11, without checking OOM or scheduling immediate retry in
  that dispatch branch. Other driver events may trigger retries; absence of
  local retry does **not** prove permanent starvation.

Type 1's initial descriptor headroom is 36, making 1622=96+36+1490. A 1490-byte
Ethernet frame is compatible with MSS1436 plus IPv4/TCP/Ethernet headers.
Type/length were not captured; RX decapsulation and management consumers also
call ic_ebuf_alloc. **Do not promote the TX candidate to the observed hardware
caller without an additional caller frame.**

## Mechanism boundary and next verification

Supported hypothesis: head-of-queue retention plus absent/lost TX work trigger
could prevent progress, retain pbuf refs, and keep smaller ICMP replies behind
the large frame. Downstream TX completion or RX stall is another possibility.
No runtime queue occupancy, missing event or completion has been measured.

At snapshot DMA largest=2932 exceeds 1622. The failures at tick 288002 do not
prove ongoing allocation inability at uptime 299420. Retrying a head with
available matching-capability memory could succeed; timing, quota and actual
retry execution remain unknown. No later wait-only snapshot for this boot was
supplied. wlanif.c documents that generic memory-related packet drops are not
retried except via TCP timers; this explains transient loss, not indefinite
ICMP loss alone. Connected association is not functioning IP traffic.

Required discriminating evidence:

1. Longer bounded trace, e.g. 12 frames, still fixed storage/no callback
   allocation or logging, to reveal caller above ic_ebuf_alloc.
2. If output_process confirmed: conversion attempts/NULL/success over time,
   TX work and completion, netif RX/ICMP, packet type/length and outstanding
   pbuf ref/free balance through supported boundaries or verified wrappers.
   Do not walk mutable private queues unsynchronized or patch their layouts.
3. Wait-only serial snapshot plus PC/AP capture of continuous ping to distinguish
   requests, absent replies, retransmission and ACK behavior. Preserve failed
   state; avoid further HTTP probes altering the observation.

These are remaining tests, not performed measurements. No firmware changes,
gzip, allocator policy change, private driver patch, reboot/reconnect or flash
were made for this report. Gzip is only a proposed pressure reduction and must
pass hardware verification before being called a resolution. A confirmed
lost-work mechanism requires correct retry/ownership handling in the driver,
not an application recovery action concealing the fault. **Incident OPEN.**

Primary version context: [ESP-IDF 4.4.7 Wi-Fi guide](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-guides/wifi.html).
