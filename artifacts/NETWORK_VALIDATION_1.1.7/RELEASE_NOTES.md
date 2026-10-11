1.1.7-validation / 9cf48e9022c0 is a diagnostic candidate, not a fix.
Incident OPEN, PR #5 unmerged. No automatic flash. Hardware at publication
NOT RUN; subsequent owner run network FAIL at first root. Exact decode and
correlation are in OWNER_REPORT_2026-10-11.md: four distinct retained pbuf
identities coincide with four Wi-Fi conversion failures, followed by31 NO_MEM
(29 TCP80/two echo replies) with no further registered allocator failure.
Cache-pool exhaustion is strongly supported, but the actual private refusal
branch and the reason progress fails to recover are not yet demonstrated.

Saved 1.1.6 captures already show growing TX NO_MEM after allocation failures
stop and DMA heap recovers. This candidate preserves dynamic TX32/cache4 and
TX behavior while recording actual initialization fields, per-refusal interface,
length/pbuf/header kind, global failed-allocation count around the original API,
bounded pbuf ref/free event identities, and ICMP echo RX/rejected TX counts.
Logging remains deferred to serial x; callbacks allocate no diagnostic memory.

Private descriptor inventory has no verified synchronized interface in the
pinned SDK. No private opcode patch or ABI guess is used. Firmware does not
claim direct branch IDs or cache occupancy. The optional exact-ELF
TX_BRANCH_PROBE.gdb observes the type9 empty-list branch and NO_MEM return
using two hardware JTAG breakpoints, reading registers only. Requires manual
compatible SMP debugging; debugger halts perturb timing. Do not replace hbreak
with software breakpoints. The included README explains inference limits.

Short new hardware test: manual install; BT audio/continuous ping and zero-HTTP
x baseline; confirm TX_CONFIG type1/dynamic32/cache4/static0. Use up to three
root refreshes, stop at failure, record actual routes and capture x immediately,
then +5/+30/+60s without more HTTP. Include TX_NO_MEM, retained-reference rows,
summary and allocation traces; PC packet capture is useful. No repeat of 1.1.6
without new instrumentation is requested. Do not interpret non-reproduction as
a demonstrated fix. No cache resize, gzip fix claim, auto reconnect or reboot.

Artifacts: firmware bin, exact ELF in zip, SHA256SUMS.txt, build manifest,
verification report, optional GDB probe and test README. See manifest for
clean build, host checks and publication verification.
