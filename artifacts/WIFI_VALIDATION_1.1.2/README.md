# TUNER//01 firmware 1.1.2 Wi-Fi validation report

Status: **software implementation under verification; new physical regression
PENDING**. The original uncontrolled connectivity incident remains **OPEN**.

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

### New hardware tests

**PENDING / NOT RUN.** Use `OWNER_TEST_CHECKLIST.md`. A build, screen change,
or one successful reconnect is not sufficient to close the incident.

### Original incident

**OPEN / cause unknown.** The first session's loss of router visibility,
web/diagnostics, DNS and Radio connectivity has not been reproduced with
enough diagnostics to assign a cause. This patch makes a recurrence observable
but does not claim to eliminate an unidentified cause.

## Preserved scope

Bluetooth lifecycle, AVRCP/SBC, MP3, Browse, Favorites, Recent, saved-station
CRUD/NVS, GPIO, timezone, e-paper behavior and existing assets are unchanged.
AAC and menu latency remain known issues outside this intervention.
