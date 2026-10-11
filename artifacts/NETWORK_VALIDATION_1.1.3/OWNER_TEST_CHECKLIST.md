# TUNER//01 1.1.3 network instrumentation hardware checklist

Capture the complete 115200-baud serial log. Do not repeat the old test
without snapshots: command `x` is the required observation point.

1. Boot with no controls pressed. Enter `x` once and record the baseline
   BSSID, channel, IP/DNS, heap and loop/web timing.
2. Start West City MP3, pause, return to station select, then enter Bluetooth.
   Confirm SBC is audible. Enter `x` and record one Bluetooth baseline.
3. While the web page still responds, refresh it once and immediately enter
   `x`. Record whether the largest heap block recovers after the response.
4. At the first unreachable web page or failed ping, leave Bluetooth audio
   playing and enter `x` immediately. Record exactly one snapshot; do not scan,
   reconnect or restart first.
5. Wait for either spontaneous recovery or five minutes, then enter `x` once
   more. Record any BSSID/channel change and the last-event age.
6. Exit Bluetooth, enter `x`, then manually start West City. Record whether
   DNS/audio succeeds without reboot.
7. Separately, perform the previously untested controlled router/AP restart
   during audible Bluetooth playback and capture `x` before outage, while
   down, and after recovery.

One observation is required per step. The issue may be called fixed only after
repeated Bluetooth sessions retain web/ping reachability, a controlled AP
restart recovers, Bluetooth exit permits manual MP3 without reboot, queue
drops remain bounded, and snapshots show no hidden loop stall or persistent
association/DNS failure. A clean build or one successful reconnect is not
hardware PASS.
