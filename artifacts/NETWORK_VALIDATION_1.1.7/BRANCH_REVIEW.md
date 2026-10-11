# Optional exact-ELF JTAG observation

**Feasibility correction:** stock1.1.7 initializes encoder GPIO13/14 as GPIO,
conflicting with JTAG. hbreak alone may fall back to software flash breakpoints
in Espressif OpenOCD. Do not run the old proposal on the present failed boot.
See RETRY_RECYCLING_ANALYSIS.md. Revised scripts require a host readiness gate,
ESP_FLASH_SIZE0 before target config, verified real hardware/SMP support and
correct pin mux; they are static candidates, not hardware-validated probes.

ELF SHA256 `22D30785E7C25F3587A04ECFBE73610A438D7D78AB843D1773DE167EBC75D999`.
No target attached or GDB script executed. These are static checks, not hardware
proof. The firmware's metadata alternative is usable without a debugger.

In esf_buf_alloc at 40241340, type arrives in a3. Type9 jump-table entry
is verified from ELF literal/table words and targets 40241369, directly into
the static-list path. That path acquires the driver's existing lock via
callx8 at 4024138a. It loads free-head into a2 at 4024138d and branches on
nonnull at 40241396 to 40241441. **40241399** is reachable on NULL head and
is before a3 is overwritten. Thus hbreak at this point, conditioned on a3==9,
reads a synchronized empty-head observation from a2, without walking the list
or deriving an unsupported numeric descriptor inventory. Windowed ABI call8
preserves caller a2..a7; a3 is unchanged on this path before the probe.
The branch increments its existing exhaustion statistic, unlocks and returns
NULL without a heap allocation call. Breakpoints do not add target writes.

In ieee80211_output_do at 401ffb30, AP admission rejection is gated on ifx1
at 401ffbc4. Normal submission calls ieee80211_alloc_tx_buf at 401ffc15;
its NULL return at 401ffc18 joins the same unlock/NO_MEM path used by the AP
gate. At 401ffc24, a2 is set to 0x101; **401ffc27** returns that value.
The second hbreak reads a2 after initialization; a hit alone does not identify
which upstream NULL/admission branch fired. Type9-empty hits, ordering, STA
metadata and debugger thread/core context are needed for stronger attribution.
No type9 hit with a return does not prove a specific alternative branch; inspect
that context rather than asserting a leak or a private queue stall.

make_tx_branch_probe.py rejects changed patterns and checks the free-head load,
nonnull branch, type9 table entry and unchanged type register. The complete
static paths were separately reviewed against the final ELF. This does not
verify the attached debugger's hardware-breakpoint/SMP semantics.

Use only a compatible manual ESP32 SMP JTAG/OpenOCD session with two available
hardware instruction breakpoints and matching ELF. Do not substitute software
breakpoints or ignore allocation errors. Correctly halt/resume both cores;
single-core halting while holding the driver lock can perturb the other core.
No target function calls or private list dereferences are in the script. Host
convenience counters are not target writes. Halt-based timing remains a material
limitation and this must be labeled a separate experiment.
