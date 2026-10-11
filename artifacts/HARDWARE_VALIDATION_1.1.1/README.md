# TUNER//01 firmware 1.1.1 validation report

Status: **software verification complete; tested Bluetooth lifecycle/audio/web
paths PASS by owner report; incomplete checklist items remain NOT TESTED and
the separate Wi-Fi incident remains OPEN**.

This report corrects the Bluetooth-cycle conclusion from the earlier 1.1.0
hardware session. The observed `re-start not supported after end(true)` line
proves that the later Bluetooth entries in that session did not start. A mode
screen change is not evidence of an active A2DP sink. The general Bluetooth
Radio-to-Bluetooth cycle PASS is therefore withdrawn.

## Owner hardware results reported 2026-10-11

The owner identified the tested image as firmware 1.1.1 with `build_git_id`
`76bdcd13d662`:

- Boot with no controls pressed stayed in Radio without automatic Bluetooth:
  **PASS**.
- Three complete audible Radio MP3 → Bluetooth SBC from the phone → Radio MP3
  cycles, without reboot: **PASS**.
- Web page access and refresh during audible Bluetooth playback, with no audio
  problem reported: **PASS**. Exact UTF-8 symbol rendering was not explicitly
  checked and remains **NOT TESTED**.
- West City MP3 continued for approximately 2 h 46 min after the cycles:
  **PASS observationally**.
- Free internal heap after cycles / after extended playback: 13,036 / 13,028
  bytes. Largest block: 10,228 / 10,740 bytes. Free PSRAM: 4,144,135 bytes in
  both captures. These two Radio-session observations do not replace the
  checklist's unrecorded same-state Bluetooth samples.
- A controlled AP interruption recovered connectivity without reboot and West
  City started manually: **PASS for that controlled scenario**. Automatic
  station playback did not resume and was not part of this firmware.

AVRCP operation on 1.1.1, boot with Play deliberately held, exact UTF-8 glyph
rendering, and the remaining unreported smoke-test rows are **NOT TESTED / no
evidence supplied**.

The first session's connectivity incident remains **OPEN / cause unknown**.
During cycle 3 Bluetooth audio continued, but the tuner disappeared from the
router's active clients; web and diagnostics stopped responding; returning to
Radio produced DNS/play failures; the encoder still responded; and reboot
restored operation. The successful retest does not close this incident.

The later Wi-Fi state/diagnostics candidate is tracked separately under
`artifacts/WIFI_VALIDATION_1.1.2/` and does not alter these 1.1.1 results.

## Verified baseline

- Reported failing firmware source: `8fac1da6ccd749a47843d99dc746fc3d6e4bf19b`.
- Repair branch base: coordination `origin/main` at
  `cb17e945b9c0e086a03e7176d9ad442a8c78d89b`.
- Target: PlatformIO `esp32-dev`, `espressif32@7.0.1`, Arduino framework,
  `esp32dev`, 240 MHz CPU, 80 MHz DIO flash, QIO/QSPI PSRAM configuration and
  framework `huge_app.csv`.
- Framework package: `framework-arduinoespressif32`
  `3.20017.241212+sha.dcc1105b` (Arduino-ESP32 2.0.17 / ESP-IDF 4.4 family).
- Toolchain: Xtensa `8.4.0+2021r2-patch5`.
- Dependencies were resolved exactly from `platformio.ini`; none was updated:
  GxEPD2 1.6.9, Adafruit GFX 1.12.6, Adafruit BusIO 1.17.4,
  Bounce2 2.72.0, ArduinoJson 7.4.3, ESP32-A2DP
  `3245602afc494f9e62160a0cfb2af864af45a37f`, and ESP8266Audio
  `058e131b26e459b9aadcb589a50f07877f1a09fd`.

## Verified causes and corrections

### 1. Bluetooth could not restart, while the application claimed success

`src/audio.cpp` called `BluetoothA2DPSink::end(true)`. In the pinned library,
`BluetoothA2DPCommon::end(bool)` disables/deinitializes Bluedroid and the
controller, calls `esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)`, and
sets `is_start_disabled = true`. `BluetoothA2DPSink::start()` checks that flag,
logs `re-start not supported after end(true)`, and returns. ESP-IDF's installed
`esp_bt.h` independently documents that released controller memory cannot be
reclaimed until reboot.

The application ignored this because the pinned library's `start()` returns
`void`; `AudioPlayer::enableBluetooth()` then unconditionally set
`btEnabled`, source, codec and playback state and `main.cpp` unconditionally
changed the UI mode.

The repair uses the library's restartable `end(false)` path. That path still
disconnects and deinitializes A2DP/AVRCP, stops its queue/task, and ends the
legacy I2S output, but deliberately retains the controller/Bluedroid base
allocation needed for the next start. It is not equivalent to the old
stream-only `stop()` path.

