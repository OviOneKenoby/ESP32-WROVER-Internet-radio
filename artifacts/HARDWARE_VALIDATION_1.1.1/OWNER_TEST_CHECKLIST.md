# TUNER//01 owner-assisted Bluetooth regression

Status at publication: **PENDING / NOT RUN**. Do not mark a row PASS from a
successful build, a screen change, pairing alone, or a serial success message.
The executor did not flash or operate the hardware.

## Preconditions

1. Verify `firmware.bin` against `SHA256SUMS.txt` and record its byte count.
2. Flash the app image without erasing NVS, using the owner's actual serial
   port. Do not assume a COM number.
3. Capture the full 115200-baud boot log and GET `/api/diagnostics`. Confirm
   firmware `1.1.1` and the exact build ID from the artifact manifest.
4. Keep the phone available for audible SBC playback. Use a currently working
   MP3 station; stream availability must be checked on the test date.

## A. Boot with no controls pressed

Power-cycle with no hand on Play or the encoder and observe for at least 30
seconds after setup completes.

- Expected: station-select/normal idle behavior; no `Long press detected`, no
  Bluetooth entry, and no A2DP initialization.
- Verify Play/Next/Previous idle at a stable HIGH level. GPIO34/35/39 require
  physical external pull-ups; record resistor values or voltage measurement.
- Repeat once while Play is deliberately held during boot, then release it.
  Expected: no boot-generated long-press command; a new deliberate press after
  release is required.

## B. Three complete Radio MP3 to Bluetooth cycles

Perform all steps three times in the same boot:

1. Start the selected MP3 station and confirm audible radio audio.
2. Enter Bluetooth with the deliberate long press.
3. Connect the phone and play audio long enough to confirm audible SBC output.
4. Exercise Play/Pause, Next and Previous; confirm the phone responds.
5. Exit Bluetooth with the deliberate long press.
6. Confirm the log contains `Bluetooth stop complete` and `restartable=yes`,
   with no A2DP/AVRCP/controller deinit error and no I2S uninstall error.
7. Start the MP3 station again and confirm audible radio audio before the next
   cycle.

| Cycle | MP3 before | BT audible | AVRCP | clean/restartable exit | MP3 after | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 2 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 3 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |

Any `re-start not supported after end(true)`, false success message, missing
audio, deinitialization error, panic or reboot is FAIL for the affected cycle.

## C. Web page and memory in Bluetooth

During actual audible Bluetooth playback in one of the cycles:

1. Open the root page. Confirm the ellipsis in `Loading...` and
   `Choose a common region...`, plus the middle-dot separator, render as their
   intended single glyphs rather than mojibake byte sequences.
2. Load `/api/diagnostics` and save the response. Confirm source `bluetooth`,
   codec `SBC`, lifecycle `active`, and `bluetooth_restartable: true`.
3. Record `free_internal_heap`, `largest_internal_heap_block`,
   `allocator_region_minimum_free_internal_heap`, `free_psram`, and whether the
   web request completed.
4. Record the same fields after each exit in the same radio-stopped state and,
   separately, during each later audible Bluetooth state. Compare like with
   like; do not call an increase/decrease across different states a leak.

| State/action | Free internal | Largest block | Allocator region minimum | Free PSRAM | Result/evidence |
| --- | ---: | ---: | ---: | ---: | --- |
| boot, radio stopped | PENDING | PENDING | PENDING | PENDING | PENDING |
| BT audible + web, cycle 1 | PENDING | PENDING | PENDING | PENDING | PENDING |
| radio stopped, after cycle 1 | PENDING | PENDING | PENDING | PENDING | PENDING |
| BT audible + web, cycle 2 | PENDING | PENDING | PENDING | PENDING | PENDING |
| radio stopped, after cycle 2 | PENDING | PENDING | PENDING | PENDING | PENDING |
| BT audible + web, cycle 3 | PENDING | PENDING | PENDING | PENDING | PENDING |
| radio stopped, after cycle 3 | PENDING | PENDING | PENDING | PENDING | PENDING |

## D. Preserved-function smoke test

After the cycles, check one successful path each for MP3, Browse Countries and
Tags, Favorites, Recent, saved-station web CRUD with persistence after reboot,
Next/Previous on saved stations, Bluetooth metadata, and e-paper controls.
AAC and menu latency remain known issues; record observations without changing
their scope.

## Result record

| Field | Owner result |
| --- | --- |
| Test local/UTC date | PENDING |
| Tester | PENDING |
| Binary bytes/SHA-256 | PENDING |
| Diagnostics build ID | PENDING |
| Boot untouched | PENDING |
| Three audible cycles | PENDING |
| Web in Bluetooth | PENDING |
| Same-state memory comparison | PENDING |
| Preserved-function smoke test | PENDING |
| Overall hardware result | PENDING |
