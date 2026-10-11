# TUNER//01 firmware 1.1.3 network instrumentation candidate

Status after owner test on 11 October 2026: **PARTIAL; network access FAIL;
incident OPEN**. Boot/serial work; radio opened a stream; Bluetooth reports
SBC playing, but explicit owner audio confirmation is absent. The outage
survived Bluetooth exit. Controlled AP/router test in BT: **NOT RUN**.
See [owner report](OWNER_REPORT_2026-10-11.md). This build did not eliminate
the recurring Bluetooth-network incident.

## Demonstrated findings

- Entering Bluetooth calls `AudioPlayer::teardownRadioPlayback()` after
  stopping radio I2S. The decoder, buffered source, PSRAM ring buffer,
  AAC storage and HTTP/TLS source are released before A2DP starts. The earlier
  pause log therefore does not prove a concurrent retained radio source.
- Arduino-ESP32 2.0.17 reconnects only after a framework disconnect event and
  only for its accepted reason set. The persistent outage produced no second
  disconnect event, so no application-visible reconnect was triggered.
- The synchronous web JSON paths previously held both a dynamic ArduinoJson
  document and a fully serialized `String`. In Bluetooth, measured largest
  internal blocks were as low as 1,972-6,388 bytes. Direct serialization
  removes the duplicate contiguous output allocation.
- The supplied log contains no crash or boot after the outage. Its `watchdog`
  reset reason describes the boot at the start of the log, not the later
  failure.

## Unresolved hypotheses

Heap pressure/fragmentation, Wi-Fi/Bluetooth coexistence behavior, and roaming
among the router plus same-SSID extenders remain plausible but unproven.
`BEACON_TIMEOUT`, low heap, and router client-list observations do not each
establish root cause. No BSSID evidence was available in firmware 1.1.2.

## Candidate changes

- Serial command `x` prints a bounded, read-only snapshot even when HTTP is
  unavailable.
- Current association identity is obtained from
  `esp_wifi_sta_get_ap_info()`. On failure or disconnection, SSID, BSSID,
  channel and RSSI are explicitly unavailable rather than stale.
- Network address, event, heap, Bluetooth and loop/web/task health fields make
  it possible to distinguish a blocked application loop, a framework-visible
  disconnect, a stale/failed association, and severe allocator pressure.
- `/api/diagnostics` carries the same additional network identity fields while
  JSON is streamed without a duplicate serialized `String`.
- No periodic `WiFi.begin()`, competing reconnect loop, Wi-Fi sleep change,
  automatic reboot, station auto-resume, dependency update, partition change,
  GPIO change or irreversible Bluetooth teardown was introduced.

## Software verification

- Clean `esp32-dev` build: PASS.
- Host correctness suite: PASS, 266 deterministic checks with `-Wall -Wextra
  -Werror`.
- Linked static RAM: 83,336 / 327,680 bytes (25.4%).
- Linked flash: 2,009,881 / 3,145,728 bytes (63.9%).
- Source commit and embedded build ID: see `BUILD_MANIFEST.md`.
- Hardware validation: **PARTIAL**, as recorded in the owner report above.

The validation binary and exact checksum are published in the
[candidate prerelease](https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/tuner-1.1.3-network-validation-603c4b1).
The published binary was downloaded again and matched the local byte count and
SHA-256. Build success is not hardware validation.