`src/audio.cpp` now wraps the pinned output/sink without changing the
dependency. It records the real I2S `begin()`/uninstall result and checks
Bluetooth initialization, controller state, Bluedroid state, task/queue state,
I2S release and the library's restart-disabled flag. `enableBluetooth()` and
`disableBluetooth()` return success/failure. `src/main.cpp` changes mode only
on success. A partial start is cleaned; an incomplete cleanup is quarantined
as `cleanup_failed`, blocks radio-mode handoff, and asks for reboot instead of
claiming success.

The three audible cycles were later performed successfully as recorded above.
Retaining the base Bluetooth stack changes the post-exit heap budget compared
with irreversible `end(true)`.

### 2. Uncommanded Bluetooth entry at boot

The installed Bounce2 2.72 source defines `Button::stateForPressed = 1`
(HIGH) unless `setPressedState()` is called. The firmware treated falling
edges as active-low presses but used `buttonPlay.isPressed()` for the long
press without configuring its pressed polarity. This inconsistent software
polarity is a verified defect.

`src/input.cpp` now sets every button's pressed state to LOW and uses
`pressed()` consistently. A Play button already low at initialization must be
released once before long-press detection is armed, so a boot-held/stuck input
cannot issue the Bluetooth command.

Hardware remains a separate concern: the active Play/Next/Previous pins are
GPIO34/35/39. The classic ESP32 documentation states that input-only
GPIO34-39 do not have software-controlled pull-up or pull-down resistors.
`INPUT_PULLUP` was therefore ineffective. The firmware now declares these as
plain `INPUT`; each assembled button requires a real external pull-up to 3.3 V
and a reliable active-low contact to ground. Imperfect contacts or a missing
pull-up can still generate real electrical lows and must not be classified as
a software failure without measurement.

### 3. Web text mojibake

The source contains valid UTF-8 for the ellipsis and middle-dot characters,
but the root handler sent only `text/html`, and the document had no charset
declaration. A browser choosing Windows-1252 renders the UTF-8 bytes as the
reported `â€¦` and `Â·` sequences.

`src/web_portal.cpp` now sends `text/html; charset=utf-8`, includes
`<meta charset=utf-8>`, and uses `&hellip;`/`&middot;` for these UI symbols.

### 4. Meaning of `minimum_free_internal_heap`

The installed ESP-IDF header defines
`heap_caps_get_minimum_free_size(caps)` as the **sum of per-region low-water
marks for all currently registered matching regions**. It also warns that the
individual regions can reach their minima at different times. It is not one
process-wide, immutable low-water counter. When a heap region is added (the
old irreversible Bluetooth memory-release path is one example), the aggregate
can increase in the same boot.

The existing JSON key is retained for compatibility. `/api/diagnostics` now
also returns `allocator_region_minimum_free_internal_heap` and
`minimum_free_internal_heap_scope: "sum_of_current_region_low_watermarks"`.
Bluetooth lifecycle and restartability are also reported. Leak comparisons
must use free heap, largest block and PSRAM in the same operational state and
after the same actions; the earlier different-state samples neither prove nor
disprove a leak.

## Preserved scope and exclusions

The patch does not change station storage, Favorites, Recent, Radio Browser,
MP3 decoding, AVRCP commands/metadata, display refresh policy, partitions,
pin assignments, or dependency versions. AAC stability and slow menus during
playback remain known issues and were not expanded into this repair.

## Software verification

The clean `esp32-dev` build from source commit
`76bdcd13d6628d81e4f96c42534ff781faf8e9db` passed. It links 83,160 bytes of
static RAM (25.4%) and 1,997,689 bytes of flash (63.5%). The resulting
2,004,272-byte `firmware-1.1.1-76bdcd13d662.bin` embeds build ID
`76bdcd13d662` and has SHA-256
`A7BDCA919E426EBD1F6777385AFF64E7886E009F31B6B5D9BB636E198BC1D59A`.
It is published as a non-production
[hardware-validation prerelease](https://github.com/OviOneKenoby/ESP32-WROVER-Internet-radio/releases/tag/tuner-1.1.1-validation-76bdcd1).
The existing host suite passed all 249 deterministic checks; targeted static
assertions for this repair also passed. The final teardown directly checks the
AVRCP controller/target and A2DP sink deinitialization results before allowing
the Radio handoff. Full details are in `BUILD_MANIFEST.md` and
`SHA256SUMS.txt`.

Compilation itself is not a hardware PASS. The owner results and explicitly
untested rows are recorded above and in `OWNER_TEST_CHECKLIST.md`.
