# Owner hardware report — 11 October 2026

Status: **PARTIAL; network access FAIL; incident OPEN**.

Installed executable: `603c4b12a56b83cae712e891c5b1f41b4c158ca9`,
firmware `1.1.3-validation`. Boot and serial `x` work; reset reason is
`power_on`. Radio opened West City and supplied metadata. Bluetooth reports
SBC playing and metadata, but the owner has **not explicitly confirmed audio
quality for 1.1.3**. Do not mark that audio test PASS.

After the requested web refresh, the owner reported that pages no longer
loaded. No subsequent ping excerpt contains a reply from tuner .129; Windows
received counts include unreachable errors from PC .135. Serial snapshots
continue and loop_count increases. The 13,417 ms web call had completed
before snapshot C: WebPortal::handle updates its duration before serial input
is processed in that iteration. This does not show a still-blocked handler.

BSSID E8:48:B8:66:61:DF/channel 7 stayed constant in captured snapshots;
framework status and association API still report connected. Event counts
remain 3 processed/0 dropped/0 queued. Those observations do not validate IP
traffic or establish roaming, AP failure, coexistence failure or lwIP failure.

Internal free/largest bytes: idle 114272/110580; BT before refresh 9416/6388;
after refresh 8428/3700; later outage 5172/3060; after BT exit 25556/3956.
Allocator-region minima 96/88 are sums of regional minima, not proof of a
simultaneous total free heap of 96/88. No failed allocation was captured.
BT exit recovers heap but not ping. No recovery/reboot/MP3 retest after the
final snapshot is supplied. Direct JSON serialization did not eliminate
the incident.

Controlled router/AP test in Bluetooth 1.1.3: **NOT RUN**. The watchdog boot
and temporary reconnect in the annex belong to the separate historical
1.1.2 session. Source/build/host checks are not hardware validation.
