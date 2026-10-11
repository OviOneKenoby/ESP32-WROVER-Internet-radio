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

Owner is preserving the tuner in failure without restart or further refresh.
A later wait-only snapshot with continuous ping is pending. Do not invent
heap recovery, additional events, restart or audio changes.

Correction to historical 1.1.3 evidence: owner subsequently explicitly
confirmed that music remained audible and correct throughout, including
after manual Bluetooth re-entry; pages stayed inaccessible. Record this as
owner-observed audio PASS while network remains FAIL/OPEN. This confirmation
does not retroactively establish controlled AP testing or stable networking.
