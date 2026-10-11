# Owner hardware evidence — 1.1.4 / e135a1c0aace

**PARTIAL validation; network outage reproduced; incident OPEN.**

Before refresh, uptime 400084: Bluetooth active/SBC playing; continuous ping
had no interruptions observed. Seven HTTP responses had already occurred
since boot (last route stations): this is **not a complete no-HTTP test**.
Allocation failures 0/internal-capability failures 0; internal free/largest
9252/6644. Wi-Fi connected, BSSID E8:48:B8:66:61:DF/channel 7; disconnects 0,
processed 3/dropped 0/queued 0.

The owner refreshed the **main page**, and observed ping stop exactly then.
This was not an isolated `/api/diagnostics` request. At uptime 457789:

| Field | Observation |
|---|---|
| Failed allocation counts | total 5 / internal-capability 5 |
| Last / largest failed allocation | 1622 / 1622 bytes |
| Last requested capability mask | 0x0000080c |
| Internal free / largest | 4940 / 1972 bytes |
| Response count / last route | 9 / root |
| Root build / header / body | 1 / 2 / 2 ms |
| Root body expected / socket-accepted | 3617 / 3617 bytes |
| Header socket-accepted | 100 bytes |
| Socket calls / retry waits / socket error | 2 / 0 / 0 |
| failed / json_overflow | no / no |
| Internal free/largest: start / built / end | 9968/7156 / 9840/7156 / 5128/1396 |
| Cumulative allocation failures: start / built / end | 0 / 0 / 4 |
| Current / last loop elapsed | 1 / 11 ms |
| Maximum web handle time | 47 ms |
| Association / BSSID / new Wi-Fi events | still connected / unchanged / none |

Four failures occurred between response start and end samples, after the
first-header sample. This interval includes concurrent activity in other
tasks and WebServer/client cleanup, not just execution of root's code. Five
failures are recorded by the later snapshot. Only the last and largest sizes
were retained: these fields do not establish that every failure was 1622 B
or carried the last capability mask. Socket-accepted byte counts do not
prove complete browser receipt or TCP acknowledgement.

New demonstrated fact: **internal-capability allocation failures are temporally
correlated with main-page refresh and loss of ping**. Requester, allocation
call path, retention and causal mechanism are not yet established by the
hardware record. Fast output/no retry excludes a long response-write retry
in this observed root request; it does not establish data-plane health.

## Final wait-only snapshot: observation concluded

Owner supplied the final read-only snapshot at uptime **2088101 ms** and
confirmed continuous ping still received no replies. This is **1630312 ms
(27 min 10.312 s)** after the first post-refresh snapshot. No further HTTP
response is recorded: response count remains 9 and retained root metrics
remain unchanged. The owner concludes the 1.1.4 observation.

| Field | Final observation |
|---|---|
| Firmware / build / reset | 1.1.4-validation / e135a1c0aace / power_on |
| Wi-Fi cached/framework/association | connected / connected(3) / available, result 0 |
| BSSID / channel / RSSI | E8:48:B8:66:61:DF / 7 / -47 dBm |
| IP / gateway / subnet / DNS | 192.168.1.129 / 192.168.1.1 / 255.255.255.0 / 192.168.1.1, 0.0.0.0 |
| Events processed / dropped / queued | 3 / 0 / 0 |
| Disconnects / got-IP / lost-IP | 0 / 1 / 0 |
| Last event | STA_GOT_IP at 12468 ms, age 2075633 ms |
| Internal free / largest / allocator-region minimum | 8232 / 3188 / 1364 bytes |
| PSRAM free | 4177447 bytes |
| Audio | Bluetooth / playing / SBC / active / restartable=yes |
| Audio / main task stack HWM | 5900 / 4780 |
| Loop count / current / last / maximum / completed age | 155894 / 1 / 11 / 6053 / 1 ms |
| Web handle count / last / maximum / age | 155894 / 1 / 47 / 0 ms |
| Hook result / failures / internal-cap failures | 0 / 5 / 5 |
| Last size / caps / largest failed size | 1622 / 0x0000080c / 1622 |
| HTTP response count / last route | 9 / root |

Internal free increased **3292 B**, largest block increased **1216 B**, and
no additional allocation failures were recorded during this interval. Ping
did not recover despite this partial heap recovery. This weakens a purely
transient low-free-heap explanation; it does not prove a leak, exhausted DMA
arena, stuck driver, missing packet completion or irreversible allocator
damage. Current free/largest are INTERNAL samples, not the exact DMA-capable
arena; 1.1.4 has no requester PCs or DMA-specific samples. Historical response
heap/end and allocation start/built/end fields are retained measurements of
the old request, not new measurements at uptime 2088101.

The main loop and web polling continue; no new association event or reset is
observed. Cached/framework connected status does not establish functioning IP
traffic. Audio state remains playing in this snapshot; this alone is not a
new explicit confirmation of audible output. `auto_reconnect=yes` is the
reported existing setting, not an added recovery action.

**Outcome: 1.1.4 observation concluded; persistent ping failure reproduced;
incident OPEN, causal attribution unresolved.** No restart, refresh or flash
was performed by Codex.

Correction to historical 1.1.3 evidence: owner subsequently explicitly
confirmed that music remained audible and correct throughout, including
after manual Bluetooth re-entry; pages stayed inaccessible. Record this as
owner-observed audio PASS while network remains FAIL/OPEN. This confirmation
does not retroactively establish controlled AP testing or stable networking.
