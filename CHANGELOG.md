# Changelog — What Was Actually Wrong and Fixed

## 2026-08-18 — Synchronize internet-radio metadata snapshots

Confirmed issue: the Core 1 internet-radio decode task wrote ICY
`StreamTitle` metadata directly into `nowPlaying` while the UI task read the
same mutable buffer. The public API now copies a protected snapshot into a
caller-owned buffer; the metadata callback uses its own dedicated critical
section and never takes `audioMutex`, avoiding a decoder-callback deadlock.
Hardware playback validation is still required.

## 2026-08-17 — Document AI collaboration

The README now transparently identifies that the firmware and documentation
were developed collaboratively with OpenAI's ChatGPT/Codex, while the project
owner supplied the hardware, requirements, flashing, and real-device testing.

## 2026-08-17 — Non-blocking NTP idle clock

When the radio is stopped on the main station screen for one minute, it now
shows an idle clock with Europe/Bucharest local date and time. SNTP setup is
requested asynchronously after Wi-Fi connects, so it never blocks audio, UI,
or the web server. The minute changes through a small partial refresh; any
physical or serial control wakes the regular station screen without also
executing that control's original action.

The web manager now saves the installed location's POSIX timezone rule in the
`time` NVS namespace. It offers common North American, European, Japanese,
and Australian presets plus a custom rule field, so the device is no longer
hard-wired to Bucharest time. The default remains Bucharest until changed.

Follow-up correction: SNTP now starts with `configTzTime()` rather than the
UTC-offset `configTime(0, 0, ...)` overload. The latter silently replaced the
saved timezone with UTC, so a Romania-configured idle clock was three hours
behind during EEST. The selected POSIX rule is now preserved through NTP sync.

## 2026-08-17 — Align README with the current hardware and features

The README now identifies the ESP32-WROVER-DEV target and its actual wiring:
GPIO19/21 for e-paper reset/data-command and GPIO27 for encoder clock.
It explains that GPIO16/17 are reserved for WROVER PSRAM, documents the local
web manager and AVRCP controls, and removes dropped SD card, recording,
equalizer, and redundant preset ideas from the future list.

## 2026-08-14 — Local web configuration and NVS station manager

The radio now serves a local management page on its Wi-Fi address. It shows
connection status, saves Wi-Fi credentials, and can add/delete the separate
user station list and remove Favorites. User stations are persisted in the
`stations` NVS namespace; Favorites and Recent retain their existing,
separate NVS namespaces.

On a device with no web-added stations yet, that namespace is now created
quietly at startup rather than producing Preferences' misleading `NOT_FOUND`
diagnostic; it does not modify the existing Favorite or Recent namespaces.

If both the saved Wi-Fi connection and configured fallback fail, the device
starts a WPA2 setup AP named `ESP32-Radio-Setup-XXXX`. Its password is derived
from the device MAC (rather than committed as a shared password) and printed
to the serial monitor and shown on the e-paper. Join that AP and open
`http://192.168.4.1/` to enter new Wi-Fi credentials.

## 2026-08-14 — E-paper refresh policy and Bluetooth AVRCP metadata

The existing partial full-screen redraws now automatically use a full panel
refresh every 12 redraws, limiting e-paper ghosting while preserving fast,
non-flashing encoder updates. The existing header and Now Playing tickers
remain timer-driven partial-region updates, so they do not block audio.

Bluetooth mode now requests AVRCP Title and Artist metadata, displays the
combined artist/title through the existing ticker, and maps encoder click or
Play/Pause to phone play/pause; Next/Previous send AVRCP track commands.
Long press still leaves Bluetooth mode. Metadata is copied under a critical
section before the UI reads it, avoiding an asynchronous callback race.

Leaving Bluetooth now calls the A2DP library's `end(true)` rather than its
stream-only `stop()`. This releases the Classic Bluetooth/AVRCP memory that
otherwise left too little contiguous internal heap for the next HTTPS station
directory request. The serial log reports free and largest heap blocks before
and after the teardown for hardware verification.

## 2026-08-14 — Browse stations by name letter within a country

After choosing a country, the browser now presents `Station Letter` with
`Popular Stations` followed by A–Z. `Popular Stations` preserves the existing
country/genre popularity search. Choosing a letter adds a server-side station
name filter and then retains only names beginning with that letter before
showing the results. This makes a station such as Romania's `West City Radio`
discoverable under `W` without loading the country's entire directory.

The Radio Browser API's name filter is a substring search, not a prefix
search, which was verified directly. The local first-character filter is
therefore required to keep the display's A–Z classification truthful.

The serial `s` screen dump now also recognizes the new `Station Letter` mode;
without that case it printed `(unknown mode)` immediately after choosing a
country even though the e-paper menu had been drawn.

## 2026-08-14 — Allow slow-starting HTTP Icecast streams

DIGI FM (`http://edge76.rdsnet.ro:84/digifm/digifm.mp3`) was checked directly
and is a healthy, continuous `audio/mpeg` Icecast stream. On the ESP32 it
reported `No stream data available` then repeated disconnect messages because
the pinned upstream HTTP ICY reader gave the server only 500 ms to produce
its first audio bytes. The checked build patch changes only that exact
first-byte wait to 3 seconds, matching this project's existing connection
timeout. It aborts on an unexpected library source rather than silently
patching a different version.

An empty `StreamTitle` no longer replaces the `Live stream` fallback with a
blank title.

## 2026-08-14 — Trim KIIS machine metadata from the Now Playing title

102.7 KIIS FM sends its ICY title as a readable prefix followed by a
`text="…"` field and many machine-only fields (`song_spot`, IDs, artwork URL,
and more). The application previously displayed that whole payload. Stream
title handling now recognizes this format and displays only the useful
`prefix - text` portion (for example, `Olivia Rodrigo - Stupid Song`), while
leaving normal ICY titles unchanged.

## 2026-08-14 — Show a truthful fallback when track metadata is absent

Europe 1 plays audio but explicitly sends `icy-metaint: 0`, so it cannot
provide individual track titles. The UI showed the inherited placeholder
`Now playing: No stream`, which incorrectly suggested a playback failure.
The fallback is now `Live stream`; real `StreamTitle` metadata still replaces
it when a station supplies it. The initial discovered-station display also
uses the protected local station-name copy, consistent with the Recent-list
pointer fix.

## 2026-08-14 — Read chunked Europe 1 stream as raw AAC

Europe 1 opened successfully but repeatedly reported `No stream data
available`. The public endpoint was checked directly: it redirects to
`europe1.lmn.fm`, which sends `Content-Type: audio/aac` using HTTP/1.1
`Transfer-Encoding: chunked`. The project's ICY source reads from
`HTTPClient`'s underlying stream, so it requires a continuous audio body,
not HTTP chunk framing.

The HTTPS/ICY source now requests HTTP/1.0 before its GET. The same endpoint
then sends a continuous raw AAC stream with `Connection: close`; redirect
handling and ICY metadata requests remain enabled. This change applies only
to HTTPS radio streams using the project’s direct-stream reader.

## 2026-08-14 — Preserve the selected station while updating Recent

Selecting an entry from Recent could play a different station. The click
handler passed pointers into the Recent array to `playDiscoveredStation()`,
which immediately called `addToRecent()`. That function shifts the same array
to put the selected entry first, invalidating the incoming pointers before the
log, favorite check, and `audioPlayer.play()` used them. The observed result
was selecting Europe 1 while the log and connection attempt named 102.7 KIIS
FM.

`playDiscoveredStation()` now copies the selected name and URL into local,
null-terminated buffers before it modifies Recent. The copied values are used
for every later operation, so Browse, Favorites, and Recent all retain the
station the user actually chose.

## 2026-08-14 — Correct saved AAC codec and reject invalid MP3 clock data

The new hardware trace was an `IntegerDivideByZero` in ESP-IDF's
`i2s_set_clk()`, called from `AudioGeneratorMP3a::loop()`. This was not the
previous decoder-lifetime race. The saved Europe 1 entry announced itself as
MP3 while its saved stream URL is `https://stream.europe1.fr/europe1.aac`.
The MP3 decoder was therefore fed AAC data and supplied a zero sample rate to
the legacy I2S driver, which divides by the requested rate.

Saved Favorites/Recent entries, and newly added entries, now treat a URL
containing `.aac` as AAC. This is a deliberately narrow migration for
unambiguous AAC URLs; all other stored/API codec values are preserved. It
makes already-saved Europe 1 entries play through the AAC decoder immediately
after reboot, and the next playback re-saves the corrected Recent entry.

The checked ESP8266Audio pre-build patch also rejects a decoded MP3 frame with
zero rate or channel count before it reaches `AudioOutputI2S::SetRate()`. That
does not make a wrongly labelled stream playable, but ensures it ends cleanly
instead of resetting the ESP32. The patch validates the exact expected pinned
source before changing it.

## 2026-08-14 — AAC+/SBR PSRAM allocation

The AAC path was compiled and inspected against the exact resolved
`ESP8266Audio` dependency (`058e131b26e459b9aadcb589a50f07877f1a09fd`).
`AudioGeneratorAAC`'s default constructor allocates its Helix decoder state
from the ordinary heap; that includes the SBR state allocation which reports
as roughly 50KB when it fails. This is the direct cause of the previously
recorded `OOM in SBR` failure, and moving only the 16KB stream ring buffer did
not change where that allocation occurs.

`AudioGeneratorAAC` provides a verified preallocated-memory constructor. The
AAC path now reserves a 96KB block with `ps_malloc()` and passes it to that
constructor, so the decoder's input buffer, PCM buffer, base state, and SBR
state live in PSRAM. If that block is unavailable, playback fails cleanly with
a serial message rather than falling back to internal heap and risking the
known crash. MP3 playback is unchanged.

Validated with PlatformIO `espressif32@7.0.1` / Arduino ESP32 framework
`3.20017.241212`: the `esp32-dev` build completed successfully (flash:
1,927,261 / 3,145,728 bytes; RAM: 80,420 / 327,680 bytes). Hardware playback
still needs to be checked on the WROVER board with an AAC+ stream.

## 2026-08-14 — AAC+/SBR output-buffer corruption fix

Hardware testing of Europe 1 (`https://stream.europe1.fr/europe1.aac`) exposed
a second, independent AAC+ issue: a Core 1 `LoadStoreError` in
`raac_QMFSynthesisConv` / `raac_DecodeSBRData`. The stack trace is the known
non-ESP8266 `AudioGeneratorAAC` SBR buffer-overflow path. The pinned library
allocated space for 1024 stereo PCM samples, but SBR produces 2048 samples per
channel and writes 4096 interleaved samples.

The precise upstream fix is commit `05f2fb0045cc294b4e0d1a1a9747b89c22c1fea4`
(`Fix memory corruption in AudioGeneratorAAC on non-ESP8266 platforms`). This
project retains its pinned pre-IDF-6 library revision for I2S compatibility and
uses a checked pre-build script to apply only that upstream AAC buffer fix.
The PSRAM reservation is increased to 128KB to cover the corrected 8KB AAC+
PCM buffer and all decoder state. The script aborts the build if the expected
pinned library source is not present, rather than silently patching an unknown
version.

## 2026-08-14 — Playback-chain race fix

Selecting another station while the previous HTTPS stream was stalled caused an
`IntegerDivideByZero` in `AudioGeneratorMP3a::loop()`. The backtrace showed the
audio task running decoder code concurrently with the main task replacing the
decoder chain. `play()` locked its teardown, but the audio task did not take
that same lock before reading or calling `mp3Decoder`; it could therefore use a
decoder/source object after the main task had deleted it. The normal stop and
Bluetooth handoff had the same teardown gap.

The audio task now holds `audioMutex` while it checks and drives the decoder;
`stop()` and `enableBluetooth()` use the same mutex while tearing down the
chain. This serializes access to the decoder, stream sources, and PSRAM buffers
and prevents use-after-free while changing stations. The separate HTTPS
chunked-stream issue remains tracked independently.

This replaces several earlier documents that made confident claims which turned out
to be wrong. This one only lists things that were confirmed against either the real
compiler output you pasted back, or the actual library source code fetched directly
from GitHub.

## Fixed, in the order they were found

1. **Bluetooth library package name** — `pschatzmann/ESP32-A2DP` isn't listed in
   PlatformIO's registry under any name/version tried. Fixed by installing it directly
   from GitHub (`https://github.com/pschatzmann/ESP32-A2DP.git`), which clones it
   rather than looking it up in the registry.

