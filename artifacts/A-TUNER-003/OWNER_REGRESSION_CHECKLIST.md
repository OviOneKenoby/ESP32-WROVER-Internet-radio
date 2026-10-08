# A-TUNER-003 owner-assisted physical regression checklist

Status at publication: **PENDING / NOT RUN**. The owner performs every flash
and physical action deliberately. This procedure must not be treated as a
PASS until its result table contains dated observations and attached serial,
HTTP and catalog evidence for the exact binary below.

## 1. Artifact and preservation preconditions

1. Download `firmware.bin` from the
   [A-TUNER-003 test prerelease](https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/a-tuner-003-regression-8fac1da).
2. Verify its size is exactly 1,999,408 bytes and SHA-256 is
   `AF066BA215F863A7D2583A6313ACEE00020CCFAD4F8F8C3168DEDA76BCD6F5D4`.
   On PowerShell: `Get-FileHash -Algorithm SHA256 .\firmware.bin`.
3. While the currently installed radio is running, save the existing catalog:
   `curl.exe -sS http://<RADIO-IP>/api/stations -o catalog-before.json`.
   Record `Get-FileHash -Algorithm SHA256 .\catalog-before.json` and keep the
   file outside the project. It contains saved stations and Favorites,
   including their codec fields. It does not contain Wi-Fi credentials or the
   Recent list.
4. Photograph or transcribe the current Recent list if it matters to the
   owner. There is no supported Recent export/import route.
5. Use only sacrificial entries during deletion tests. Saved stations can be
   restored through the web form/API. Favorites can be restored from the
   exported JSON by playing each station with its recorded codec and using
   the playing-screen `+Favorite` action. No NVS erase is allowed.
6. Record the actual serial port selected by the owner. Never assume COM4.

## 2. Owner-controlled app-only flash

Close Serial Monitor, substitute the actual port, and run from a PlatformIO
terminal with the downloaded binary as the current file:

```text
pio pkg exec --package tool-esptoolpy -- esptool.py --chip esp32 --port <ACTUAL_PORT> --baud 460800 write_flash 0x10000 firmware.bin
```

This writes only the `app0` image at `0x10000`; it does not request a flash
erase and does not write the NVS partition at `0x9000`. Record the complete
command/output. After the owner's deliberate reboot, capture the full 115200
baud boot log and GET `/api/diagnostics`. Required identity is firmware
`1.1.0` with `build_git_id` exactly `8fac1da6ccd7`. If the hash, size or build
ID differs, stop: no result belongs to this artifact.

## 3. HTTP DELETE matrix — B01

Before every rejected case, save `/api/stations` to a uniquely named JSON
file. Send the request with `curl.exe -i --path-as-is -X DELETE`, save the
response, GET the list again and compare the before/after files. Test both
`/api/station/` and `/api/favorite/` where an entry exists.

| Suffix/case | Expected HTTP result | Expected catalog result | Observed/status/evidence |
| --- | --- | --- | --- |
| empty suffix | 404 | byte-identical | PENDING |
| `+1` | 404 | byte-identical | PENDING |
| `-1` | 404 | byte-identical | PENDING |
| leading or trailing encoded space | 404 | byte-identical | PENDING |
| `1x` and `x1` | 404 | byte-identical | PENDING |
| `0000` | 404 | byte-identical | PENDING |
| oversized decimal string | 404 | byte-identical | PENDING |
| `255` when count is at most 255 | 404 | byte-identical | PENDING |
| `256` | 404 | byte-identical | PENDING |
| decimal value exactly equal to current count | 404 | byte-identical | PENDING |
| valid zero | 200 | only item zero removed | PENDING |
| valid middle index | 200 | only selected item removed | PENDING |

For spaces use `%20` in the request target. Preserve every raw request target,
status line and JSON pair. A modeled test is not a substitute for this table.

## 4. Saved-list traversal — bounded B04 scope

