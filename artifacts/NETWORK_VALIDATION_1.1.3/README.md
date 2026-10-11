# TUNER//01 firmware 1.1.3 network instrumentation candidate

Status: **software candidate; hardware validation PENDING**. This build does
not claim that the recurring Bluetooth-network incident is fixed.

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

Build identity, resource usage, binary checksum and download URL are recorded
in `BUILD_MANIFEST.md` after the clean identified build.