2. **Version pins** — `platformio.ini` no longer pins exact version numbers for
   `GxEPD2`, `Adafruit GFX Library`, `Bounce2`, or `ArduinoJson`. Every specific
   version I typed from memory in earlier rounds turned out to be wrong. Leaving
   them unpinned lets PlatformIO fetch whatever's currently published.

3. **`display.h` include path** — `GxEPD2_154_D67.h` is real, but lives in the
   library's `epd/` subdirectory, which isn't on the include path by itself (only
   the library's top-level `src/` is). Confirmed by cloning the actual GxEPD2 repo
   at the resolved version (1.6.9) and testing the include path directly with `g++`.
   Fixed: `#include <epd/GxEPD2_154_D67.h>`.

4. **`stations.h` / `net_manager.h` missing `config.h`** — both used macros
   (`MAX_STATIONS`, `MAX_SSID_LENGTH`, etc.) defined in `config.h` without including
   it themselves, relying on the `.cpp` file happening to include `config.h` first.
   Fixed by having each header include `config.h` directly.

5. **`input.cpp` Bounce2 API** — `previousDebouncedTime()` isn't a real method on
   this library's `Button` class. Replaced with `currentDuration()`, confirmed via
   the compiler's own suggested-alternative output.

6. **`i2s_mode_t` cast** — `I2S_MODE_MASTER | I2S_MODE_TX` produces a plain `int` in
   C++ (unlike C), which won't implicitly convert into an enum-typed struct field.
   Added an explicit cast.

7. **`i2s_pin_config_t` designator order** — C++ requires designated initializers to
   appear in the same order as the struct's actual field declarations. `mck_io_num`
   is declared first in this struct; it was listed last. Reordered.

8. **`wifi.h` filename collision with the framework's `WiFi.h`** — on a
   case-insensitive filesystem (Windows), `#include <WiFi.h>` inside a file named
   `wifi.h` can resolve back to itself, silently preventing the real library header
   from ever loading. Renamed the file to `net_manager.h` / `net_manager.cpp` so it
   can't collide.

9. **Stale leftover files** — after the rename above, the *old* `wifi.h`/`wifi.cpp`
   were still present in the project folder alongside the new `net_manager.*` files
   (both got copied over rather than the old ones being removed), causing duplicate
   class/enum definitions. Confirmed directly from the build log, which showed both
   `wifi.cpp.o` and `net_manager.cpp.o` being compiled. This download has only the
   new files — the old `wifi.h`/`wifi.cpp` don't exist in this package at all.

10. **Unverified 16MB flash assumption** — `platformio.ini` had been declaring
    `board_build.flash_size = 16mb` since the very first draft, without ever
    confirming that against real hardware. Your build log's own board info reported
    `4MB Flash` as this board's default. Removed the override entirely rather than
    risk a confusing upload-time failure later — `huge_app.csv` (the partition table
    in use) is specifically sized for 4MB flash anyway.

11. **`display.h` missing `config.h`** — same bug class as #4, just in a file that
    hadn't compiled far enough yet to reveal it: `display.cpp` uses `EPD_CS_PIN` /
    `EPD_DC_PIN` / `EPD_RES_PIN` / `EPD_BUSY_PIN` from `config.h` without it being
    included anywhere in the chain. After finding this, every other file was
    systematically checked for the same pattern (grep every config.h macro against
    every file that uses it) rather than waiting to hit each one individually -
    confirmed no other instances remain anywhere in the project.

12. **`epd.powerOn()` doesn't exist** — confirmed against the real GxEPD2 1.6.9
    source: the library only has `powerOff()` and `hibernate()`, no `powerOn()`.
    The documented way to resume after `powerOff()` (which just cuts driving
    voltage, doesn't hibernate the controller) is calling `init(bitrate, false)`
    again - the `false` means "no full reset needed, power was kept," which is
    exactly this scenario. This function wasn't called anywhere else in the
    project yet, so the fix carries no other side effects.

13. **`GxEPD2_BW` constructor signature** — `epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RES_PIN,
    EPD_BUSY_PIN)` was passing 4 raw pin numbers straight to the `GxEPD2_BW<>`
    wrapper, but that wrapper only takes ONE argument: an already-constructed
    `GxEPD2_154_D67` driver instance. The 4 pins belong to that inner driver
    class's constructor, not the wrapper's. Fixed to
    `epd(GxEPD2_154_D67(EPD_CS_PIN, EPD_DC_PIN, EPD_RES_PIN, EPD_BUSY_PIN))`.
    The compiler's own error output listed the correct 1-argument constructor
    signature directly, which is what this fix is matched against.

14. **ESP32-A2DP library: `'min' was not declared in this scope`** — this is what
    the `#warning "AudioTools library is not included first or installed"` (visible
    in every log since round 1, previously assumed benign) was actually about.
    The library's `writeSilence()` helper calls a bare, unqualified `min()`,
    expecting it to exist globally - normally supplied either by the optional
    AudioTools library, or by a legacy ESP-IDF<5.0 code path. Neither applies here
    (no AudioTools installed; this core's ESP-IDF is 5.x). Confirmed by cloning the
    exact resolved commit (195258d) of the ESP32-A2DP repo and reading
    `BluetoothA2DPOutput.h`/`config.h` directly. Fixed with a small header
    (`src/compat_fixes.h`, `using std::min; using std::max;`) force-included into
    every translation unit via `-include` in `platformio.ini`'s `build_flags`
    (which PlatformIO applies to library sources too, not just project sources) -
    this is the same technique the real Arduino-ESP32 core itself uses internally
    (confirmed directly in `cores/esp32/Arduino.h`), just made available to the one
    library that doesn't happen to pull in that chain.

15. **`compat_fixes.h` broke plain C files** — `-include` applies to *every* file
    PlatformIO compiles, not just C++ ones. `glcdfont.c` (font data in Adafruit
    GFX Library) is plain C, and `<algorithm>`/`std::min`/`std::max` don't exist
    in C at all, so the fix from #14 broke C compilation the moment it reached a
    `.c` file. Wrapped the header's contents in `#ifdef __cplusplus ... #endif`,
    the standard guard for headers force-included into mixed C/C++ builds - this
    makes it a no-op for any C file anywhere in the dependency tree, not just
    `glcdfont.c` specifically.

16. **Linker error: `undefined reference to InputControl::encoderISR()`** — this
    was a real logic bug, not a missing definition. `input.h` declared
    `encoderISR()` as a *class member*. `input.cpp` only ever defined a
    *free function* of the same name at file scope. Inside
    `InputControl::init()`, the call `attachInterrupt(..., encoderISR, ...)` is
    itself inside a member function, so C++ name lookup resolves the unqualified
    `encoderISR` to the class's own member first (member lookup takes priority
    over the enclosing namespace) - not the free function, even though the free
    function was fully defined. The member was only ever declared, so the linker
    had nothing to point to.
    A second bug was hiding underneath: `encoderLastState` existed as *both* a
    file-scope global (written by the free-function ISR) *and* a class member
    (read by `update()`) - two different variables sharing a name, silently
    disconnected. Even patching just the linker error would have left the encoder
    non-functional. Fixed by consolidating the encoder state into `inline static`
    class members (C++17) that both the real `InputControl::encoderISR()`
    definition and `update()` share, and removing the disconnected free
    function/globals entirely. Verified the `inline static` member + static
    member function pattern actually compiles and links with a standalone g++
    reproduction before shipping this fix.

## What's in this download

Exactly these files, and nothing stale:

```
InternetRadio_ESP32_EPaper/
├── platformio.ini
├── README.md
├── QUICK_START.md
├── COMPILATION_GUIDE.md
├── READ_THIS_FIRST.md          - how to self-verify library versions if needed
├── CHANGELOG.md                 - this file
└── src/
    ├── main.cpp
    ├── config.h
    ├── display.h / display.cpp
    ├── audio.h / audio.cpp
    ├── input.h / input.cpp
    ├── net_manager.h / net_manager.cpp   (formerly wifi.h/wifi.cpp)
    └── stations.h / stations.cpp
```

No `.pio` build cache is included — delete any existing `.pio` folder in your own
project directory before building, since a stale cache was part of what caused
confusion in earlier rounds too.

## To build

```bash
rm -rf .pio
cd InternetRadio_ESP32_EPaper
pio run -e esp32-dev -t upload
```

---

# Post-Flash Functional Fixes

The firmware compiled and flashed successfully. These are real functional bugs
found once it was actually running on hardware - a different category from the
compile errors above, but genuine bugs nonetheless.

1. **Garbled/"weird characters" on the station list** — `showStationList()` was
   being called with `(const char**)new char*[count]` in three places. This
   allocates an array of `count` completely uninitialized pointers and never
   fills them with anything - it has no connection to the actual station names
   at all. The display was reading whatever garbage happened to be at those
   pointer addresses as if it were text. Replaced all three call sites with a
   `refreshStationListDisplay()` helper that actually populates the array from
   `stationManager.getStationName()`.

2. **"Failed to play station"** — two separate bugs:
   - Two of the six preset station URLs were the wrong *format* for what this
     firmware can read (an HLS `.m3u8` playlist for BBC, a `.pls` playlist file
     for NPR). This code only reads raw audio bytes over plain HTTP - it doesn't
     parse either playlist format. Replaced both with direct-MP3-stream URLs.
   - Separately, `connectToStream()` never extracted a port from URLs like
     `http://host:8000/path` - the `:8000` stayed stuck inside the hostname
     string passed to `wifiClient.connect()`, which can't resolve a hostname
     with a port glued onto it. Fixed to split the port out properly. This
     affected the WFUV station regardless of which station list was in use.
   - Important limitation either way: this firmware cannot connect to HTTPS
     streams at all (`WiFiClient`, not `WiFiClientSecure`) or parse HLS/.pls/.m3u
     playlists. I have no network path from this environment to actually test
     live connectivity to any streaming server, so I can't guarantee any given
     station URL is currently live - only that its *format* is one this code
     can handle. If a station still fails, open its URL directly in VLC
     (Media > Open Network Stream) or a browser first to check whether it's a
     dead URL versus a firmware issue.

3. **Error screen loops instead of recovering** — there was no error *mode* at
   all, only error *graphics*. `showError()` drew text but never changed
   `currentMode`, so the app silently stayed in `MODE_STATION_SELECT`. Pressing
   anything on the error screen re-ran station-select input handling, which (for
   Play/Click) immediately retried the same failing station and drew the same
   error again - that's the loop. Added a real `MODE_ERROR` state and
   `handleErrorInput()`: any input now just returns to the station list without
   retrying anything.

4. **Encoder rotation showed no visible feedback** — `handleStationSelectInput()`
   updated the internal `selectedStation` variable on encoder rotation but never
   redrew the screen to show the new selection - you'd only see anything once
   you clicked into a station. Fixed to redraw after every scroll step. This is
   a real bug independent of any hardware cause, though see the hardware note
   below too.

5. **Encoder not registering rotation at all (hardware note, not a code bug)** —
   GPIO 36 (CLK/S1 in this firmware's wiring) is one of the ESP32's four
   input-only pins (34/35/36/39), which have **no internal pull-up hardware at
   all** - `pinMode(pin, INPUT_PULLUP)` is silently ignored on these specific
   pins by the silicon itself. If your encoder module doesn't supply its own
   onboard pull-up resistors on that line, GPIO36 floats, and a floating CLK
   corrupts the whole quadrature state machine regardless of what the firmware
   does. GPIO32 (DT) and GPIO14 (KEY/SW) are normal GPIOs with working internal
   pull-ups, so they're less likely to be the issue.
   **Recommended fix: add an external ~10kΩ pull-up resistor from GPIO36 to
   3.3V** (and it doesn't hurt to add the same on GPIO32/DT and GPIO14/KEY too,
   even if their internal pull-ups are probably already sufficient). Also added
   a small time-based noise filter to the ISR itself (ignores transitions
   faster than realistic rotation speed) - this helps with chatter from a
   marginal signal but is not a substitute for the resistor if that's the
   actual root cause.

---

# Round 3 of Post-Flash Fixes

6. **Station names overlapping when too long for the screen** —
   `showStationList()` drew raw, unmeasured text with Adafruit GFX's default
   text-wrap behavior left on. A name too wide for one line (e.g. "SomaFM -
   Deep Space One") would wrap onto the next line's vertical space instead of
   being clipped, visually overlapping whatever was drawn there. Fixed by
   disabling wrap for this screen and adding a real `truncateToFit()` helper
   that measures actual rendered text width via `getTextBounds()` and trims
   with an ellipsis until it fits - not a guess at character count, an actual
   pixel-width measurement against the font in use.

7. **Every station failing with "Failed to play station"** — a real, serious
   bug in `connectToStream()`: immediately after sending the HTTP request, the
   code checked `while (wifiClient.available())` to start reading the
   response. `available()` reports whether bytes have *already* arrived at
   that exact instant - and since the request had only just been sent
   microseconds earlier, the server hadn't had any real-world time (a network
   round trip is tens to low-hundreds of milliseconds) to respond yet. The
   loop's condition was false before it ever ran once, so `headerFound` never
   got set, and every single connection attempt failed with "Invalid HTTP
   response" - regardless of whether the server would have responded
   correctly. This explains why it now failed for *every* station rather than
   just the format-incompatible ones from the previous round: this bug was
   always there, just previously masked by the URL and display bugs that were
   fixed first. Fixed by adding a proper wait-with-timeout before reading
   begins, and a bounded timeout on the header-read loop itself so a stalled
   or dead connection can't hang forever.

8. **One encoder click registering as two movements** — this is a well-known
   failure mode of the approach originally used: interrupts on CHANGE of
   *both* CLK and DT, decoded through a full 4-state Gray code table. Many
   common rotary encoder modules produce more than one electrical transition
   per mechanical detent click, and triggering on both pins' changes
   compounds that further. Replaced with the simpler, standard technique for
   this class of encoder: interrupt only on CLK's falling edge, with
   direction read synchronously by comparing DT's level against CLK's at that
   moment. This is both simpler and far less prone to over-counting than the
   dual-pin approach. The now-unused `encoderLastState` tracking was removed
   entirely rather than left dangling.

---

# Round 4 of Post-Flash Fixes

9. **Encoder scrolling continuously with no physical input** — this is the
   predicted consequence of the GPIO36 floating-pin issue flagged in item 5,
   now confirmed by the specific symptom shape. GPIO36 (CLK) is input-only
   with no internal pull-up; if the encoder module has no onboard pull-up of
   its own, that pin floats and picks up electrical noise. With the switch to
   single-edge (FALLING) triggering in item 8, a floating CLK line firing on
   noise gets compared against DT (GPIO32, which has a working pull-up and
   sits stable) - and that comparison lands the same way almost every time,
   producing a steady stream of same-direction movement rather than random
   jitter. That's a specific, recognizable signature of this exact issue, not
   a new unexplained bug.
   No firmware fix fully solves a genuinely floating pin. Two real options:
   - **Add a ~10kΩ pull-up resistor from GPIO36 to 3.3V** (recommended -
     cheap, simple, fixes the actual cause).
   - **Move CLK to a different GPIO** - GPIO27 is a normal pin (not
     input-only, has a working internal pull-up) and unused elsewhere in
     this design on most ESP32 DevKit V1 boards. Verify it's actually broken
     out on your specific board before rewiring. `config.h` now documents
     this option next to `ENCODER_CLK_PIN`.
   As a partial software mitigation, the ISR's debounce window was widened
   from 2ms to 5ms - this reduces how often noise crosses the threshold, but
   it is explicitly a mitigation, not a fix, and can't fully substitute for
   addressing the floating pin directly.

---

# Round 5 of Post-Flash Fixes

10. **Encoder still double-stepping on GPIO27** — since this persisted even on
    a pin with a proper working pull-up, it confirmed the cause wasn't
    electrical noise but the encoder mechanically producing 2 real edges per
    detent (extremely common for this class of component). Fixed properly
    this time: instead of firing an event on any nonzero raw count and
    zeroing it, `update()` now requires the raw count to reach
    `ENCODER_STEPS_PER_DETENT` (2, tunable in `config.h`) before firing one
    event, and only subtracts that threshold rather than zeroing - so a
    leftover partial count from a fast double-detent isn't lost or misread.

11. **"PLAYING" and "Buffering..." shown together, contradicting each other**
    — `playStation()` always passed a hardcoded `playing=true` *and* a
    literal `"Buffering..."` placeholder that nothing ever updated, so both
    appeared permanently regardless of actual state. Reworked
    `showPlaying()` to take one accurate status string instead, and wired it
    into the real playback state: shows "Buffering..." when the connection
    starts, automatically updates to "Playing" once `AudioPlayer`'s state
    actually transitions (checked every second in `updateUI()`), and
    Play/Pause now actually updates the display too (previously it silently
    toggled the audio state with no visual feedback at all).

12. **Volume bar never disappeared** — `showVolume()` drew a partial-window
    overlay (covering the same screen region as the footer text) with
    nothing to ever clear it afterward. Added `volumeDisplayUntil`, set 2
    seconds out on every volume change; `updateUI()` checks it and redraws
    the correct underlying screen (now-playing or Bluetooth) once it
    expires, which also restores the footer text the overlay had been
    sitting on top of.

13. **Station name cut off mid-word under the header line** — `drawHeader()`
    drew raw, unmeasured text with default wrap behavior, so long names
    (e.g. "SomaFM - Deep Space One") wrapped onto a second line that spilled
    below the divider. Implemented a stepped marquee: text that fits is
    drawn normally; text that doesn't is scrolled in visible chunks every
    ~1.5-2s via its own small partial-window refresh. This is deliberately
    not smooth pixel scrolling - e-paper partial refreshes take real time
    (a few hundred ms) and smooth scrolling would mean constant flicker and
    unnecessary panel wear. A stepped marquee is the standard approach for
    this class of display.

14. **Whole screen flashing/inverting on every encoder scroll step** — this
    was `setFullWindow()` doing exactly what it's designed to do: a
    deliberate black/white flash cycle to prevent e-paper ghosting on a full
    refresh. It's normal on an *occasional* full redraw, but the station
    list now redraws on every single scroll step (from the previous round's
    fix for "no visible feedback while scrolling"), so the flash was firing
    constantly. Switched `showStationList()` and `showPlaying()` (which now
    also redraws automatically on state changes) to `setPartialWindow()`
    instead, which updates without the flash. Trade-off, noted in code
    comments: partial updates can accumulate faint ghosting over many
    cycles without an occasional full refresh - acceptable given neither
    screen is typically shown continuously for very long stretches.

---

# Round 6 of Post-Flash Fixes

15. **Encoder still misbehaving after moving to GPIO27** — this wasn't a
    hardware or ISR problem at all: `config.h` only ever had a *comment*
    suggesting GPIO27 as an option, the `#define ENCODER_CLK_PIN` value was
    never actually changed from `36`. The wire moved to GPIO27, but the
    firmware kept reading GPIO36 - which was now sitting fully disconnected
    (the wire had moved away from it), floating and noisy, while GPIO27 sat
    there correctly wired and pulled up but never actually read by anything.
    Confirmed by serial log: the `[INPUT] Play/Pause pressed` /
    `Next pressed` / `Prev pressed` lines firing with zero physical input
    turned out to be a completely separate, genuine issue - those buttons
    aren't wired to anything at all yet, so GPIO 34/35/39 (all input-only,
    same pull-up limitation as the original GPIO36 problem) are floating for
    real. Not a firmware bug - those pins need actual buttons wired to them.
    Fixed `ENCODER_CLK_PIN` to actually be `27` this time (not just a
    comment), and confirmed no other file hardcodes pin 36 anywhere - every
    reference goes through this one macro, so this single edit is complete.

---

# Round 7 of Post-Flash Fixes

16. **Buffering indefinitely, every time** — a real, precisely-located bug:
    `connectToStream()` correctly sets `playbackState = STATE_PLAYING` once
    the HTTP connection and headers are confirmed. But `play()` (which
    calls `connectToStream()`) unconditionally overwrote that right back to
    `STATE_BUFFERING` immediately afterward, every single time, on every
    successful connection. Since the actual audio-reading task only reads
    and plays data `if (playbackState == STATE_PLAYING ...)`, and nothing
    else in the codebase ever set it back to `STATE_PLAYING` again after
    that point, the read loop could never run - no audio data was ever
    pulled from the stream, and the state genuinely never left
    "Buffering..." Removed the incorrect overwrite; `connectToStream()`
    already owns that transition correctly on its own.

17. **Marquee scroll speed** — reduced the step interval by 200ms as
    requested (1500ms -> 1300ms per step) in `updateHeaderScrollIfNeeded()`.

---

# Round 8 of Post-Flash Fixes (pending log confirmation)

18. **Every station immediately shows the error screen** — a plausible
    real cause, applied as a defensive fix, but not yet confirmed against
    a serial log for this specific failure (the code prints a distinct
    message depending on exactly where a connection fails - TCP connect
    vs HTTP header parsing - which would confirm or rule this out).
    Key structural fact: the audio task was completely dormant before item
    16's fix (it never reached real work due to the state-stomping bug), so
    any bug inside it - including this one - could never have been
    observable until now. The task runs a tight read loop with no yield
    point while stream data keeps arriving, which for a continuous radio
    stream is nearly always true, at priority 24 (`TASK_PRIORITY_HIGH`) -
    a classic FreeRTOS/ESP32 anti-pattern where a high-priority
    never-yielding task can starve other tasks sharing the same core,
    including the WiFi stack's own internal processing needed to open a
    *new* connection. Applied two changes: a dedicated, more moderate
    `TASK_PRIORITY_AUDIO` (5) instead of reusing the generic HIGH constant,
    and an explicit `taskYIELD()` every 64 bytes inside the read loop so it
    can't fully monopolize the CPU even during continuous playback.

---

# Round 9 of Post-Flash Fixes/Features

19. **Round 8's priority fix confirmed working** — serial log showed clean
    connects, stable continuous playback, and successful station switching
    with no errors. The audio task priority/yield fix from item 18 was the
    actual cause.

20. **Now-playing title (ICY metadata) added, "Playing" moved up** — real
    feature, not previously implemented at all. Radio streams broadcast
    song/station text as ICY metadata embedded periodically INSIDE the raw
    audio byte stream itself (not a separate channel) - every `icy-metaint`
    bytes of audio, a length byte followed by a metadata string like
    `StreamTitle='Artist - Title';` appears. Implemented a proper byte-level
    parser (`processStreamByte()`/`parseIcyMetadata()` in `audio.cpp`) that
    separates real audio bytes from these embedded metadata blocks -
    important that metadata bytes never reach the DAC, they'd be heard as
    clicks/noise if written through as audio. Renamed the old, misleadingly
    named `bitrate` member (it was actually storing icy-metaint, never a
    bitrate) to `icyMetaInt`. On the display side: status text ("Playing"/
    "Buffering..."/"Paused") moved up, with a second independent scrolling
    region below it for the title text, using the same stepped-marquee
    technique as the header. Caught and fixed a real bug of my own before
    shipping: `metadataLength`/`metadataBytesRead` need to hold values up
    to 4080 (255*16 per the ICY spec) but were declared `uint8_t` (max
    255) - fixed to `uint16_t`.

21. **Marquee speed** — reduced another 200ms as requested (1500ms -> 1300ms,
    from the prior round's number).

## Known gap worth flagging: no MP3 decoding

This firmware reads raw bytes from the stream and writes them straight to
the I2S DAC as if they were already 16-bit PCM samples. The default
stations are MP3 streams (note "128-mp3" in the SomaFM URLs) - actual MP3
is compressed audio that requires real decoding (Huffman decoding, IMDCT,
etc.) before it's valid PCM. **This codebase has never implemented that
decoding step**, at any point in this conversation.

I don't know whether this is actually audible as a problem on your hardware
without you telling me - it's possible the connection/playback fixes so far
have been validated only by "it connects and doesn't error," not by
critically listening to the output. If what's coming out of the DAC sounds
like clean music, ignore this section entirely. If it sounds like static,
garbled noise, or anything that doesn't resemble the actual song, that's
this gap, and fixing it properly means integrating a real decoder (e.g. a
library like ESP32-audioI2S or arduino-audio-tools with its MP3 decoder) -
a substantially bigger change than anything done in this conversation so
far, worth discussing deliberately rather than bolting on speculatively.

---

# Round 10: Bluetooth Screen Enhancement

22. **Added a Bluetooth icon and connected device name.** The A2DP library
    actually exposes `is_connected()` and `get_connected_source_name()` -
    verified directly against the real library source before using them,
    not assumed. Along the way, found and fixed two real gaps:
    - `AudioPlayer::isBluetoothConnected()` was a stale inline getter
      reading a member (`btConnected`) that was declared and initialized
      but never actually updated anywhere - always false regardless of
      real connection state. Removed the dead member; the method now
      queries the library directly, live.
    - Both call sites for `showBluetooth()` in `main.cpp` passed
      `BT_DEVICE_NAME` (the ESP32's own advertised name) unconditionally,
      even in the "connected" case - meaning even after wiring up a real
      name lookup, the screen would have shown the ESP32's own name
      instead of the connected phone's. Fixed to pass the real device name
      when connected, the ESP32's own name (for pairing guidance) when not.
    - The Bluetooth screen previously only redrew when first entering
      Bluetooth mode or when a volume-overlay timeout happened to fire - a
      phone connecting without the user touching the volume would never
      actually update the screen. Added proper connection-state transition
      detection in `updateUI()`, mirroring the existing playback-state
      pattern.
    Icon is drawn from 5 line segments (the classic Bluetooth bind-rune
    shape) via `drawBluetoothIcon()` - no bitmap/glyph assets needed on a
    monochrome e-paper display. The connected device name reuses the same
    scroll-region mechanism built for the now-playing title on the radio
    screen (the two screens are mutually exclusive, so sharing the region
    and its marquee logic is safe and avoids a third near-duplicate
    implementation) - its gate was broadened from "radio screen only" to
    "radio screen OR Bluetooth screen."

---

# Round 11: Compile Fixes for the Bluetooth Screen Feature

23. **`get_connected_source_name()` is protected within this context** — a
    real miss on my part: last round I confirmed this method *exists* in
    the library but didn't check its access specifier. It's actually
    `protected`. The library provides a public wrapper right above it,
    `get_peer_name()`, which just calls the protected method internally -
    switched to that instead. Also went back and specifically re-verified
    `is_connected()` is genuinely in a `public:` section this time (it is),
    rather than repeating the same class of mistake on a second method.

24. **`inline static` triggering "only available with -std=c++17" warnings**
    — non-fatal, but worth removing rather than ignoring: `platformio.ini`
    does request `-std=c++17`, but PlatformIO/the ESP32 Arduino framework
    can silently override a project's `-std=` flag with its own internal
    default depending on build flag ordering, and that's not something
    fully diagnosable from a log excerpt alone. Rather than chase the exact
    mechanism, converted `encoderPosition` and `encoderLastInterruptMicros`
    in `input.h`/`input.cpp` from C++17-only "inline static" syntax to the
    traditional static-member pattern (declared in the header, defined with
    initial value in the .cpp) - this has worked correctly in every C++
    standard for decades, removing the dependency entirely rather than
    hoping the flag takes effect.

---

# Round 12: Encrypted WiFi Support

25. **No way to connect to an encrypted network** — a real, pre-existing gap
    since the very first draft: the first-boot fallback (when no saved
    EEPROM credentials exist) scanned and connected to whichever network
    was found first, with an empty password - only ever worked for open
    networks. There was no way to specify a password at all. Added
    `WIFI_SSID`/`WIFI_PASSWORD` to `config.h` (edit these to your actual
    network before flashing) and replaced the scan-and-guess fallback with
    a direct connection attempt using them. Password persistence through
    EEPROM was already implemented correctly (verified before touching
    anything) - this only needed a fix at the entry point, not the whole
    pipeline. After the first successful connect, later boots reconnect
    from EEPROM automatically; config.h's credentials are only needed once.

---

# Round 13: Real MP3 Decoding (Major Change)

26. **Implemented actual MP3 decoding - the gap flagged since early in this
    conversation.** Previously `audio.cpp` read raw compressed MP3 bytes
    and wrote them straight to the DAC as if they were already PCM
    samples. This is a complete rework of the audio pipeline, not a patch.

    **Library selection, and why:** The most commonly-recommended library
    for this exact use case (ESP32-audioI2S) was checked first and
    rejected - it hard-requires PSRAM (verified directly in its source:
    `if (!psramFound) { result = false; goto exit; }`), and this board
    (logged as "DOIT ESP32 DEVKIT V1" in every build) is almost certainly
    the standard variant without it. Switched to `earlephilhower/ESP8266Audio`
    instead - verified zero PSRAM dependency anywhere in its source (it's
    designed to also run on plain ESP8266, which never has this kind of
    PSRAM). Added via git URL in `platformio.ini` rather than a registry
    name, consistent with every other lesson learned earlier in this
    conversation about registry name guessing failing repeatedly.

    **What changed:** `audio.h`/`audio.cpp` rewritten around a real
    pipeline: `AudioFileSourceICYStream` (HTTP + ICY metadata, delegates
    URL parsing entirely to Arduino's standard `HTTPClient` rather than a
    hand-rolled parser - notably this sidesteps the exact port-parsing bug
    my own old code had) -> `AudioFileSourceBuffer` (ring buffer, the
    library's own recommended pattern) -> `AudioGeneratorMP3a` (Helix
    decoder, chosen over the libmad-based `AudioGeneratorMP3` for its much
    smaller fixed-size buffers, no PSRAM available) -> `AudioOutputI2S`
    (I2S to the PCM5102A). Every API used was verified against the actual
    cloned library source and a real working example
    (`examples/StreamMP3FromHTTP`), not assumed from memory.

    **Public interface preserved exactly** - every method `main.cpp` calls
    on `audioPlayer` (`play`, `pause`, `resume`, `getState`,
    `getNowPlaying`, `enableBluetooth`, etc.) has the identical signature
    as before, so `main.cpp` needed zero changes. Verified this directly
    against the file, not assumed.

    **Two real issues found and fixed before finishing:**
    - This new library uses the *newer* ESP-IDF `i2s_std.h` channel API
      internally, while the Bluetooth library uses the *legacy*
      `driver/i2s.h` API - they can't both own the I2S peripheral
      simultaneously. Verified `AudioOutputI2S::stop()` properly releases
      the channel (`i2s_channel_disable`/`i2s_del_channel`) and that
      `begin()` can be safely re-called afterward, and coordinated the
      handoff explicitly in `enableBluetooth()`/`disableBluetooth()`.
    - `enableBluetooth()` still needs the legacy `i2s_pin_config_t` type
      for the A2DP library's pin config - after removing `driver/i2s.h`
      from `audio.h` (no longer needed there), this was left relying on
      an unverified transitive include. Added it explicitly to `audio.cpp`
      instead of hoping the A2DP library happened to pull it in.

    **Design notes:**
    - This library has no true pause/resume concept (makes sense for a
      live stream - there's nothing to "pause" in the broadcast itself).
      Simulated by muting output (`SetGain(0)`) while leaving the
      stream/decoder running in the background, so resume is instant with
      no reconnection delay - this seemed like the better UX trade-off
      than fully disconnecting and reconnecting.
    - Bluetooth audio doesn't route through `AudioOutputI2S` at all - the
      A2DP library writes to I2S via its own separate internal path, so
      `setVolume()` now branches: `a2dp_sink.set_volume()` (0-127 range)
      for Bluetooth, `i2sOutput->SetGain()` (0.0-1.0 range) for radio.
    - Audio task stack bumped 6144 -> 8192 bytes as a safety margin now
      that a real decoder runs inside it, not just byte copying.

---

# Round 14: ESP8266Audio Version Pin

27. **`fatal error: driver/i2s_std.h: No such file or directory`** —
    diagnosed against the library's actual commit history rather than
    guessed at. The unpinned git URL fetched the latest commit
    (`74fc1f0`), which includes `e0143bb "ESP IDF 6.0 compat, update I2S,
    DAC, ULP (#783)"` - a migration to ESP-IDF 6.0's newer `i2s_std.h`
    driver. This board's bundled ESP-IDF (via Arduino core 3.x) is 5.x, a
    full major version behind what that migration needs - hence the
    missing header.
    Found the exact parent commit right before that migration
    (`058e131`, 2025-10-13) and pinned to it instead. Verified before
    committing to this: it still uses the legacy `driver/i2s.h` API (the
    same one the Bluetooth library already uses successfully on this exact
    toolchain), every class/method this code calls is present
    (`AudioFileSourceICYStream`, `AudioFileSourceBuffer`,
    `AudioGeneratorMP3a`, `AudioOutputI2S::SetPinout`/`SetGain`), the
    no-arg `AudioOutputI2S()` constructor still defaults to external I2S
    mode at this commit (via `output_mode = EXTERNAL_I2S` default
    parameter, confirmed even though this older commit structures the
    constructor differently than the version originally checked), and
    `stop()` uses `i2s_driver_uninstall()` - the exact same legacy
    teardown function already relied on elsewhere for the Bluetooth
    handoff, which if anything makes that handoff more consistent than
    originally designed around (both libraries now use the same I2S API
    generation, not two different ones as first assumed).

---

# Round 15: Real Audio Working - Performance/Responsiveness Fixes

28. **Real bug in my own audio task rewrite: no yield in the active-playback
    branch.** The audio task runs at priority 5 - higher than the default
    main loop task (priority 1). `audioTaskFunc()`'s main loop called
    `mp3Decoder->loop()` back-to-back with zero `vTaskDelay`/yield whenever
    a station was actively playing (which is essentially always while
    playing). Since this task never blocked, FreeRTOS's preemptive
    scheduler had no reason to ever give the lower-priority main loop any
    CPU time at all - a textbook starvation bug. This explains both
    "sluggish everything" (main loop, which drives the whole UI, was
    starved) and "can't get back to the station menu" (`inputControl.update()`
    - which reads the encoder click - never got scheduled to run). Verified
    the menu-return logic itself has no separate bug; this one root cause
    covers both symptoms. Fixed with a mandatory `vTaskDelay(1)` after every
    decode iteration - brief enough (~1ms) not to meaningfully affect audio
    throughput, since a single `loop()` call processes far more than 1ms
    worth of audio data.

29. **Audible buffering/cutting** — `AudioFileSourceBuffer` was sized at
    2048 bytes, which at typical 128kbps MP3 is only ~128ms of cushion
    against WiFi jitter and scheduling gaps - not much margin at all.
    Increased to 16384 bytes (~1 second of cushion). Adjustable if this
    turns out to be too large for available RAM or still insufficient on
    actual hardware - no PSRAM on this board, but this is the only large
    buffer this pipeline allocates now (the old manual byte-reading buffer
    from before real decoding was implemented is gone entirely).

---

# Round 16: Fixing My Own Previous Fix - Throttled Decode Throughput

30. **Repeated audio cutting, likely caused by round 15's own starvation
    fix.** Traced the decoder's actual `loop()` implementation in the
    pinned commit's source: it tries to push already-decoded samples one
    at a time via `output->ConsumeSample()`, and returns almost
    immediately the moment that call reports the I2S output's small
    internal buffer is momentarily full - relying on being called again
    very soon after. This library is designed to be called back-to-back,
    as fast as possible.
    Round 15's fix (`vTaskDelay(1)` after every single call) capped this
    loop near ~1000 calls/sec regardless of how little work each call
    actually did - very likely throttling real decode throughput below
    what real-time playback needs, causing chronic (not just
    jitter-related) underrun. That's probably what "cuts multiple times"
    actually was.
    Replaced the per-call delay with a time-bounded one instead: the loop
    runs freely, and only yields once at least 10ms of wall-clock time has
    passed since the last yield - regardless of how many iterations that
    spans. This still gives the main loop a deterministic, guaranteed
    scheduling opportunity (fixing the original round-15 problem: sluggish
    UI / can't reach the station menu), without capping how much real
    decode work happens between those yields (fixing this round's
    problem). Chose an explicit, deterministic time-based bound over
    relying on same-priority round-robin scheduling behavior, since the
    latter's exact behavior isn't something I can fully verify without
    live hardware access to check exact FreeRTOS tick configuration -
    priority stayed at 5 (elevated above the main loop's default 1) since
    audio decode is genuinely more time-sensitive than UI responsiveness,
    and the explicit bounded yield now provides the safety guarantee that
    priority 5 alone previously lacked.
    Buffer size (16KB, from round 15) deliberately left unchanged this
    round - suspect it was already adequate and just overwhelmed by the
    throttling bug above; wanted to isolate whether this fix alone
    resolves the cutting before changing a second variable.

---

# Round 17: HTTPS Support + Station List Display Fix

31. **Station list starting from the middle of the screen** — `yPos` was
    hardcoded to 100, nearly halfway down the actual available space
    between the header line (y=20) and footer line (y=180). Regardless of
    how many stations there were, this left roughly the top half of the
    list area empty. Moved to y=45.

32. **Added real HTTPS support**, since most modern streaming platforms
    have moved off plain HTTP (confirmed while researching Romanian
    stations - this wasn't a hypothetical concern). The ESP8266Audio
    library's HTTP/ICY source classes only support plain `WiFiClient`.
    Rather than patch the pinned upstream library directly (which would
    make future updates fragile), vendored two new files into this
    project - `AudioFileSourceHTTPSStream`/`AudioFileSourceICYSStream` -
    copies of the library's own well-tested `AudioFileSourceHTTPStream`/
    `AudioFileSourceICYStream` with `WiFiClientSecure` substituted in for
    the client member. The ICY metadata parsing logic itself (partial-read
    handling, StreamTitle extraction) is untouched, copied as-is.
    Verified before writing any of this: `WiFiClientSecure` genuinely
    inherits from `NetworkClient` in this core version (confirmed directly
    in the Arduino-ESP32 source), so it's a valid argument wherever
    `HTTPClient::begin(NetworkClient&, ...)` is expected. Also confirmed
    `AudioFileSource`'s destructor is virtual, so `audio.cpp` can hold
    either the HTTP or HTTPS concrete source behind one polymorphic
    `AudioFileSource*` pointer and delete it safely either way.
    `play()` now picks the HTTP or HTTPS class automatically based on
    whether the URL starts with `https://` - both can coexist in the same
    station list.
    Two things worth knowing, stated plainly rather than glossed over:
    - Certificate validation is skipped (`setInsecure()`), since managing
      CA certificates for arbitrary streaming servers isn't practical here.
      Standard practice for this kind of project, but it does mean no
      protection against a spoofed server.
    - TLS handshakes use meaningfully more RAM than plain HTTP, and this
      board has no PSRAM. Untested under real memory pressure - worth
      watching for if HTTPS stations are added.

## Station list content (items 2 and 3 of the request)

Not completed this round. Extensive research went into finding real,
verified Romanian station URLs (West City Radio specifically, plus Radio
România's public broadcaster network) - genuinely hit tool limitations,
not just difficulty: the radio-browser.info API (the right tool for this)
is blocked by robots.txt from this environment, and most individual
station sites load their actual stream URL via JavaScript that a static
page fetch can't see. I did not want to guess/fabricate URLs the way the
original placeholder stations were - that caused real problems earlier in
this conversation and I'm not repeating it.

Recommended path: on your own machine, open a station's live-player page,
open browser DevTools (F12) -> Network tab, filter for Media/audio, and
press play - the actual stream URL will appear as a request. This takes
under a minute per station and completely sidesteps the JS-obscuring and
robots.txt issues I hit. Send me whatever URLs you find and I'll verify
the format is compatible (and fix the port-in-URL/playlist-wrapper class
of issues if any come up) before adding them to stations.cpp.

---

# Round 18: AAC Support + West City Radio

33. **Added AAC decoding**, since the real West City Radio URL (found via
    the DevTools method) turned out to be AAC, not MP3 - a completely
    different codec the MP3 decoder can't handle at all. Checked the
    already-integrated library first before adding anything new: it
    includes `AudioGeneratorAAC`, backed by the same Helix decoder family
    as the MP3a class already in use (`libhelix-aac`, same design
    philosophy as `libhelix-mp3` - no PSRAM needed, same reasoning as the
    original MP3 decoder choice). Same public interface
    (`begin`/`loop`/`stop`/`isRunning`) as `AudioGeneratorMP3a`, confirmed
    before using it - genuinely a drop-in alternative, not a new
    integration pattern to design from scratch.
    Added a `codec` field to the station data model (`StationCodec` in
    `stations.h`) rather than trying to auto-detect codec from the URL or
    HTTP response - explicit and reliable beats a fragile heuristic here.
    `AudioPlayer::play()` now takes an `AudioCodec` parameter and picks
    `AudioGeneratorMP3a` or `AudioGeneratorAAC` accordingly, held behind
    one polymorphic `AudioGenerator*` pointer - confirmed the base class
    has a virtual destructor first, so the existing teardown code (already
    written generically) needed no changes at all.

34. **Removed all previous placeholder stations** (SomaFM x4, Radio
    Paradise, WFUV) per request - replaced with real, user-confirmed
    stations instead of more URLs I can't verify from this environment.

35. **Added West City Radio** at the top of the list, using the real URL
    the user found via browser DevTools (Network tab while the stream
    played) - the reliable method suggested after this environment's tools
    hit real limits (robots.txt blocks, JS-obscured players) trying to
    find it directly. HTTPS + AAC, both now supported as of this round and
    the previous one.

More Romanian stations still to come as their real URLs get confirmed the
same way - station list is deliberately minimal right now rather than
padded with more guessed placeholder entries.

---

# Round 19: Vendored HTTPS Files Compile Fix

36. **`'NetworkClient' was not declared in this scope`** in my own vendored
    `AudioFileSourceHTTPSStream.cpp`/`AudioFileSourceICYSStream.cpp`. Real
    mistake: the library's original files guard this with
    `#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3`
    (NetworkClient vs WiFiClient depending on core version) - when
    vendoring, I hardcoded just the newer NetworkClient branch (reasonable
    given this project targets core 3.x) but never explicitly included the
    header that type comes from, assuming it would arrive transitively
    through `WiFiClientSecure.h`. Traced the actual chain in the Arduino-
    ESP32 core: `WiFiClientSecure.h` -> `NetworkClientSecure.h` ->
    `Network.h` -> `NetworkClient.h`, which technically should have worked
    - but I'd only checked this against a generic `3.0.7` tag, not the
    exact resolved framework version (`3.20017.241212`), and didn't want to
    assume identical internal header structure between them. Fixed by
    including `<NetworkClient.h>` explicitly and directly in both vendored
    headers, rather than continuing to rely on a transitive chain I could
    only partially verify - the same "explicit over transitive" lesson
    from an earlier round's `i2s_pin_config_t` issue, which I should have
    applied here from the start.

---

# Round 20: Actually Fixing the NetworkClient.h Include (properly this time)

37. **`fatal error: NetworkClient.h: No such file or directory`** — my
    round 19 "fix" was itself wrong: I added an explicit direct include
    for a header path that isn't reachable from this project's own src/
    files the same way it apparently is from inside a proper PlatformIO
    library's build context. Went back to actual evidence instead of
    guessing a third time: the *original, unmodified* library file uses
    bare `NetworkClient` with no explicit include of that header at all,
    and it compiles successfully on this exact machine (confirmed - no
    error reported for it in the same build log that failed on my files).
    Checked `HTTPClient.h` directly (already included by both the original
    file and mine) and confirmed it includes `<NetworkClient.h>` AND
    `<NetworkClientSecure.h>` internally itself. Removed my incorrect
    explicit include entirely - reverted to relying purely on
    `<HTTPClient.h>`'s own transitive provision, exactly matching what the
    proven-working original file does. Also removed the separate
    `<WiFiClientSecure.h>` include and switched to the real class name
    `NetworkClientSecure` directly (`WiFiClientSecure` is just a typedef
    alias for it) - reduces this to depending on only the one header
    that's actually confirmed to work here, instead of two separate
    unverified assumptions.

---

# Round 21: The Real Fix - Trusting the Compiler's Own Evidence

38. **`'NetworkClientSecure' does not name a type; did you mean
    'WiFiClientSecure'?`** — this error was actually much more useful than
    the previous ones: it's direct evidence from the compiler about what
    this exact framework build genuinely provides, not an ambiguous
    "file not found." The suggestion itself confirmed `WiFiClientSecure`
    IS a real, recognized name here - this specific framework build uses
    the classic `WiFiClient`/`WiFiClientSecure` naming, not the newer
    `NetworkClient`/`NetworkClientSecure` abstraction I'd been assuming
    based on a generic version check that turned out not to match this
    exact resolved build closely enough.
    Fixed properly this time:
    - Member is `WiFiClientSecure` again, with its own confirmed-safe
      `#include <WiFiClientSecure.h>` (this was never actually reported
      as a missing-file error in any prior log, unlike NetworkClient.h -
      real evidence it's fine here).
    - The `stream` local variable (previously guessed as `NetworkClient*`,
      almost guessed again as `WiFiClient*`) now uses `auto` instead of
      naming a concrete type at all - it only ever calls `->available()`
      and `->read()`, both standard base-class methods, so there's no
      reason to guess a specific type name here a third time when the
      compiler can just deduce it correctly regardless of what this
      framework calls it.

---

# Round 22: Remote Control + Screen Text Dump via Serial

39. **Added serial command input and a text screen summary**, useful for
    testing over remote desktop or before the physical buttons/panel are
    wired up. `checkSerialCommands()` maps single keystrokes to the exact
    same `InputEvent` values the physical encoder/buttons produce, and
    feeds them through the same `handleInput()` pipeline used everywhere
    else - not a separate/parallel control path, so anything that works
    physically works identically here.
    
    Commands (see also the banner printed at boot):
    - `u`/`d` - encoder up/down (scroll stations, adjust volume)
    - `k` - encoder click (select/confirm/back)
    - `space` - play/pause
    - `n`/`p` - next/previous station
    - `l` - long-press (enter/exit Bluetooth mode)
    - `s` - print the current screen's contents as text
    
    `printCurrentScreen()` reads the same state (`currentMode`,
    `stationManager`, `audioPlayer`) the actual display already draws
    from, without touching `display.cpp` at all - a separate, parallel
    text summary rather than trying to mirror the e-paper's exact pixel
    layout.

---

# Round 23: West City Radio - Real Working URL

40. **Switched West City Radio to a direct plain-HTTP MP3 URL, confirmed
    working by the user.** The original DevTools-found URL went through a
    relay (`api.3.5.2.webradio.tools`) in front of the actual Icecast
    server, over HTTPS with AAC+SBR - which hit the RAM ceiling from item
    39 (`OOM in SBR, can't allocate 50788 bytes`). Fetching that relay's
    own status API (the same URL, opened in a browser) revealed the real
    underlying server's IP and its `/mp3` mount point directly in the
    JSON response. Confirmed by the user: `http://188.214.156.114:8000/mp3`
    plays. This sidesteps both problems at once - plain HTTP (no TLS
    overhead) and MP3 (no SBR/AAC decode RAM issue), using the
    already-solid decode path instead of the newer, RAM-hungrier one.
    Noted in the source comment: this is a bare IP, not a hostname, so
    it's more fragile long-term if the station migrates servers than a
    proper domain would be - worth revisiting if it ever stops working.

---

# Round 24: Verifying the Volume Overlay Actually Works, Remotely

41. **Made the volume overlay's show/clear cycle actually verifiable over
    serial**, since the `'s'` screen dump previously always printed the
    volume level as flat status text - it couldn't tell you whether the
    *overlay* was actually appearing and auto-clearing on the physical
    e-paper, which matters when you can't see the panel directly (remote
    desktop). Consolidated 4 duplicated call sites (encoder up/down, in
    both radio and Bluetooth modes) into one `showVolumeOverlay()` helper
    that explicitly logs `"[SCREEN] Volume overlay shown: X% (clears in
    2s)"`, and added a matching log line where it auto-clears. Also
    updated `printCurrentScreen()` (the `'s'` command) to explicitly show
    when the overlay is currently active and how much time is left on it,
    rather than only ever showing the flat "Volume: X%" line that looked
    identical whether the overlay was showing or not.

---

# Round 25: 11 More Real Romanian Stations

42. **Added 11 more stations, all user-confirmed via browser DevTools** -
    not guessed. Increased `MAX_STATIONS` from 10 to 15 to fit them
    (West City Radio + 11 new = 12 total) - a small, fixed, compile-time
    struct array size increase (~324 bytes/station), a completely
    different kind of memory cost than the dynamic TLS/SBR decode-time
    pressure that caused the earlier OOM crash, so this doesn't
    reintroduce that risk.
    
    Codec-tagged individually based on the user's own notes and URL
    naming conventions:
    - **Plain MP3, plain HTTP (lowest risk)**: Digi FM, Dance FM, ProFM -
      all on the same edge*.rdsnet.ro infrastructure.
    - **AAC over plain HTTP**: Radio Guerrilla (.aac, weaker SBR signal
      than the "aacp" ones below), Radio ZU and Europa FM (both
      "aacp"/AAC+ - the same HE-AAC/SBR codec family that caused the
      "OOM in SBR, can't allocate 50788 bytes" crash with West City
      Radio's original stream. Real chance of hitting the same wall).
    - **AAC+ over HTTPS (highest risk)**: Kiss FM, Rock FM, Magic FM,
      Magic FM 90s Hits - this is the exact TLS+SBR combination that
      caused the original crash. Genuinely likely to repeat it on this
      board (no PSRAM).
    - **Radio Impuls**: codec not confirmed either way from the URL;
      defaulted to MP3 as the safer guess - worst case if wrong is no
      sound rather than a crash.
    
    Not silently adding these and hoping - flagged clearly in the source
    comments (and here) which ones are expected to likely fail with the
    known SBR OOM, so if/when that happens it's recognizable as the known
    issue rather than a surprise.

---

# Round 26 (Part 1): Station Discovery Foundation

This is a large feature (browse any country/genre via radio-browser.info,
always MP3, plus a persisted Recent-5 and user-curated Favorites) - built
the foundational, highest-risk pieces first and checked each carefully
before moving on. UI wiring (the actual on-screen browse flow) is not
done yet - see next entry.

43. **New `browser.h`/`browser.cpp` module** - the actual API client.
    Verified real API field names directly from multiple independent
    sources (not assumed): countries return `name`/`iso_3166_1`, tags
    return `name`, station search returns `name`/`url_resolved` (the
    pre-resolved direct stream link - sidesteps the playlist-wrapper
    problem entirely). Always requests `codec=mp3` server-side per
    requirements - also sidesteps the AAC+/SBR memory risk entirely for
    anything found this way, not just by request but by design.
    
    RAM-conscious by construction, not as an afterthought:
    - Uses ArduinoJson's `Filter` option (verified against the library's
      own test suite for exact syntax) so only the 2-3 fields actually
      needed are kept in the parsed document, not the full ~15-field
      station record radio-browser.info returns.
    - Parses directly from the HTTP response stream
      (`deserializeJson(doc, http.getStream(), ...)`) rather than first
      buffering the complete raw response as a String, which would
      double memory use for no benefit - confirmed ArduinoJson's
      deserializer is generically templated to accept any stream-like
      reader, not just raw buffers.
    - Fixed-size static arrays (250 countries / 40 tags / 30 results),
      matching this project's existing style - a small, fixed,
      compile-time cost, not the dynamic TLS/decode-time pressure that
      caused the earlier OOM crash.
    
    Noted in code comments: radio-browser.info's own docs ask clients not
    to hardcode a single mirror and instead discover servers via DNS -
    implementing that properly means parsing multiple raw DNS records,
    real added complexity for a small risk. Used the single mirror
    directly as a deliberate, documented simplification.

44. **Recent (last 5) and Favorites, persisted via `Preferences` (ESP32's
    built-in NVS key-value flash storage)** - added to `StationManager`
    rather than a separate module, since it already owns the "collection
    of stations" concept and reuses the same `RadioStation` shape.
    Recent is a most-recently-used ring buffer (re-selecting an existing
    recent moves it to the front rather than duplicating); Favorites is
    explicit add/remove, checked for duplicates. Verified `Preferences`'s
    real API (`putString`/`getString`/`putUChar`/`getUChar`) directly
    against the Arduino-ESP32 core source before using it.

45. **Generalized `Display::showStationList()`** with an optional `title`
    parameter (defaulted to "Select Station", so the existing call site
    is unaffected) - this lets the same list-rendering code get reused for
    Country/Genre/Results/Favorites/Recent screens instead of writing five
    near-duplicate functions.

**Still to come**: the actual state machine wiring in `main.cpp` - entry
points from the main station list, the Country -> Genre -> Results flow,
a loading indicator for the blocking API calls (these take a real amount
of time over the network), and hooking "select from results" into
addToRecent()/an explicit favorite-add action. Everything built so far
compiles cleanly on its own and doesn't touch any existing working
behavior - continuing from here is additive, not a rework.

---

# Round 26 (Part 2): Fixing My Own JsonDocument Forward-Declaration Bug

46. **`reference to 'JsonDocument' is ambiguous`** - real mistake in
    `browser.h`: used `class JsonDocument&` as a forward declaration to
    avoid including the full `<ArduinoJson.h>` header there, but
    ArduinoJson's real `JsonDocument` lives inside its own namespace and
    gets brought into scope by its own header - once `browser.cpp`
    included both `<ArduinoJson.h>` (for the real type) and `browser.h`
    (with my separate, colliding forward-declared one), the compiler saw
    two distinct things both named `JsonDocument` and couldn't tell them
    apart. Fixed by just including `<ArduinoJson.h>` properly in
    `browser.h` instead of trying to avoid it - there was no real benefit
    to the forward declaration here since `browser.h` is only ever
    included by `browser.cpp` and `main.cpp`, both of which need the real
    ArduinoJson types anyway.

---

# Round 26 (Part 3): Station Discovery UI - Fully Wired

Completed the remaining piece from Part 1/2 - the actual on-screen flow.
The station list now has 3 entries at the top: "Browse All Stations",
"Favorites (N)", "Recent (N)", followed by the existing 12 stations.

47. **Full Country -> Genre -> Results flow.** Each step shows a loading
    screen during its (blocking) network call, then a scrollable list
    (reusing the now-generalized `showStationList()` rather than writing
    three near-duplicate screens). Long-press steps back one level at
    each stage (Results -> Genre -> Country -> main list). On the Results
    screen: encoder click plays the station; Play/Pause button plays
    *and* adds to Favorites - the explicit "should I choose to" gesture
    from the original request, not automatic.

48. **Favorites and Recent screens**, both purely local (no network call,
    instant). Favorites: click plays, Play/Pause button removes it from
    the list (deliberately a different action from play, so browsing
    doesn't accidentally trigger a delete). Recent: click plays and
    re-promotes it to the front (standard MRU behavior).

49. **Fixed a real bug found while wiring this in**: `handlePlayingInput()`
    (pause/resume, and the periodic status-refresh in `updateUI()`) read
    the now-playing name directly via
    `stationManager.getStationName(selectedStation)` - which would show
    the *wrong* name whenever a discovered station (browse/favorites/
    recent) was playing, since `selectedStation` doesn't point at
    anything in that case. Added a dedicated `nowPlayingName` tracker set
    consistently by every play path (`playStation()` and the new
    `playDiscoveredStation()`), and fixed all 4 places that were reading
    the old, sometimes-wrong source. Also: Next/Prev are now explicitly
    disabled (with a clear serial log explaining why) while a discovered
    station is playing, since there's no sensible "next station" in a
    one-off search result the way there is in the fixed list.

50. **`playDiscoveredStation()`** is the single shared entry point for
    playing anything not from the hardcoded list - Browse Results,
    Favorites, and Recent all funnel through it instead of duplicating
    play/track-setting logic three times.

Serial `'s'` screen dump and the remote-control keys (u/d/k/space/n/p/l)
work identically across every new screen too - nothing about the
existing remote-testing setup needed to change, it just extends
naturally since the new modes plug into the same handleInput() dispatch
and printCurrentScreen() pattern as everything else.

---

# Round 27: Removed Hardcoded Stations

51. **Removed the 12 hardcoded stations from the default load.** Wrapped
    in `#if 0`/`#endif` rather than deleted outright, given how much
    verification went into finding real, working URLs for each one (the
    DevTools captures, the per-station SBR/OOM risk analysis) - trivial
    to restore with `#if 1` if ever wanted back. The station list now
    shows only the three menu entries (Browse All Stations, Favorites,
    Recent) - fully driven by the discovery flow from here on.

**Important for testing**: the previous "browse menu missing" report
looks like a reflash issue, not a code issue - the serial log showed
`device monitor` being run directly, with no `run --target upload`
beforehand, meaning the ESP32 was still running whatever was flashed
*before* round 26's UI wiring. Make sure to Upload (not just Monitor)
after pulling in a new zip.

---

# Round 28: Fixed the Serial Debug Path Being Out of Sync

52. **Real bug found**: `printCurrentScreen()` (the `'s'` serial command)
    has always been a separate, parallel implementation from
    `refreshStationListDisplay()` (the actual e-paper draw path) - they
    each independently read the same state and reconstruct their own
    view of it. When the 3 menu entries (Browse/Favorites/Recent) were
    added to `refreshStationListDisplay()` in round 26, I never updated
    `printCurrentScreen()`'s own separate loop to match - it kept
    iterating only `stationManager.getStationCount()` real stations, with
    no awareness of the special entries at all. With the hardcoded
    stations now at 0 (round 27), this loop had literally nothing to
    print, which is exactly what showed up as "nothing after Select
    Station:" - not the menu actually being absent, but the one tool
    available for checking it (serial, since the physical panel isn't
    visible here) giving a stale answer.
    
    Fixed by mirroring the real display path's logic exactly in
    `printCurrentScreen()` too. Confirmed separately that
    `refreshStationListDisplay()` (the real draw path) is in fact called
    right after WiFi setup completes at boot - meaning the physical
    e-paper panel was very likely showing the menu correctly the entire
    time; only the serial debug view was out of sync.
    
    This is a pattern worth watching for going forward: any future
    change to what a screen shows needs updating in both places (the
    real `Display` draw call and `printCurrentScreen()`'s mirror of it),
    since nothing enforces them staying in sync automatically.

---

# Round 29: Discovery Flow Fixes (Batch of 7 Reported Issues)

53. **#1 - Browse by country OR genre independently.** Rather than a
    separate up-front choice screen, added "Any Country" as the first
    entry in the country list - mirroring the "Any Genre" entry that
    already existed in the tag list. This gives all four combinations
    (country+genre, country-only, genre-only, or neither/just popularity)
    with far less new state to track than a dedicated choice screen would
    have needed.

54. **#2 - Countries now alphabetical**, via `&order=name` on the API
    call - the server sorts, no client-side sort needed for ~250 entries.

55. **#3 - "Back" was already implemented (long-press) but completely
    undiscoverable.** Added an explicit `footer` parameter to
    `showStationList()` (same pattern as the earlier `title` parameter)
    and gave every browse/favorites/recent screen a clear on-screen hint
    (e.g. "Click=Play Space=+Fav Long=Back"). Also fixed the boot-time
    serial banner, which unconditionally claimed long-press means
    Bluetooth - true only in the main list, misleading everywhere else in
    the browse flow.

56. **#4 - partially addressed.** Confirmed `HTTPClient`'s real default
    timeouts (5s connect, 5s response) - reduced to 3s/3s on every
    connection path (browse API calls and the actual station-connection
    classes). This bounds how long a slow/unreachable server can freeze
    the whole UI, since these calls are synchronous on the main loop by
    design - not eliminated, but shorter and more predictable. Full fix
    (a genuinely non-blocking/background connection) would be a much
    larger architectural change - noted as a known limitation, not
    silently left unaddressed.

Known incomplete: the timeout reduction only applies to HTTPS-scheme
stations and the browse API (my own vendored/written code) - plain
`http://` stations still use the unmodified library's default 5s
timeouts, since that class isn't something I can edit without vendoring a
third pair of files. Flagged rather than left unmentioned.

**#5 (errors on "Any Genre") and #7 (audio not playing) need more
diagnostic detail before fixing** - specifically the exact error text or
serial log from when each happens. Did not want to guess blindly at
either, given that exact mistake caused real problems earlier in this
conversation (the original placeholder station URLs). Asked the user for
the actual log output for both rather than speculating.

**#6 (long country names should scroll, not truncate) needs a bit more
design work** - the existing "now playing" marquee scrolls a single fixed
text region; scrolling a specific row within a broader list while other
rows stay static is a different, less trivial mechanism. Deferred rather
than rushed, given the volume of other fixes in this round already.

---

# Round 30: Favoriting from Now Playing and Recent

57. **Added a way to favorite the currently-playing station** - repurposed
    the Next button/event specifically for discovered stations (browse/
    favorites/recent-originated), since Next already had no sensible
    meaning there (no ordered "next" in a one-off search result - this
    was already disabled a few rounds back). Now: Next = add to
    Favorites when a discovered station is playing; still does its
    normal "skip to next station" job for the fixed list, unaffected.
    Added `AudioPlayer::getCurrentURL()` (was private-only before) to
    actually get the URL to save.

58. **Added the same play+favorite pattern to the Recent screen** as
    already existed on Results - encoder click plays only, Play/Pause
    plays and favorites, for consistency across both screens.

59. **Fixed a footer-drift risk before it could happen again**: the
    Now Playing footer text is shown from 6 different call sites
    (pause, resume, initial buffering x2, periodic status refresh x2) -
    added one `nowPlayingFooter()` helper used by all of them (and by
    `printCurrentScreen()`'s serial mirror too), so the on-screen hint
    and the actual behavior of Next can't drift apart the way the
    station-list serial view briefly did a couple rounds back.

Updated on-screen footers and the boot serial banner to reflect all of
this - "< Back Pause +Fav >" replaces "< Prev Pause Next >" specifically
while a discovered station plays.

---

# Round 31: Alphabet Navigation, Freeze Fix, and a Timeout Correction

60. **TO-DO #1 - Alphabet-first country navigation.** Scrolling through
    ~250 countries one at a time was taking minutes to reach anything
    late in the alphabet. Added a new letter screen (Any Country, A-Z)
    between "Browse All Stations" and the country list - filters the
    already-fetched countries client-side by first letter (no extra
    network call needed), then shows just that subset. "Any Country" at
    the top still works exactly as before (skips straight to genre).
    Back-navigation from the genre screen now correctly returns to
    whichever screen you actually came from (letter screen if you picked
    "Any Country", filtered country list otherwise) via a new
    `skippedCountrySelection` flag.

61. **Issue #2 - ESP32 freezing when browsing while a station plays.**
    Real, serious bug: a background audio stream and a foreground HTTPS
    browse-API call were competing for the same limited RAM/network
    resources simultaneously (potentially two TLS sessions at once - this
    board has no PSRAM, and TLS overhead has been a recurring theme all
    session). Fixed by stopping playback before entering the browse flow
    - `audioPlayer.stop()` now runs once at the "Browse All Stations"
    entry point, covering the whole flow downstream. Confirmed `stop()`
    is safe to call even when nothing is playing (already null-checked
    internally) - not just called defensively without checking.

62. **Issue #1 - "no stations found" for Romania + Any Genre, verified
    against the actual API docs before concluding anything.** First
    hypothesis (codec parameter case-sensitivity) was checked directly
    against the official API documentation's own example, which uses
    `codec=mp3` lowercase too - ruled that out with evidence rather than
    guessing further. Real cause: I'd reduced the browse API's timeout to
    3s in round 29 specifically to bound the station-connection freeze -
    but a broader query (country-only, no genre to narrow the candidate
    set) gives the server more results to rank by popularity before
    responding, plausibly exceeding that tight a timeout. That 3s value
    was tuned for a different problem (detecting a dead stream server
    fast) and never reconsidered for the browse API's own needs.
    Increased to 8s for the browse API specifically, leaving the
    station-connection timeout at 3s (different, correct tradeoff there).

---

# Round 32: The Real Fix, Confirmed by Actual Measurement

63. **"No stations found" for Romania - root cause confirmed by direct
    measurement, not another guess.** Asked the user to fetch the exact
    query this code builds directly in their own browser (same technique
    that found West City Radio's real URL back in round 12) - it came
    back with a large, completely valid list of real Romanian MP3
    stations, and took ~15 seconds to respond. That's the whole bug: a
    country-only search (no genre to narrow the candidate set the server
    has to rank by popularity) genuinely takes around 15s, and the
    timeout had only been raised to 8s in round 31 - itself an
    improvement over the original 3s, but still short of what this
    specific kind of query actually needs. Increased to 20s, a value with
    real measured margin behind it instead of another estimate.
    
    Also updated the loading screen for this specific step to say
    "(15-20 seconds)" so a long wait doesn't look like a hang - added
    two-line support to `showLoading()` for this, since the full
    "up to 15-20 seconds" message doesn't fit on one line at this font
    size and `drawCenteredText()` has no wrapping/clipping handling of
    its own.
    
    Worth knowing going forward: a specific genre alongside a country
    should search a narrower candidate set and likely respond faster in
    practice, even though the timeout now safely covers the broad,
    unfiltered case either way.

---

# Round 33: Self-Diagnosing Browse Errors + Settling Delay

64. **Ruled out one real hypothesis with evidence**: `WiFiClientSecure`'s
    own TLS handshake timeout (a separate setting from HTTPClient's,
    confirmed via `setHandshakeTimeout()` in the core source) defaults
    to 120 seconds - far longer than the "fails after a few seconds"
    symptom, so that's not the cause. Checked before ruling it in or out.

65. **Made browse failures self-diagnosing** rather than all showing the
    same generic message. Added `StationBrowser::getLastError()`,
    populated with the specific reason at every failure point:
    `HTTPClient`'s own internal error code (negative codes like -8 mean
    "too little RAM", -11 means read timeout - very different problems
    that were previously indistinguishable), the specific ArduinoJson
    parse error if the response was malformed, or "0 X returned" if the
    request technically succeeded but matched nothing. Every browse error
    screen now shows this real reason instead of a generic one - if this
    happens again, the actual cause is on the screen and in the log
    without needing another round of asking for a capture.

66. **Added heap diagnostics at the two most likely trouble points** -
    right after stopping playback and right before/after every browse
    HTTPS request - logging both total free heap and (critically) the
    largest single contiguous block available. Given this board's
    recurring RAM constraints all session (no PSRAM, TLS overhead flagged
    repeatedly), a healthy total-free number with a small largest-block
    number would be a very specific, confirmable sign of heap
    fragmentation - visible directly in the log now rather than inferred.

67. **Added a 200ms settling delay after stopping playback**, before the
    first browse network call. `stop()` deletes several objects (decoder,
    buffer, source) and immediately demanding a new large contiguous
    allocation (a fresh TLS session) right after freeing one is exactly
    the kind of sequence that can expose fragmentation even when total
    free memory looks adequate. Cheap, low-risk insurance either way.

---

# Round 34: Fragmentation Confirmed, Two Real Fixes Applied

Real serial log with the round-33 heap diagnostics gave concrete, not
guessed, findings:

68. **Heap fragmentation confirmed empirically.** Largest contiguous
    block: 98KB after stopping playback -> 94KB before the countries
    request -> 53KB before the tags request -> still 53KB before search.
    Total free heap barely moved (~113KB -> ~109KB) - this is a textbook
    fragmentation signature: memory being freed correctly in total bytes,
    but not coalescing back into one large usable block. Root cause:
    `httpsGetJson()` constructed a brand new `WiFiClientSecure`+
    `HTTPClient` pair (each allocating its own internal TLS session
    buffers) for every single call - three separate alloc/free cycles
    back to back for country -> tag -> search. Fixed by making both
    persistent members of `StationBrowser`, reused across all three
    calls instead of torn down and rebuilt each time, with a defensive
    `http.end()` at the start of each call to guarantee clean state.

69. **A second, different bug found in the same log**: the search request
    itself actually succeeded - no HTTP error, no JSON parse error - yet
    still reported 0 stations, despite the exact same query returning 30
    valid Romanian stations when fetched directly in a browser. Verified
    my filter syntax against a real matching test case in ArduinoJson's
    own test suite (`"filter members of object in array"`, using a bare
    top-level array shape) - confirmed correct, not the bug. Added
    `doc.size()` and `doc.memoryUsage()` logging right after every
    successful parse, which will definitively show on the next attempt
    whether the parsed array is genuinely empty (a parser/memory issue
    despite no reported error) or has entries that something in the
    extraction loop is silently skipping - narrows the remaining mystery
    to one specific, checkable question rather than an open one.

Two real, evidence-based changes went in this round rather than another
guess: the object-reuse fix directly addresses the confirmed
fragmentation, and the new size/memoryUsage logging will pin down the
second bug precisely on the next test, whichever of the two possible
causes it turns out to be.

---

# Round 35: Chunked-Encoding Hypothesis - Definitive Data, Targeted Fix

The new logging from round 34 gave a conclusive answer to the open
question: `Parsed OK - 0 top-level elements` for the search request -
genuinely an empty array with no error reported, not an extraction-loop
bug (there was nothing to extract). This rules out the filter/loop
entirely and points at how the response itself is being read.

70. **Working theory, checked against real HTTPClient source before
    acting on it**: the search response is the only one of the three
    large/dynamic enough to plausibly be served with
    `Transfer-Encoding: chunked` (30 full station records with ~25
    fields each, versus simple country/tag lists). Confirmed real,
    explicit chunked-encoding handling exists in `HTTPClient`'s source -
    but `getStream()` (used for the RAM-efficient streaming parse) may
    expose the raw underlying network stream, chunk-size markers and
    all, bypassing that de-chunking logic entirely. That would produce
    exactly this symptom: malformed-looking input that doesn't trigger a
    hard parse error but never reaches valid array content either.
    
    Switched to `http.getString()` first, then parsing that string -
    `getString()` must fully de-chunk internally to produce a correct
    combined body, so this sidesteps the issue if chunking is the cause.
    Trades the streaming-parse RAM optimization for correctness on this
    specific call - confirmed heap headroom at this point in the flow
    (~65KB free, per round 34's actual measurement) comfortably covers a
    temporary full-body buffer for this occasional, non-continuous
    operation. Added body-length logging so the next test will show
    directly whether the raw bytes are now being received as expected.

Being upfront: this is a well-reasoned, evidence-informed fix based on
real HTTPClient source and the actual measured data, not a guaranteed
one - correctly identifying "parses to empty with no error" as the
symptom doesn't by itself prove chunked encoding is the mechanism, only
that it's a strong, checkable candidate given what's known so far. The
next test will confirm or rule it out directly via the new logging.

---

# Round 36: Chunked-Encoding Fix Confirmed, New Handoff Issue Found

The round-35 fix worked - confirmed by the actual log: `Found 30 stations`.
Progress, but the log revealed a real, different problem right after:
playing a discovered station (Digi24FM) failed
(`start_ssl_client: -1`), and the *next* browse attempt showed
fragmentation actively getting worse (largest block 45KB -> 34KB, despite
higher total free memory - a classic fragmentation signature, not a
"running low overall" one).

71. **Real cause, verified before fixing**: the browse API's persistent
    `httpsClient`/`http` objects (made persistent in round 34
    specifically to reduce fragmentation *within* the browse flow) can
    still be holding onto TLS session resources even once "idle" -
    `http.end()` alone doesn't guarantee this is fully released.
    Confirmed `NetworkClient::stop()` exists as a more thorough teardown
    (checked directly in the core source). Right as you go to actually
    play a discovered station, the audio stream needs its own completely
    separate, fresh TLS connection - and the still-held browse resources
    were competing with it for the same limited RAM at exactly the wrong
    moment.
    
    Added `StationBrowser::releaseConnection()` (`http.end()` +
    `httpsClient.stop()`, with heap logging), called right before every
    discovered-station playback attempt (`playDiscoveredStation()` -
    covers Results, Favorites, and Recent uniformly, since all three
    already funnel through it), followed by the same 200ms settling delay
    already used when entering Browse.
    
    Also checked whether my own vendored HTTPS stream class was cleaning
    up properly on a failed connection attempt - confirmed it does call
    `http.end()` on failure. If cleanup is still incomplete somewhere,
    that gap would be inside mbedTLS/WiFiClientSecure's own handshake
    failure path, not something fixable by editing more of this code.

Being direct about where this stands: fighting heap fragmentation on a
board with no PSRAM, while running both HTTPS browsing and HTTPS
streaming, is a genuinely hard, ongoing problem - this is a real,
targeted fix for a confirmed cause, not a claim that fragmentation is
fully solved. Worth testing the same sequence again (browse, play
something, and if that works, try browsing again afterward) to see
whether this specific handoff is now clean.

---

# Round 37: Actual Memory Management - Root Cause Fixed, Not Just Worked Around

Two real changes, both directly evidence-based rather than another guess.

72. **The actual, confirmed root cause of "IncompleteInput", fixed at the
    source.** Checked `HTTPClient::getString()`'s real implementation:
    it can only pre-reserve its buffer when Content-Length is known
    upfront - for a chunked response (unknown length, exactly what a
    dynamically-sized search result is), that reserve is a silent no-op,
    and the buffer has to grow incrementally as data streams in instead.
    On this board's confirmed, recurring fragmented heap, a reallocation
    during that growth failing to find a big enough contiguous block
    would silently truncate the result at whatever size it reached -
    exactly matching the repeated "IncompleteInput" failures with a
    shorter-than-expected body.
    Fixed by reserving a generous fixed capacity (`RESPONSE_BUFFER_RESERVE`,
    24KB) *before* any data arrives, using `HTTPClient::writeToStream()`
    (confirmed public, and the same method `getString()` calls internally
    - so still properly de-chunked) instead of letting a `String` grow
    incrementally. This is a fix for the confirmed mechanism, not another
    layer of guessing.
    Also reduced `MAX_BROWSE_RESULTS` from 30 to 20 - directly shrinks the
    response size this buffer needs to hold, giving more margin within
    this board's typical ~45KB largest-contiguous-block reality. Still
    plenty for a list scrolled one at a time on a small e-paper screen.

73. **Following through on the broader memory-management idea from last
    message.** `countries[250]`, `tags[40]`, and `results[]` were
    permanent static arrays - reserved for the device's entire runtime
    (~18KB), whether actively browsing or just listening to a station.
    Converted to lazily-allocated pointers (allocated on first use inside
    each fetch function) with a new `freeBrowseMemory()`, called at both
    places the browse flow actually ends: backing out via long-press from
    the letter screen, and successfully selecting a station to play
    (alongside the existing `releaseConnection()` call). Gives that ~18KB
    back the rest of the time - including to the audio stream's own TLS
    connection, which needs it most.

Both changes directly reduce the kind of allocate/free churn that's been
causing fragmentation all session, rather than working around individual
symptoms of it.

---

# Round 38: Compile Fix - StreamString Ambiguity

74. **`ambiguous template instantiation for Reader<StreamString, void>`** -
    real compile error introduced by round 37's fix. `StreamString`
    inherits from both `Stream` and `String` simultaneously (that's the
    whole point of the class - reservable like a String, writable like a
    Stream), but `deserializeJson()` has separate template specializations
    keyed on each of those base classes. Passing the `StreamString` object
    directly made both match at once, which the compiler can't resolve.
    Fixed by extracting the plain, null-terminated C-string via `c_str()`
    instead of passing the object itself - a raw pointer matches neither
    specialization, sidestepping the conflict. This exact (pointer,
    Filter) pattern was already verified working in this project's very
    first version of this function, before ever switching to the
    Stream-based approach - not a new, unverified guess.

75. **Also cleared up something that had been quietly wrong all session**:
    the build log showed `memoryUsage()` is deprecated and always returns
    zero in this ArduinoJson version - which is exactly why every single
    log this whole time showed "0 bytes used". That was never a real
    signal, just a dead stub. Replaced it with the actual heap state
    (free + largest block) right after parsing, which is genuinely useful
    rather than a constant, meaningless zero.

---

# Round 39: Real Crash - Getting Precise Diagnostics Before Guessing

A genuine hardware-level crash occurred: `Guru Meditation Error: Core 1
panic'ed (IntegerDivideByZero)`, while playing a station discovered via
Browse (`Digi FM`, the HTTPS variant specifically -
`https://edge76.rcs-rds.ro/digifm/digifm.mp3` - a different entry than
the HTTP "DIGI FM" tested earlier). Happened partway through playback,
after ICY metadata had already been parsed successfully, suggesting
something encountered later in the stream (not the initial connection)
triggered a division by a value that turned out to be zero - most likely
deep inside the third-party Helix MP3 decoder's own internal frame
calculations, not code in this project directly.

This is a hardware trap, not a C++ exception - it can't be caught and
recovered from after the fact, only prevented by keeping whatever caused
it from happening in the first place. The backtrace in the log is just
raw addresses, useless without the exact matching compiled binary to
resolve them against.

76. **Enabled PlatformIO's `esp32_exception_decoder` monitor filter by
    default** (`monitor_filters = esp32_exception_decoder` in
    `platformio.ini`) rather than guess at the cause from raw addresses.
    This automatically translates any future crash's backtrace into real
    function names and line numbers. Deliberately not making a speculative
    fix this round - dividing by a value from deep inside a third-party
    decoder is exactly the kind of thing worth diagnosing precisely rather
    than guessing at, given how much of this session's actual progress
    has come from insisting on real evidence over assumptions.

Next step if this happens again: the log will show the actual function
and line where the crash occurred, which will make the real fix obvious
rather than speculative.

---

# Round 40: Multi-Mirror Fallback for the Browse API

Real `HTTP GET failed, code 503` in testing - a genuine server-generated
status (not something a flaky connection or client bug could produce;
that's specifically the server, or something in front of it, saying
"temporarily unavailable" for a completed request), but transient - the
same request worked later. This is exactly the scenario the official
radio-browser.info docs warn about when recommending against hardcoding
a single mirror - noted as a deliberate simplification back in round 26,
which turned out to matter in practice.

77. **Added fallback across multiple known mirrors** rather than
    implementing full DNS-based server discovery (a meaningfully bigger
    architectural change - parsing multiple raw DNS A records). Used the
    5 real mirror hostnames already confirmed via search during earlier
    development in this conversation (de1, de2, fi1, nl1, at1), tried in
    sequence.
    
    Deliberately selective about *when* to retry a different mirror:
    split the request logic into an inner single-host attempt and an
    outer loop that only advances to the next mirror for
    connection/server-level failures (couldn't connect, non-200 status) -
    not for client-side issues (buffer reservation, JSON parsing) that
    would fail identically regardless of which server responded. Retrying
    all 5 mirrors for a problem that isn't server-specific would just
    burn up to 100 seconds (5 x the 20s timeout) for no benefit.

---

# Round 41: PSRAM Enable + AAC in Browse (Scoped, "Start Slow")

Deliberately scoped to just these two things, per request - pin
reassignment for the wire-tapped GPIOs is a separate follow-up once the
actual soldering is done and the real pin mapping is known.

78. **PSRAM enabled**, verified against official PlatformIO docs and a
    real working esp32cam project's config (not guessed): `board =
    esp32cam`, `-DBOARD_HAS_PSRAM`, `-mfix-esp32-psram-cache-issue`,
    `board_build.arduino.memory_type = qio_qspi`. Added explicit
    `psramFound()`/`ESP.getPsramSize()` logging right at boot - this is
    the actual test, not an assumption baked into the rest of the code.
    
    The 16KB audio ring buffer now allocates from PSRAM via `ps_malloc()`
    when available, with a real fallback to regular heap (and a clean
    abort, not a crash, if both fail) - not assumed to always succeed.
    Verified `AudioFileSourceBuffer`'s pre-allocated-buffer constructor
    sets `deallocateBuffer=false` directly in its source before relying
    on it, so this code now correctly owns and frees that memory itself
    rather than risking a leak or double-free.
    
    Deliberately did *not* move the browse API's response buffer to
    PSRAM this round - that would mean abandoning the `StreamString`-based
    de-chunking fix from round 35 for a manual read loop, reintroducing
    the exact "chunk markers leak into the parser" risk that took several
    rounds to actually fix. Not worth the trade for a buffer that's only
    used occasionally, versus the audio buffer which matters continuously.
    
    **Honest caveat**: confirmed via direct research that plain
    `malloc()`/`new` does not automatically use PSRAM - this matters
    because the AAC+/SBR decoder that crashed back in round 18/23 uses
    plain `new`/`malloc()` internally, not `ps_malloc()`. Moving the audio
    buffer to PSRAM frees real internal headroom that crash could now fit
    into, but that's not a guarantee the way explicitly making the
    decoder's own allocation PSRAM-aware would be (a deeper ESP-IDF
    config change, not attempted this round). Worth specifically testing
    an AAC+/SBR station to see if this residual risk is actually gone.

79. **AAC now real in Browse, not MP3-only by design.** The `codec=mp3`
    restriction on search results existed specifically because AAC+/SBR
    wasn't safely supportable without PSRAM - with PSRAM now available,
    that restriction is gone. Search results now read the real `codec`
    field per station and map it to MP3 or AAC (case-insensitive, covers
    "AAC+"/"AACP" too), skipping anything neither decoder supports (OGG,
    FLAC, WMA, etc.) rather than adding a result that would fail to play.
    `playDiscoveredStation()` (shared by Results, Favorites, and Recent)
    now threads the real codec through end to end instead of hardcoding
    MP3 - `RadioStation` already tracked codec per entry from earlier
    work, this just makes the code actually use it.

Next real test: check the boot log for PSRAM detection first, then try a
few AAC stations from Browse (should just work), then specifically an
AAC+/HE-AAC one if you can identify one, to see whether the SBR crash
risk is actually gone or still lurking.

---

# Round 42: Full Pin Remapping for ESP32-CAM

The user identified GPIO16/17 (PSRAM) and GPIO4 (flash LED) as blocked -
checked this against a detailed ESP32-CAM pin reference rather than just
fixing those two in isolation, since the project's other pins were chosen
for a generic DevKit V1 board and never checked against ESP32-CAM's
specific constraints.

80. **Found a third, real conflict beyond what was reported**: I2S_DIN
    was on GPIO33, which turns out to be the board's own onboard status
    LED (not a camera pin, so "camera unused" doesn't free it) - would
    have caused that LED to flicker with whatever audio data happened to
    be on the bus.

81. **Confirmed the rest of the existing pins are genuinely camera-only**
    (Y2/Y3/Y4/Y5/Y6/Y7/Y8/Y9/HREF/PCLK/SIOD/SIOC/PWDN/VSYNC) - safe to
    keep since the camera isn't being used, cross-checked against the
    reference rather than assumed.

82. **Reassigned the three genuinely-blocked pins**: RST 16->19, DC
    17->21 (both were PSRAM), BUSY 4->36 (was the flash LED; 36 happens
    to be one of the classic ESP32's input-only pins, which is actually
    a clean match since BUSY only ever needs reading, never driving),
    DIN 33->22 (was the onboard LED; needed an output-capable pin since
    I2S DIN is ESP32-to-DAC, so couldn't reuse an input-only pin like the
    one BUSY moved to).

Flagged one pin (ENCODER_SW, GPIO14) as slightly less certain than the
rest - not found on the camera/PSRAM/LED conflict lists checked, but
worth confirming once wired rather than treating as fully verified like
the others.

---

# Round 43: Retargeted for Genuine ESP32-WROVER-DEV Board

The ESP32-CAM wire-tap plan is retired - the user acquired a genuine
ESP32-WROVER-DEV board (ESP32-WROVER-E module), which has PSRAM properly
integrated in the chip package (unlike the CAM board's separate discrete
PSRAM chip situation from round 42) and standard, fully accessible
header pins.

83. **`board` target changed from `esp32cam` to `esp32dev`** - verified
    via a real PlatformIO forum thread with the identical situation
    (a generic WROVER DevKitC-style board), which explicitly confirmed
    `esp-wrover-kit` is the wrong choice here (that's specifically for
    Espressif's own full-featured kit with a built-in LCD, microSD slot,
    and JTAG debugger - none of which this simple board has, and using
    it gives wrong flash-size defaults). PSRAM build flags kept as-is,
    since they're chip-family flags (classic ESP32 + QSPI PSRAM), not
    board-specific ones.

84. **Pin situation is much simpler on this board than ESP32-CAM was.**
    Confirmed via Espressif's own ESP-WROVER-KIT documentation: this
    module's PSRAM only uses GPIO16/17, and "by default the two GPIOs
    are not broken out to the board's pin headers" - unlike the CAM
    board's tangle of camera pins, a flash-LED conflict, an onboard
    status LED conflict, and PSRAM data lines sharing the flash bus.
    Only RST and DC (which sat on GPIO16/17) needed to move - to GPIO19
    and GPIO21, reusing the exact values already chosen for the CAM plan
    since they were free there too. BUSY and DIN reverted to their
    original DevKit V1 pins (4 and 33) - the conflicts that moved them
    away before (flash LED, onboard LED) were specific to the CAM board
    and don't exist here.

85. **Investigated properly whether AAC+/SBR's remaining risk (flagged
    honestly in round 41) could now be fully closed**, given this is a
    confirmed real WROVER board rather than an uncertain wire-tap.
    Found a PlatformIO Arduino-framework user with the exact same build
    flags this project uses confirming directly: "PSRAM are available
    for ps_malloc(), but not for malloc()" - and a very recent (Oct 2024)
    open arduino-esp32 GitHub issue showing experienced developers still
    struggling to get deeper SPIRAM-for-malloc config to apply reliably
    under the Arduino framework at all. Didn't attempt a risky,
    poorly-supported deeper fix and call it resolved - this remains an
    honest, not-fully-guaranteed situation for AAC+/SBR specifically,
    though meaningfully better than before given how much has already
    moved out of internal SRAM this session (the audio buffer via
    ps_malloc, the browse arrays now on-demand rather than permanent).

Everything else - AAC-aware browsing, Recent/Favorites, the browse flow,
memory management - is pure software untouched by this hardware switch,
confirmed still intact.
