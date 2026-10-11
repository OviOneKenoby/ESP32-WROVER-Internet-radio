# OPTIONAL: manually attach a compatible ESP32 SMP OpenOCD/JTAG setup first.
# Use hardware breakpoints only; no software breakpoint/instruction patch.
# Never call target functions. Halts perturb timing; label a separate experiment.
# Verify this ELF is loaded and both cores are halted/resumed appropriately.
set $cache9_empty_hits = 0
set $tx_nomem_returns = 0
hbreak *0x40241399
commands
silent
if $a3 == 9
set $cache9_empty_hits = $cache9_empty_hits + 1
printf "[JTAG_CACHE9_EMPTY] hits=%u pc=0x%08x type=%u head=0x%08x\n", $cache9_empty_hits, $pc, $a3, $a2
end
continue
end
hbreak *0x401ffc27
commands
silent
set $tx_nomem_returns = $tx_nomem_returns + 1
printf "[JTAG_TX_NOMEM_RETURN] returns=%u cache9_empty_hits=%u pc=0x%08x result=%u\n", $tx_nomem_returns, $cache9_empty_hits, $pc, $a2
continue
end
