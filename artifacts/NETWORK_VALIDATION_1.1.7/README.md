# 1.1.7 refusal discriminator: safe boundary alternative

**Hardware NOT RUN; incident OPEN; PR unmerged.** Uses the saved 1.1.6 captures,
which already demonstrate conversion failures, a retry via TX-done processing,
then growing API NO_MEM with recovered DMA heap and four unreleased ref events.
No repetition of 1.1.6 is requested.

## Private branch observation limit

Direct executed-PC capture at type9's empty-list branch requires a debugger,
supported vendor trace or private binary modification. There is no declared
ESF ABI/synchronized descriptor inventory interface verified in this SDK.
No binary opcode patch, private-list walk or guessed-function wrapper is used.
Therefore this candidate does **not** print a direct branch ID or cache-free
descriptor count. It supplies a new alternative experiment using declared
boundaries, with fixed storage and unchanged function arguments/results.
An optional TX_BRANCH_PROBE.gdb, generated and reviewed against this exact ELF,
uses two hardware instruction breakpoints to observe the type9 empty-list branch
and output-do NO_MEM return directly. It requires a compatible external JTAG/SMP
debug setup; no probe is installed, attached or run by Codex. It reads registers
only, never calls target functions or modifies driver data/instructions.

## New evidence

- TX_CONFIG captures scalar fields actually passed to esp_wifi_init and its
  result. It neither edits configuration nor retains cfg pointer. Expect
  result0/type1/dynamic32/cache4/static0, feature bit 0x2 set for cache support.
  This records initialization, not live private-state readback.
- TX_NO_MEM preserves first four and latest refusal: tick, interface, length,
  nonnull buffer flag, opaque pbuf identity, bounded header kind and global
  allocation failure count immediately before/after the original TX API call.
  Equal counts show no registered failed-allocation callback during that interval;
  changed counts include concurrent tasks and are not automatic attribution.
- TX_RETAINED_REF records up to eight opaque pbuf identities, reference-event
  multiplicity and first tick. Only reference/free callback events are tracked;
  no pbuf dereference, private descriptor read or packet payload retention.
  ledger_overflow/unmatched_free invalidate claims of complete tracking.
  Callbacks and serial observations can interleave; this is a shadow of events,
  not an atomic inventory of private queues. Handles may be reused after release.
- TX_REFUSAL_SUMMARY counts accepted netif ICMP echo requests and rejected TX
  echo replies/TCP80 packets. Kind values: 0 Other, 1 ARP, 2 TCP80,
  3 EchoRequest, 4 EchoReply. IPv4 nonfragmented Ethernet only; malformed,
  fragmented, VLAN, unsupported/cache-off traffic is Other. Classification occurs
  before original ownership transfer and stores no payload bytes.

No extra packets/probes, allocation/logging in callback, retry, reconnect,
reboot, gzip or change to TX policy/buffer sizing. Original API called once;
no diagnostic lock spans original calls. Instrumentation adds RAM and timing
overhead, so retain this candidate's baseline. Existing 12-frame traces remain.

## Short new hardware experiment

1. Preserve any desired evidence from current failed boot, then install manually
   and verify manifest identity. Start continuous ping and PC packet capture.
   In BT, confirm audible audio/ping and capture x before any HTTP; require
   response_count0/failures0, TX_CONFIG expected values, ledger flags0.
2. Use the same browser root trigger, up to the third refresh as in saved 1.1.6,
   stopping immediately at failure. Record actual response routes/count; browser
   auxiliary requests are not hidden. If no failure after three refreshes,
   report that result rather than asserting resolution.
3. Capture x immediately/+5/+30/+60 seconds, without further HTTP/restart.
   Return TX_CONFIG, TX_NO_MEM, TX_RETAINED_REF, summary, full snapshots and
   existing allocation traces. Decode PCs with this candidate's exact ELF.

Discriminating outcomes:

- ifx0/STA rules out the AP-only admission gate for those refusals. Nonnull
  pbuf plus observed cache-enabled initialization selects the cache-capable
  submission path in the pinned binary analysis. This is control-flow
  inference, not a captured private PC; inspect all conditions before attributing.
- Stable failuresBefore==failuresAfter across new NO_MEM, persistent identities,
  and echo requests still accepted while echo replies are refused support
  resource/admission exhaustion affecting small replies despite DMA recovery.
  Four outstanding events matching cache4 strengthen, but do not prove, pool
  exhaustion, a leak or descriptor ownership. Distinct identities distinguish
  four held buffers from repeated references to one pbuf.
- Advancing release events/identity turnover during refusal weakens the simple
  permanently-retained-four-buffer model; heap failure deltas or non-STA/null
  pbuf cases select different explanations. Missing echo RX leaves request
  delivery unresolved even if unrelated RX increases.
- Definitive private branch proof remains a controlled JTAG trace/breakpoint
  or supported vendor diagnostic with verified synchronization. Use matching
  ELF to locate output-do NO_MEM return and type9 empty-list branch; never
  reuse 1.1.6 addresses after relinking. Halt-based tests perturb scheduling
  and must be labeled as a separate experiment.
  The supplied script uses hbreak only; do not substitute software breakpoints
  or silently accept hardware-breakpoint allocation failure. Correct SMP/core
  halt handling is required. Cache9-hit/NO_MEM counts alone do not pair calls
  across threads; use event order, interface metadata and debug context.

No outcome is a hardware-verified fix. Incident remains OPEN.

## Exact artifact and decode

Executable source: `9cf48e9022c0a9e8b88943c073bb29f447bfa80a`;
embedded build ID `9cf48e9022c0`, firmware `1.1.7-validation`.
Use firmware-1.1.7-validation-9cf48e9022c0.bin and extract
firmware-1.1.7-9cf48e9022c0.elf from the matching zip. Verify all entries in
SHA256SUMS.txt. Run the pinned xtensa-esp32-elf-addr2line with `-pfiaC -e`
that exact ELF followed by the new PC addresses. The optional GDB script's
addresses apply only to this ELF; no earlier addresses may be substituted.
BUILD_MANIFEST.md and ELF_VERIFICATION.txt record the final checks.
