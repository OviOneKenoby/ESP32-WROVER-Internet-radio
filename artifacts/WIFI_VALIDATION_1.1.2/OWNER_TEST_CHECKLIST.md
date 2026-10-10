# TUNER//01 Wi-Fi recovery owner checklist

Status at publication: **PENDING / NOT RUN**.

Before testing, verify the firmware byte count and SHA-256 from the build
manifest. Capture the complete 115200-baud serial log and save
`/api/diagnostics` before and after each interruption.

## 1. Radio AP off/on

1. Boot without pressing controls and start a known-working MP3 station.
2. Confirm audible playback and save diagnostics.
3. Turn the configured AP off. Keep the AP unavailable for at least 60 s.
4. Confirm encoder/UI remain responsive. Record the disconnect log, numeric
   reason/name, framework status, IP (`0.0.0.0` while down), counters and queue
   drops. Do not expect automatic station resume.
5. Turn the same AP on and wait up to 120 s without rebooting or manually
   calling connect.
6. Confirm a new `STA_GOT_IP`, a current nonzero IP, reachable web root and
   reachable `/api/diagnostics`.
7. Start West City or another known-working MP3 station manually and confirm
   audible playback without reboot.

## 2. Bluetooth AP off/on

1. Enter Bluetooth, connect the phone and confirm audible SBC playback.
2. Open and refresh the web page once, then turn the AP off for at least 60 s.
3. Confirm Bluetooth audio and controls remain responsive while Wi-Fi is down.
4. Turn the AP on and wait up to 120 s without rebooting.
5. Confirm web and diagnostics recover, the IP is current, and Bluetooth audio
   still works. Exit Bluetooth cleanly and start MP3 manually.

## Result record

| Check | Result | Evidence |
| --- | --- | --- |
| Correct build ID and SHA-256 | PENDING | PENDING |
| Radio: down state and stale IP cleared | PENDING | PENDING |
| Radio: GOT_IP and web recovered | PENDING | PENDING |
| Radio: manual MP3 start without reboot | PENDING | PENDING |
| Bluetooth responsive while AP absent | PENDING | PENDING |
| Bluetooth: web recovered after AP return | PENDING | PENDING |
| Bluetooth exit and manual MP3 without reboot | PENDING | PENDING |
| Event queue drops remain zero | PENDING | PENDING |

Any panic, reboot, stale connected/IP report, lost Bluetooth responsiveness,
unreachable web after GOT_IP, or inability to start MP3 manually is a FAIL for
the corresponding row. Keep the original uncontrolled incident OPEN unless
the captured evidence establishes and verifies its cause.
