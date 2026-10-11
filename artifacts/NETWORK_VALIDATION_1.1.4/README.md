# TUNER//01 1.1.4 network validation candidate

**Incident OPEN. Hardware validation NOT RUN.** This candidate corrects
observable response-write behavior and collects causal evidence; it does not
claim that persistent IP loss is fixed. No automatic flashing is authorized.

## Repository and evidence

Live check on 11 October 2026: main/base
`112d68a49a053d302a87fc5658d497617eedda88`; PR #5 open, unmerged,
head before this work `459103b89320159897645c491fecf2ee5d74eb0d`, branch
`tuner/wifi-observability`. Its changes after hardware source
`603c4b12a56b83cae712e891c5b1f41b4c158ca9` affect only four documentation
files. All were preserved. The current workspace was an unversioned copy;
work was isolated in a fresh `investigation` checkout. No applicable AGENTS.md
was found in the checkout or workspace ancestor directories inspected.

The corrected 1.1.3 owner report is in
`../NETWORK_VALIDATION_1.1.3/OWNER_REPORT_2026-10-11.md`. Boot/serial work;
radio opened a stream; Bluetooth reports SBC playing. Explicit owner audio
quality confirmation is absent. HTTP and tuner ping failed while serial and
association reports continued; leaving BT recovered heap but not ping.
Controlled Bluetooth AP/router test was NOT RUN. That boot was power_on;
the 1.1.2 watchdog report is a separate historical boot.

## Exact-source findings

The executable files in PR head match the tested source. Inspected APIs are
from the resolved pinned packages, not newer online examples:

- `WebPortal::handleRoot()` uses `send_P`. Arduino-ESP32 2.0.17
  `WebServer::send_P` constructs a dynamic HTTP header String, but streams
  `WEB_PAGE` directly. Its virtual `_currentClientWrite_P` calls
  `WiFiClient::write_P`, which delegates to `write` on memory-mapped ESP32
  flash. **No complete page-sized String copy exists on this root path.**
  Request parsing still allocates Strings/argument arrays; WiFiClient owns a
  lazily allocated 1,436-byte receive buffer and shared socket/RX objects.
- ArduinoJson 7.4.3 defaults to malloc/realloc/free for its pools and strings.
  Its Print writer forwards single bytes and spans to WiFiClient. JSON
  streaming in 1.1.3 removed an output String, but retained document pools,
  parser/header allocations and TCP buffers. sdkconfig's
  `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=4096` means small default allocations
  prefer internal RAM despite several MB of free PSRAM.
- The pinned WiFiClient::write gives each call ten one-second select waits;
  successful partial sends renew its retry count. `setTimeout` does not
  change these constants. Repeated small JSON writes therefore have no
  common response deadline. This is a demonstrated source behavior, not
  proof that it caused the 13,417 ms handler or persistent ping outage.
- `WebPortal::handle` records duration after `handleClient` returns. Serial
  commands are processed later in the same iteration, before loop completion
  is recorded. Snapshot C describes a finished web call in a still-current
  loop iteration; D's increasing loop count rules out a permanently wedged
  main loop in these observations. This timer also includes request parsing,
  waiting and DNS processing; it is not exclusively handler execution.
- `enableBluetooth` stops radio I2S and calls `teardownRadioPlayback` before
  A2DP starts. Decoder, source buffer wrapper, PSRAM ring, AAC storage and
  HTTP/TLS source are deleted/freed. The pause message does not prove a
  retained radio stream during Bluetooth. The reusable I2S output object and
  audio task remain owned by AudioPlayer.
- The pinned A2DP implementation has application task/queue, metadata
  allocations and controller/Bluedroid ownership. Existing checked teardown
  deinitializes profiles and the app task/I2S while retaining base BT state
  for restartability. No irreversible BT memory-release change was made.
  Therefore incomplete heap recovery alone is not proof of a leak.
- Resolved sdkconfig enables classic BT and fixed controller/BTC/BTU tasks,
  Wi-Fi static RX 8/static TX 8/cache TX 16 and dynamic RX 32 buffers; lwIP
  has 16 sockets, 5,760-byte default TCP window/send buffer and a TCP/IP task
  on core 0. Association state is maintained by the Wi-Fi driver; it does
  not test ARP, IP forwarding, TCP buffers or reachability. No Wi-Fi event
  is required for a data-plane failure while still associated. No public
  driver-private allocator ownership/leak is established by these sources.
  lwIP statistics are not enabled in the effective prebuilt configuration;
  changing build flags alone would not instrument that compiled library.

## Candidate changes and limits

- Fixed 512-byte JSON staging buffer replaces socket sends per serialized
  byte. There is no complete serialized output allocation.
