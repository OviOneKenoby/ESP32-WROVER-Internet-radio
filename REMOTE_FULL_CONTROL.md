# REMOTE Full Control

## 2026-10-31 Browser playback hardware checkpoint

TUNER `12b1b24` and REMOTE `8fdabc6` passed exact application readback and one
complete hardware Browser flow through Country, Name, Genre, Results and
station selection. The selected MP3 station started once; audio, metadata,
PLAYING and volume were correct, overlays cleared, and no disconnect card was
shown. TUNER recorded HTTP 200, three completed Browser jobs, zero failures and
no new retained RTCP close. The fixed deferred Browser-start path is therefore
hardware PASS for this bounded scenario. Extended soak and exhaustive
error/cancellation/empty-result paths are not claimed.

## 2026-10-31 Browser playback response correction

Implementation `12b1b24` extends the existing fixed deferred station-start
slot to Radio Browser PLAY. Revision fencing and exactly-once admission remain
on the RTCP owner; stream opening remains on the persistent audio task. This
prevents synchronous network/audio startup from delaying the encrypted command
result beyond REMOTE's response deadline. Browser navigation, HTTPS transport,
physical TUNER priority and catalog semantics are unchanged. Offline focused
tests and the production build pass; hardware validation is pending.

## 2026-10-29 action-time transport checkpoint

Hardware on the `0ed0ffb` / `9142401` pair verified Bluetooth control and
authority, Radio recovery, authoritative volume, sleep/wake restoration and
continuous audio. Favorite playback itself now succeeds immediately, but its
post-command e-paper redraw coincided with a REMOTE response timeout and reset;
Radio Browser also failed before HTTP status at its one-second per-transport
fence. TUNER source `579f412691df6ffc4d03513d1c64f7a65ea18320` defers that
redraw until authoritative state is published and gives the asynchronous
Browser transport a bounded five-second operation window across the official
DNS endpoint plus two resolving fallbacks. The 30-second whole-job deadline,
RTCP protocol/security, revision fencing and no-replay rules are unchanged.

Offline tests and production build pass. The candidate BIN is 2,135,920 bytes,
SHA-256 `663983a35e060eb1fee6943512dc60d22ef7414255725a46f329b7c5915aed3c`.
It is **not installed**. Hardware acceptance requires one Favorite start with
both close histories unchanged and one complete Country -> Name -> Genre ->
Results Browser flow.

## Compatibility and ownership

TUNER remains the sole application owner. RTCP/1 authentication, record
counters, replay protection, request correlation, and the existing D-009 and
BTX records are unchanged. A bounded `+fc1` suffix in the existing identity
text tells the Milestone 1 REMOTE that the separate Full-Control capability
request is safe. Milestone 2 advances that suffix to `+fc2`; a Milestone 1
REMOTE therefore stays on the unchanged D-009/BTX records instead of parsing
new feature bits. The current REMOTE recognizes both suffixes. Older clients
accept and ignore identity text; BTX feature bits remain unchanged. An older
TUNER therefore never receives an unknown record.

## Milestone matrix

| Milestone | Status | Notes |
| --- | --- | --- |
| 1 Source switching | PASS (offline) | Capability/state/select records, generation fence, one-slot owner mailbox, idempotency, Radio/Bluetooth lifecycle. Hardware pending. |
| 2 Read-only menu projection | PASS (offline) | Fixed five-item authenticated window, canonical physical mode/cursor, stable revision, bounded UTF-8 names; hardware pending. |
| 3 Semantic navigation | PENDING | Not implemented. |
| 4 Saved/Favorites/Recent | PENDING | Not implemented. |
| 5 Radio Browser | PENDING | Blocking HTTPS path must not be bridged synchronously. |
| 6 UI/lifecycle integration | PARTIAL | Source view implemented on REMOTE; broader lists pending. |
| 7 Final verification | PENDING | Requires all safe milestones. |

## Source-switch wire records

All integers are big-endian and all payloads remain below the existing
512-byte authenticated plaintext limit.

- `16` Full-Control capability request: 1 byte.
- `17` capability response: version, 32-bit feature mask, 2,000 ms freshness,
  and 64-bit capability revision; 20 bytes.
- `18` source-state request: 1 byte.
- `19` source-state response: source, transition, Bluetooth connection flag,
  last sanitized error, sample sequence, and source generation; 24 bytes.
- `20` source-select request: request ID, Radio/Bluetooth target, and expected
  source generation; 18 bytes. The response is existing correlated record `8`.

Only explicit Radio and Bluetooth targets are valid. The main-loop owner
performs the restartable audio transition. Switching to Radio stops at Station
Select and does not resume a station. A physical source change advances the
authoritative generation; stale REMOTE requests are rejected and never
retried. Mailboxes are cleared on disconnect.

Milestone 1 static extension storage was compile-time bounded to 48 bytes.
Milestone 2 raises the combined source/menu scalar budget to 72 bytes. Runtime DMA
headroom remains hardware-only evidence and is not inferred from static size.

## Read-only menu projection

Full-Control version 1.1 adds feature bit 1 and two records:

- `21` menu-state request: 1 byte.
- `22` menu-state response: 28-byte header followed by at most five entries;
  each entry contains a 16-bit opaque index, active flag and at most 48 bytes
  of validated UTF-8 display text. The absolute maximum is 288 bytes.

The projection reads the actual main-loop-owned mode, selection and underlying
station/browser storage. It never injects an encoder event, allocates memory,
transmits a URL, or mutates TUNER state. A 64-bit revision advances when mode,
cursor, count, visible text or active identity changes. Empty lists are valid.
Only an internal URL comparison determines active discovered/favorite/recent
identity; URLs never leave TUNER. The fixed server-side revision tracker adds
16 bytes; the ephemeral view is built on the existing request stack.

Implementation commit `4cf9b65e00e0a6c5a28f6833ec06a672a046a461`
passes the pinned `esp32-lwip-psram` build. The application is 2,076,272 bytes
(SHA-256 `d308d56a52f46241f0c2eae432eccf0bf2e542f4279cc34c57c9d614b292a982`),
the 30,380,224-byte ELF SHA-256 is
`3816b9cfcbdb14d0a7648c6a6e53f8914a1d2374fa5f99fbc2a8b88b6198a933`,
linked static RAM is 79,208 bytes, and the 3 MiB application slot retains
1,069,456 bytes by exact image length. Relative to Milestone 1 this adds
2,176 application bytes and 80 static RAM bytes.
