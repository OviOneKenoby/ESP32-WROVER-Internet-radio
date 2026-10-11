# TUNER//01 firmware 1.1.2 Wi-Fi validation report

Status: **PARTIAL hardware validation; recurring Bluetooth-network incident
OPEN**. The original similar 1.1.1 incident also remains **OPEN**.

## Baseline verified before modification

- Hardware-tested firmware: 1.1.1.
- Firmware source commit: `76bdcd13d6628d81e4f96c42534ff781faf8e9db`.
- Embedded build ID: `76bdcd13d662`.
- Reference PR: #4, head `34e01c14e2841c9762d5b5753a0dcb690947b586`.
- Target and dependency pins are unchanged: PlatformIO `esp32-dev`,
  `espressif32@7.0.1`, Arduino-ESP32 package
  `3.20017.241212+sha.dcc1105b`, GxEPD2 1.6.9, Adafruit GFX 1.12.6,
  Adafruit BusIO 1.17.4, Bounce2 2.72.0, ArduinoJson 7.4.3, ESP32-A2DP
  `3245602afc494f9e62160a0cfb2af864af45a37f`, and ESP8266Audio
  `058e131b26e459b9aadcb589a50f07877f1a09fd`.

## Verified code findings

1. `WiFiManager::isConnected()` read only `currentState`; that value was set
   during the initial blocking connection and could remain connected after the
   framework had disconnected. The cached IP was not cleared.
2. A Wi-Fi event handler existed but was never registered. Its disconnected
   event did not capture a reason or synchronize state. `updateStatus()` only
   refreshed RSSI and was not called from the main loop.
3. The installed Arduino-ESP32 2.0.17 source initializes `_autoReconnect` to
   true. Its event callback retries once on the first failure and subsequently
   only for reasons accepted by `_isReconnectableReason()`; an application
   cannot infer a real retry solely from an audio `Unable to reconnect` line.
4. The pinned and vendored HTTP/ICY sources initialize `reconnectTries` to
   zero. The audio task stops after decoder failure and has no scheduled
   station restart. Automatic station resume is intentionally not added here.

## Patch design

- `WiFiManager::begin()` registers the framework callback and creates a
  fixed-size FreeRTOS queue. The callback copies only event type, timestamp and
  disconnect reason. It never logs, connects, writes flash or updates UI.
- `WiFiManager::update()` drains the queue on the application task and then
  reconciles the stored model with `WiFi.status()`. This reconciliation also
  corrects state if an event was dropped. `isConnected()` reads the framework
  directly, and disconnected callers receive `0.0.0.0` instead of a stale IP.
- Event logs occur only on `STA_START`, association, disconnect, `GOT_IP`,
  `LOST_IP` and station stop. RSSI polling does not produce continuous logs.
- `/api/diagnostics` exposes real link/framework status, current IP, RSSI,
  framework auto-reconnect, disconnect reason, event counters/timestamps,
  state age, connected duration and queue-drop count. It does not expose the
  stored Wi-Fi password.
- No supplementary Wi-Fi or stream retry was added. This preserves the
  framework's existing recovery behavior and avoids concurrent `WiFi.begin()`
  calls. No automatic station resume was added.

## Results separated by evidence type

### Hardware results already obtained on 1.1.1

- Normal Radio boot: PASS.
- Three complete audible Radio MP3 ↔ Bluetooth SBC cycles, including web
  access and refresh in Bluetooth: PASS.
- Approximately 2 h 46 min subsequent MP3 playback: PASS observationally;
  free heap 13,036 → 13,028 bytes, largest block 10,228 → 10,740 bytes,
  PSRAM unchanged.
- Controlled AP off/on: connectivity returned without reboot and West City
  started manually. Automatic playback did not resume.

### Software verification for 1.1.2

The clean `esp32-dev` build from firmware source commit
`7cf087f8c68606b9de6fc38ea400c9bf54b1b5d5` passed. It links 83,288 bytes of
static RAM (25.4%) and 2,004,813 bytes of flash (63.7%). The resulting
2,011,392-byte `firmware-1.1.2-7cf087f8c686.bin` embeds build ID
`7cf087f8c686` and has SHA-256
`847C9FE99256F6A63EB19C27543399A1BAC46979D4658951E806D56B15E094EA`.
The uploaded prerelease was downloaded again and matched both byte count and
SHA-256.

The host suite passed all 264 deterministic checks with `-Wall -Wextra
-Werror`. Transition tests cover start, association without IP, GOT_IP,
LOST_IP, disconnect reason/counters, recovery and framework reconciliation
without invented event counts. Static inspection confirmed that no new
`SetReconnect()`, `setAutoReconnect()`, periodic `WiFi.begin()` or Wi-Fi sleep
change was introduced. Full artifact details are in `BUILD_MANIFEST.md` and
`SHA256SUMS.txt`.

### Hardware results on 1.1.2 (`7cf087f8c686`)

- Radio recovery after a router restart: **PASS for the tested topology**.
  Same-SSID RE505X extenders remained available, so this was not a verified
  outage of every AP and was not the planned timed 60-second AP shutdown.
  Three disconnects were recorded (reason 200 `BEACON_TIMEOUT`, then reason 4
  `ASSOC_EXPIRE` twice), each followed by association and, where required,
  `GOT_IP`. At 286,403 ms diagnostics reported connected status, IP
  `192.168.1.129`, three disconnects, three `GOT_IP` events, 11 processed
  events and zero drops. Web/diagnostics recovered, and West City started
  manually without reboot. Automatic station resume was not observed and is
  outside this task.
- Stable network availability during Bluetooth: **FAIL observed in two
  sessions** while SBC playback continued. Web, diagnostics and ping became
  unreachable. In the second session there was one spontaneous recovery:
  `STA_DISCONNECTED` reason 200 at 384,317 ms, association at 385,628 ms and
  `GOT_IP` at 387,854 ms (3,537 ms total). A later persistent outage had no
  further Wi-Fi event in the complete log.
- Persistent outage after leaving Bluetooth: **observed**. West City then
  failed DNS at 651,710 ms despite framework state previously reporting
  connected.
- Controlled router restart during Bluetooth: **NOT TESTED**. The outages
  occurred before that planned test.
- Overall hardware validation: **PARTIAL; incident OPEN**.

### Original incident

**OPEN / cause unknown.** Low internal heap, the observed `BEACON_TIMEOUT`,
and same-SSID extenders are evidence and hypotheses, not independent proof of
root cause. The reset reason `watchdog` belongs to the boot starting the second
log and is not evidence of a watchdog reset during the later outage. The
separate REMOTE device at `192.168.1.117` is not TUNER//01. Firmware 1.1.2
does not expose enough serial-only association/loop detail to resolve the
incident when HTTP is unavailable.

## Preserved scope

Bluetooth lifecycle, AVRCP/SBC, MP3, Browse, Favorites, Recent, saved-station
CRUD/NVS, GPIO, timezone, e-paper behavior and existing assets are unchanged.
AAC and menu latency remain known issues outside this intervention.
