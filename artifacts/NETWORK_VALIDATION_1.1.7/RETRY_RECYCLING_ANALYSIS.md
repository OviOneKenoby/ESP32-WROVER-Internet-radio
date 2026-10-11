# Exact 1.1.7 retry/recycling and feasibility analysis

## Follow-up: UART-only equipment, 1.1.9 software observation

The owner confirmed CH340/UART only, no JTAG adapter. The JTAG protocol is
archived and cannot be executed here. New feasibility work found declared
OSI callback boundaries that indirectly identify private instruction paths:
typed mutex-unlock and allocation callbacks, forwarded through a static table
copy before init, with no private ABI calls or binary patches. This changes
the earlier observation feasibility, not the status of the 1.1.7 evidence.
See [software feasibility](../NETWORK_VALIDATION_1.1.9_UART/FEASIBILITY_RO.md)
and the [single complete UART protocol](../NETWORK_VALIDATION_1.1.9_UART/PROTOCOL_RO.md).
Runtime branch/pool/retry attribution is pending hardware observation;
no direct pool inventory, leak, queue deadlock or causal fix is demonstrated.
Incident OPEN. Main/source main.cpp unchanged; no automatic flash/merge/recovery.

Uses ELF22D30785E7C25F3587A04ECFBE73610A438D7D78AB843D1773DE167EBC75D999
and owner wait interval84.314s. Firmware TX policy is not changed.

## Static chain, separately from execution evidence

1. ieee80211_output_init401fe878 registers output_process4023d3ec through
   ic_register_net80211_tx_cb4020c834 -> pp_register_net80211_tx_cb4020f67c.
   Literal401fe86c contains4023d3ec. Setter stores callback in3ffcd0f4;
   later ppTask/pp_process_hmac_waiting_txq use the same callback slot.
   This is initialization code, not a readback of a potentially overwritten
   live pointer. Captured failure frames show ppTask invoking output_process.
2. ieee80211_post_hmac_tx401fe594 appends a successfully admitted cache entry
   to the pending list and submits existing event5 (401fe5bd..401fe5c5).
   ieee80211_output_do reaches this call only after buffer allocation succeeds.
   An admission NULL/NO_MEM submission does not itself take this notification
   path. A separate stop condition can recycle instead; inspect state when hit.
3. output_process dequeues a cache entry and attempts type1 conversion at
   4023d4a3. At4023d4aa, a10 is the result and a2 the dequeued cache entry.
   NULL -> existing locked requeue at head4023d4d6..4023d4e2 -> unlock ->
   return0x1014023d4f4/4023d4f7. This invocation does not spin until memory
   returns. NonNULL -> copy header -> recycle_cache_eb at4023d4b6.
4. recycle_cache_eb401fe4e8 reads saved pbuf at401fe4eb, invokes registered
   release callback401fe502 if present and then ic_ebuf_recycle_tx401fe507.
   Entering this function is not proof of completed recycling/callback execution.
   User-visible frees remain the separate actual callback-return observation.
5. ppProcTxDone4020fda0 drains completions. At4020fe78..4020fe84 it sets local
   a4 for an observed completed type1 entry. At4020fe92, retry is gated on a4;
   if set it calls pp_process_hmac_waiting_txq4020fe95. That function checks
   cache-enabled and invokes the registered output callback4023dcf1.
   Thus not every completion necessarily triggers this particular retry.
6. ppTask uses a queue receive with blocking argument -1 at4023ed50..4023ed59;
   adapter queue_recv_wrapper maps OSI_FUNCS_TIME_BLOCKING to portMAX_DELAY.
   Its RX dispatches are distinct from event5/output and TX-done dispatches.
   Continued RX activity need not retry cached output. No periodic retry is
   demonstrated merely by a running main loop or larger heap block.

## Most useful persistence hypothesis

Conversion fails while cache entries are held; subsequent calls exhaust the
admission resource. Rejected new calls do not enqueue normal output work;
if qualifying real TX completions also cease, the identified retry trigger may
cease even after heap recovers. This possible dependence on events explains
stable four identities, no frees/accepted and growing NO_MEM without a leak.
It is NOT proven that every retry trigger ceased: private timer/callback/state
paths and quota/list conditions are not all observed. Other possibilities
include retries that return NULL before the heap callback (quota), lost/unhandled
work notification, another queue or state gate, or driver corruption. Do not
turn this static dependency into a demonstrated queue deadlock.

New observations needed: actual type9 NULL-head branch and paired NO_MEM;
then conversion-result visits, context identifying direct/event5 vs TX-done
retry, and cache-recycle entry paired with real callback free events. The two
GDB phases use two instruction breakpoint slots separately. No queue walk,
private-function ABI wrapper or driver patch is justified.

## Correction to earlier JTAG proposal

The original scripts were statically located but not operationally validated.
Two blockers were previously under-specified:

- Stock firmware initializes encoder GPIO13/14 via pinMode/encoderClick.attach;
  Arduino __pinMode invokes gpio_config, which selects PIN_FUNC_GPIO. These
  overlap ESP32 MTCK/MTMS, so physically disconnecting the encoder alone does
  not preserve the JTAG mux. Do not attach the proposed experiment to the
  stock failed boot by guessing register writes. A separate diagnostic env
  disables encoder initialization/polling/interrupts, preserving JTAG mux;
  it changes input behavior and baseline timing, not TX. Physical encoder
  wires still must be isolated before manual new test.
- Espressif OpenOCD can emulate requested hardware flash breakpoints using
  software flash breakpoints. hbreak alone is insufficient. Disable flash
  breakpoints before loading target config with ESP_FLASH_SIZE0 and verify
  actual hardware insertion/SMP in debug logs. Unknown command/version/setup
  means STOP, not fallback. Two slots total per core; phases are separate.

Sources: pinned Arduino esp32-hal-gpio.c:92..128; analysis IDF4.4.7
components/driver/gpio.c:385, esp_adapter.c:428..433; official IDF4.4.7
[JTAG tips](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-guides/jtag-debugging/tips-and-quirks.html)
and [ESP32 pins/configuration](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-guides/jtag-debugging/configure-other-jtag.html).
JTAG/OpenOCD/GDB target execution NOT RUN; adapter model not supplied.
Firmware candidate is local only; complete conditional protocol accompanies
it. No new release that merely repeats counters, no flash/merge/reconnect/reboot.