- A WebServer subclass overrides the pinned virtual header/body write hooks.
  Raw `send(..., MSG_DONTWAIT)` uses the existing client fd in the same main
  task; no socket is created and there are no concurrent network callbacks.
  All response writes share a 2,000 ms application retry budget from the
  first write; partial progress does not renew it. EAGAIN/EWOULDBLOCK/EINTR
  yield 1 ms; other errors, zero sends or deadline expiry stop further writes
  and close this client. This bounds application retry loops, not scheduler,
  request parsing, Wi-Fi save handlers or arbitrary lwIP internal execution.
  Congested/slow responses can now be truncated instead of tying up the loop
  for multiple independent ten-second write retries. Browser can retry.
- ArduinoJson overflow returns a small 503 error instead of a normal 200
  with a silently incomplete document. Small error/header Strings and request
  parsing allocations remain; no dependency/framework changes were made.
- One fixed-state failed-allocation callback is registered after boot logging.
  It copies scalar count/size/capability data under the ESP32 port critical
  section shared with the snapshot reader. No heap queries, allocation,
  logging, task-name lookup or network actions occur inside the callback.
  Registration result is visible in `x`; zero is ESP_OK. Counters include all
  failed allocation attempts, possibly benign attempts/fallbacks; an increase
  establishes failure occurrence, not the subsystem owner or outage cause.
- Each completed routed response emits one `[WEB_RESPONSE]` line from the
  main task after handler/client cleanup. `x` also retains the last response.
  `heap_start/built/end` are internal free/largest block samples, not minima;
  `alloc_fail` is the cumulative failure count at those phases. `built` is
  sampled at the first header write, including header construction.
  `build_ms`, `header_ms`, `body_ms`, expected/sent bytes, socket calls/waits,
  errno and JSON overflow distinguish generation pressure from output stalls.
  `sent` means bytes accepted by the local socket, **not acknowledged delivery**.
  Failed client capture uses errno (`-1` application deadline, `-2` zero send);
  no extra `connected()` probe is used during failure.

No WiFi.begin loop, sleep change, competing reconnect, reboot, periodic DNS,
auto-resume, BT lifecycle change, GPIO/partition/dependency/AAC/menu/REMOTE
change was introduced. Closing a failed HTTP client is bounded local resource
cleanup; it is not a Wi-Fi recovery strategy or demonstration of root cause.

Open hypotheses: internal allocator pressure/fragmentation, TCP/IP resource
or driver issue, AP/ARP behavior, RF coexistence and other data-plane causes.
Captured unchanged BSSID does not demonstrate roaming. Zero dropped/queued
events only describes the application event queue. Build/host tests cannot
identify the hardware cause or validate audio/network stability.

## Next hardware test — new evidence

Install the distinct prerelease manually using the established board procedure;
verify firmware 1.1.4 and exact build_git_id from the manifest. Keep a complete
115200-baud serial log. Do not overwrite the 1.1.2/1.1.3 artifacts.

1. Boot; issue `x`. Capture idle ping and one request per endpoint using
   `curl.exe --max-time 5 http://192.168.1.129/`, then `/api/status`,
   `/api/stations`, `/api/diagnostics`. Confirm `[WEB_RESPONSE]` expected/sent
   counts and `alloc_hook_result=0`. IP is an example; use the boot address.
2. Open the known MP3 station manually, confirm audible audio. Return to main
   station list with encoder click (`k`) as appropriate; long press (`l`)
   enters BT. Pair/play SBC and explicitly report audio quality. Capture `x`.
3. For two minutes, make **no HTTP requests**; leave PC ping running
   (`ping -t 192.168.1.129`) and capture `x` before/after. This separates a
   Bluetooth-only outage from one requiring web activity.
4. While BT plays, request **each endpoint once, sequentially**, with `x`
   between requests and retain every WEB_RESPONSE line. Fetch root last, then
   one normal browser refresh (which also fetches status/stations). Compare
   allocation count deltas, heap phases, socket errors and byte counts. A
   2-second socket timeout plus continued serial answers is distinct from a
   request-parser stall and from an outage with all socket bytes accepted.
5. If traffic fails, capture `x`, PC `arp -a`, ping output and time before
   leaving BT. If packet capture is already available, retain ARP/ICMP/TCP
   traffic for the tuner and check whether ARP replies disappear. PC gateway
   reachability alone does not prove tuner reachability. Stop BT with `l`,
   capture `x` and ping again; do not assume recovery from heap growth.
6. Only after the above stages, perform the separately labelled controlled
   AP/router restart in BT, retaining BSSID/channel and event sequence. Same
   SSID extenders can keep service available; distinguish router restart from
   all-AP outage. Report exact action and reconnection result. If a spontaneous
   outage prevents this step, mark it NOT RUN rather than PASS.

Serial: `x` diagnostics (read-only); `s` current screen; `k` encoder click;
`l` contextual long press; space play/pause. `l` is not globally a BT toggle
inside browse/favorites screens. No automatic station resume is expected.