1. With one sacrificial saved station, play it and press physical and serial
   Next/Previous. Expected: safe wrap to the same item, no reboot/panic.
2. With at least three saved stations, exercise forward/backward and both
   wrap boundaries. Expected: the correct adjacent/wrapped station plays.
3. Play a sacrificial saved station, then delete all saved stations through
   the web manager while the playing screen remains active. Press physical
   and serial Next/Previous. Expected logs are `Next unavailable: saved station
   list is empty or selection is stale` and the corresponding Previous line;
   no playback attempt, divide-by-zero, panic or reboot.
4. Recreate entries, play the last one, delete it while it remains selected,
   then press Next/Previous. Expected: the same stale-selection no-op/log.

Record the complete serial log and actual list before/after. This proves only
empty/stale safety, not durable catalog identity or active-item deletion
semantics.

## 5. Saturating Volume Down — B06

In internet-radio mode, reach volume 5, press Down once and confirm 0; press
Down again and confirm it remains 0. Repeat in Bluetooth mode while connected
and audibly playing. Record serial volume lines, displayed value and whether
audio stays muted. Any jump to 100 is FAIL. Values 1–4 have deterministic
software coverage; record them physically only if an approved input method
can set those exact values without modifying firmware.

## 6. Discovered Favorite codec — B03

Preflight every stream on the test date; unavailable streams are PENDING, not
FAIL and not PASS. Use the playing-screen `+Favorite` action, not a different
add path.

1. Opaque AAC/AAC+: preflight the previously observed KIIS endpoint
   `https://stream.revma.ihrhls.com/zc185`. Play it as AAC, add it from the
   playing screen, save `/api/stations`, and verify that Favorite reports
   `"codec":"AAC"`. Reboot without erasing NVS, replay the Favorite, capture
   `Playing (discovered): ... (AAC)` and confirm audible playback.
2. Opaque MP3: use a currently verified working MP3 URL without `.mp3` or
   `.aac` in its path. Record the exact URL/source evidence. Repeat storage,
   reboot and playback; expect `"codec":"MP3"` and the MP3 path.
3. Explicit case-insensitive AAC: use a currently verified direct `.aac` URL
   (the previously observed Europe 1 endpoint is
   `https://stream.europe1.fr/europe1.aac`; an uppercase-path case requires a
   separately verified server). Confirm AAC storage/reload/playback.

The JSON codec field and serial-selected decoder path prove stored selection;
audible output plus absence of panic establishes only the tested stream/date.

## 7. Representative regression

For the same artifact/date, record PASS/FAIL/PENDING and evidence for HTTP MP3,
HTTPS MP3, HTTPS AAC/AAC+, ICY metadata, Recent play/add, Favorite add/play/
remove, saved-station web management, physical encoder/buttons, e-paper
updates, Wi-Fi reconnect, Bluetooth pairing/audio/AVRCP/metadata, Bluetooth to
radio transition and `/api/diagnostics`. Use working owner-verified streams;
do not invent availability.

## 8. Result and restoration record

| Field | Owner entry |
| --- | --- |
| Test UTC/local date | PENDING |
| Tester | PENDING |
| Actual serial port | PENDING |
| Binary bytes/SHA-256 | PENDING |
| `/api/diagnostics` build ID | PENDING |
| Flash log | PENDING |
| Boot/serial log | PENDING |
| HTTP request/response bundle | PENDING |
| Catalog before/after hashes | PENDING |
| B01 | PENDING |
| B03 | PENDING |
| B04 empty/stale only | PENDING |
| B06 | PENDING |
| Representative regression | PENDING |
| Restoration completed | PENDING |

Restore the saved stations and Favorites from `catalog-before.json`, verify
the final `/api/stations` content against the recorded data, and retain all
evidence. Do not erase NVS, credentials or the entire flash. Full B04 identity,
B02/B05/B07–B11, Native API, shared contract and the common gate remain outside
this physical regression regardless of its result.
