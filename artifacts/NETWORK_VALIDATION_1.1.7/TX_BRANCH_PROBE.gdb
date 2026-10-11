# OPTIONAL: manually attach a compatible ESP32 SMP OpenOCD/JTAG setup first.
# BLOCKED on stock 1.1.7 wiring/initialization: GPIO13/14 are encoder GPIO.
# hbreak ALONE is NOT sufficient: start OpenOCD with ESP_FLASH_SIZE=0
# before target configuration; verify two real hardware slots on both cores.
# Set host $tuner_jtag_ready=1 ONLY after the protocol's feasibility checks.
# Use hardware breakpoints only; no software breakpoint/instruction patch.
# Never call target functions. Halts perturb timing; label a separate experiment.
# Verify this ELF is loaded and both cores are halted/resumed appropriately.
if $tuner_jtag_ready != 1
echo STOP: JTAG feasibility/hardware-only setup not verified.\n
else
set $cache9_empty_hits = 0
set $tx_nomem_returns = 0
set $branch_probe_stops = 0
hbreak *0x40241399
commands
silent
set $branch_probe_stops = $branch_probe_stops + 1
if $a3 == 9
set $cache9_empty_hits = $cache9_empty_hits + 1
printf "[JTAG_CACHE9_EMPTY] hits=%u thread=%u sp=0x%08x pc=0x%08x type=%u head=0x%08x\n", $cache9_empty_hits, $_thread, $a1, $pc, $a3, $a2
bt 6
end
if $branch_probe_stops >= 16
disable
echo SAMPLE_LIMIT: all probes disabled after 16 stops.\n
end
continue
end
hbreak *0x401ffc27
commands
silent
set $branch_probe_stops = $branch_probe_stops + 1
set $tx_nomem_returns = $tx_nomem_returns + 1
printf "[JTAG_TX_NOMEM_RETURN] returns=%u cache9_empty_hits=%u thread=%u sp=0x%08x pc=0x%08x result=%u\n", $tx_nomem_returns, $cache9_empty_hits, $_thread, $a1, $pc, $a2
bt 6
if $tx_nomem_returns >= 8 || $branch_probe_stops >= 16
disable
echo SAMPLE_LIMIT: all probes disabled after 8 returns.\n
end
continue
end
end
