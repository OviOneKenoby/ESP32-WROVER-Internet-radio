# A-REMOTE-013 / D-009 TUNER implementation report

Date: 2026-10-20
Status: HARDWARE PARTIAL PASS / RTCP VALIDATION BLOCKED BY STATIC ENDPOINT

## Authority and baseline

- Frozen contract: `RTCP_1_D009_PROPOSED.md`
- Contract SHA-256: `ad6fa343c7ea6363febfffa143c6ffb62e401c19d994abcdaa7287bd0ded6c92`
- Reviewed contract identity: `4237eadf7e641308a77ba673afadd0e39b52e5d1`
- TUNER integration baseline: `4977b49756820edc149acbe1abe4d387a96aa992`
- Executable v1.1.12 source baseline: `d112dd0bbef7d86bd22090ec75d76cad3be77677`
- Hardware-validated rollback application: 2,037,632 bytes, SHA-256
  `6d7f2ebfa833bd09f287b8a7024071d736dfb2bf02356f889de94d7b980fdf6a`
- Implementation source commit: `6a58c036e84863b2b4cf2458ce159d5bc5639466`
- Branch: `tuner/a-remote-013-d009`

The approved contract was hash-verified before implementation. The rollback
binary remains unchanged and outside the public repository.

## Implementation

- Existing RTCP message IDs 1-8 retain their layouts. Capability response
  revision is 2 with the approved bits 3-7.
- ID 9 accepts the exact 19-byte playback command and returns the existing
  correlated ID 8 result with the frozen D-009 error meanings.
- ID 10 requests ID 11, a bounded now-playing response containing the selected
  radio name, combined ICY StreamTitle, context, operation availability,
  sequence, media generation, metadata revision, and validity/truncation flags.
- Play, pause, previous, and next are admitted using fresh state and generation
  fences. Previous/next use the existing guarded saved-list traversal and are
  unavailable for discovered, favorite, recent, Bluetooth, other, and empty
  contexts.
- A fixed mailbox separates record parsing from the application-loop playback
  owner. It permits one command, preserves the request ID, refuses duplicates,
  and emits success only after the owner observes the requested result.
- A 384-byte persistent projection-plus-mailbox budget is enforced at compile
  time. Request handling retains the existing two 556-byte RTCP wire buffers
  and performs no dynamic metadata allocation.
- Metadata is copied under the existing AudioPlayer synchronization, validated
  as UTF-8, rejected when it contains C0/DEL controls, and truncated only on a codepoint
  boundary. `Live stream` is unavailable metadata. No punctuation-based
  artist/title split is attempted.
- Station changes invalidate prior metadata and advance media generation before
  a successor station can be published. Bluetooth and other contexts clear all
  radio-specific fields and operations.

## Verification

Focused and relevant checks passed:

- CCM wire format and compatibility: 24/24
- D-009 projection, fences, traversal, UTF-8, revision, and mailbox: 23/23
- RTCP diagnostics: 11/11
- Existing deterministic correctness/traversal: 266/266
- D-009 integration policy: 12/12
- Existing RTCP latency policy: 9/9
- Existing display-service policy: 9/9
- Existing diagnostics/redaction policy: 14/14

Total: 368/368 assertions/checks passed.

Production build:

- PlatformIO environment: `esp32-lwip-psram`
- Board target: `esp32dev`, 4 MiB flash, 3 MiB application partition
- Framework/toolchain: Arduino-ESP32 3.20017.241212 / Xtensa 8.4.0
- Result: PASS
- Embedded firmware version: `1.1.12`
- Embedded build ID: `6a58c036e848`
- Firmware size: 2,040,704 bytes
- Firmware SHA-256: `1e6c5968f539170656e833d023086c57986b1aa73a6c2fe982dee2b31b1ff1f6a`
- Program usage: 2,034,133 / 3,145,728 bytes (64.7%)
- Application headroom by exact binary span: 1,105,024 bytes
- Static RAM: 87,904 / 327,680 bytes (26.8%)
- Delta from v1.1.12 rollback: +3,072 binary bytes, +392 static RAM bytes

The build emitted only existing dependency/framework warnings: ESP32-A2DP's
AudioTools include advisory and Arduino I2S use of the deprecated ADC attenuation
alias. No D-009 source warning was observed.

## Compatibility and limits

- New TUNER plus old REMOTE retains authenticated IDs 1-8 and volume control;
  the old peer may ignore the upper capability bits. Hardware confirmation is
  still required.
- A future D-009 REMOTE plus old TUNER must capability-gate IDs 9-11 and keep
  playback controls and media unavailable while retaining volume support.
- Current TUNER metadata has one authoritative combined StreamTitle; separate
  artist/title, Bluetooth metadata/AVRCP, source selection, stop, mute, catalogs,
  Favorites mutation, and artwork remain outside D-009.
- A disconnect after command transmission remains ambiguous delivery. Neither
  endpoint retries playback commands automatically.
- The candidate was installed application-only and independently read back on
  2026-10-20. The exact 2,040,704-byte readback matched the candidate SHA-256.
  Boot identity, retained Wi-Fi, MP3 playback, and bounded HTTP diagnostics
  passed. RTCP compatibility testing could not proceed because the existing
  REMOTE association stores the TUNER's former literal DHCP address while the
  TUNER received a different address after reboot. Neither endpoint currently
  implements discovery. This is an endpoint-lifecycle limitation rather than
  a D-009 wire-format or authentication failure. See
  `HARDWARE_VALIDATION_2026-10-20.md`.

## Minimum hardware validation

1. Verify device identity, rollback files, candidate size/hash, and the existing
   application partition; install TUNER application-only at `0x10000` and verify
   the complete readback hash.
2. With the existing REMOTE, confirm authentication, IDs 1-8 volume operation,
   MP3, Bluetooth A2DP, HTTP service, sleep/reconnect, and absence of unexpected
   RTCP disconnects.
3. With a separately approved D-009 REMOTE candidate, verify capability revision
   2, initial/reconnect snapshots, station/StreamTitle projection, freshness,
   and authoritative revision/generation changes.
4. Exercise exactly correlated play/pause and fixed-list previous/next, including
   wrap, empty/stale refusal, discovered-mode refusal, disconnect ambiguity, and
   proof that discovered Next never mutates Favorites.
5. Confirm metadata clears before station/source changes, malformed data fails
   closed, audio remains uninterrupted, and runtime internal heap/stack remain
   proportionate to the accepted v1.1.12 baseline.

No NVS, pairing, bootloader, partition-table, or non-application write
occurred. D-009 end-to-end playback control and now-playing remain untested.
