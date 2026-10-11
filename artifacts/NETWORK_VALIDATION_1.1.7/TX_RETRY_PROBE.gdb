# Same hardware-only/SMP/pin feasibility gate as TX_BRANCH_PROBE.gdb.
# Use separately: delete phase-A breakpoints first. Never load/reset target.
if $tuner_jtag_ready != 1
echo STOP: JTAG feasibility/hardware-only setup not verified.\n
else
set $retry_probe_stops = 0
hbreak *0x4023d4aa
commands
silent
set $retry_probe_stops = $retry_probe_stops + 1
printf "[JTAG_CONVERSION_RESULT] stop=%u thread=%u sp=0x%08x pc=0x%08x cache_eb=0x%08x result_eb=0x%08x\n", $retry_probe_stops, $_thread, $a1, $pc, $a2, $a10
bt 8
if $retry_probe_stops >= 16
disable
echo SAMPLE_LIMIT: all retry probes disabled.\n
end
continue
end
hbreak *0x401fe4eb
commands
silent
set $retry_probe_stops = $retry_probe_stops + 1
printf "[JTAG_CACHE_RECYCLE_ENTRY] stop=%u thread=%u sp=0x%08x pc=0x%08x cache_eb=0x%08x\n", $retry_probe_stops, $_thread, $a1, $pc, $a2
bt 6
if $retry_probe_stops >= 16
disable
echo SAMPLE_LIMIT: all retry probes disabled.\n
end
continue
end
end
