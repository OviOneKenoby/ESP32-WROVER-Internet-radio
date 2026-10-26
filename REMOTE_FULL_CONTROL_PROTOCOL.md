# REMOTE Full Control — TUNER Protocol and Ownership Record

Status: **offline partial implementation; Radio Browser blocked**  
Binding product behavior: REMOTE repository
`docs/REMOTE_FULL_CONTROL_SPEC.md`

This record describes the TUNER implementation. It does not relax the binding
owner-approved behavior. If implementation constraints conflict with that
specification, the conflict must be reported rather than silently changing the
behavior.

## Authority and compatibility

TUNER remains authoritative for source, playback, metadata, volume, station
collections and persistence. Physical TUNER controls have priority. REMOTE
commands are semantic authenticated operations, never injected raw button or
encoder events. A catalog revision changed by local activity makes a pending
REMOTE mutation stale; it is rejected, refreshed and never retried
automatically.

REMOTE navigation is intentionally independent. REMOTE page/row/scroll/filter
focus does not move the TUNER cursor, change its physical menu page, or request
an e-paper refresh. The read-only menu projection is informational. Both
interfaces remain usable simultaneously while shared media/catalog state is
still synchronized authoritatively.

RTCP/1 framing, AES-GCM authentication, replay protection, request correlation,
D-009 IDs 1-11 and BTX IDs 12-15 are unchanged. Identity suffixes negotiate
optional Full Control safely:

- `+fc1`: source switching, version 1.0;
- `+fc2`: read-only canonical menu projection, version 1.1;
- `+fc3`: bounded station catalogs, version 1.2.

Legacy clients ignore the identity text and never send optional records.

## Records

| ID | Direction | Layout and bound |
| --- | --- | --- |
| 16 | REMOTE -> TUNER | Capability request, 1 byte |
| 17 | TUNER -> REMOTE | Version/features/freshness/revision, 20 bytes |
| 18 | REMOTE -> TUNER | Source-state request, 1 byte |
| 19 | TUNER -> REMOTE | Source state, 24 bytes |
| 20 | REMOTE -> TUNER | Correlated source select and expected generation, 18 bytes |
| 21 | REMOTE -> TUNER | Menu-state request, 1 byte |
| 22 | TUNER -> REMOTE | Canonical menu window, at most 288 bytes |
| 23 | REMOTE -> TUNER | Catalog collection/offset/limit query, 5 bytes |
| 24 | TUNER -> REMOTE | Catalog page, at most 288 bytes |
| 25 | REMOTE -> TUNER | Correlated semantic catalog command, 21 bytes |

Command completion reuses correlated result ID 8. Catalog errors 8 and 9 are
the bounded already-present and full results; existing meanings 1-7 remain
unchanged.

Catalog pages contain capacity, count, window start, at most five records, a
nonzero sequence and a nonzero global catalog revision. Each record contains
only its positional token, active flag and a validated UTF-8 name no longer
than 48 bytes. URLs, credentials and packet contents are never exported.
Collections retain their existing capacities: 15 Saved Stations, 10 Favorites
and 5 Recent.

## Execution and persistence

The RTCP task admits at most one fixed-size catalog request into a fixed
mailbox. The application main loop alone executes StationManager and playback
work. Completion returns through a bounded response slot. Disconnect clears
pending work. No dynamic diagnostic allocation, automatic retry or mutation
replay is introduced.

Play uses the existing radio playback owner. Remove applies only to Saved or
Favorites and requires a fresh revision. Add-to-Favorites applies to Recent,
retains the existing capacity/deduplication behavior and never interrupts
playback. Recent remains automatic. Station creation, Wi-Fi provisioning and
timezone remain web-only. Existing NVS namespaces, capacities and binary
serialization are unchanged.

Source switching keeps its existing generation fence. Bluetooth-to-Radio uses
the restartable Bluetooth cleanup and returns to Station Select without
resuming a station. Same-source selection is idempotent. Physical source/menu
changes are published without forcing REMOTE navigation.

## Memory decisions

Catalog wire objects, projection windows and mailbox storage are fixed-size.
Responses remain within the existing 512-byte plaintext bound. StationManager
continues using its verified lifetime PSRAM record block; no collection is
duplicated into permanent internal RAM. Runtime DMA headroom is not inferred
from the static build result and remains a hardware-validation requirement.

## Radio Browser blocker

`StationBrowser::httpsGetJsonOnce()` in `src/browser.cpp` is synchronous on the
application loop, configures 20-second connect and read timeouts, buffers the
whole body in `StreamString`, then performs dynamic JSON deserialization. RTCP
response deadlines are 5 seconds in `src/rtcp_server.cpp`. The platform also
has verified internal-memory low-water marks near 1-2 KiB.

There is no verified cancellation or cooperative Browser job. Therefore no
Browser message or command is implemented or advertised. Resolving this block
requires a bounded cooperative ownership design, stale-result handling,
memory proof and long-HTTPS/RTCP coexistence tests. A new large-stack task must
not be added without evidence.

## Verification checkpoint

Implementation commit:
`c217275262266b709a244870dd0f317ce135fdab`.

- Full-Control source/menu model: PASS.
- Catalog model: PASS.
- D-009: PASS, 23/23.
- BTX: PASS, 46/46.
- StationManager PSRAM: PASS, 62/62.
- Production environment `esp32-lwip-psram`: PASS.
- Static RAM: 79,264 / 327,680 bytes.
- Linked flash: 2,072,557 / 3,145,728 bytes.
- BIN: 2,079,136 bytes; SHA-256
  `7749b1f0f819773c2ee18e964115cbea55ccefa6aa92e388d5e06b31deaf2315`.
- ELF: 30,428,856 bytes; SHA-256
  `ab5c1b58bae98baaba0d617fd87ac3a41e8db666b2586572fea1b1df439a19d0`.
- Exact-image slot headroom: 1,066,592 bytes.
- Hardware: NOT TESTED; no device was accessed.

Installation must be separately authorized. A safe staged hardware procedure
installs TUNER first, verifies legacy D-009/BTX behavior, then installs REMOTE
and validates source, lists, mutations, concurrency, sleep/wake and reconnect.
Rollback artifacts must be verified before either write. The installed TUNER
rollback is implementation `dbf9896`, 2,072,416 bytes, SHA-256
`6f02cdfd1576831a5d4a40f54eff88dcc3960fcdf72545d0909b0525cbceabf6`.
The installed REMOTE rollback is implementation `596cb71`, 2,583,008 bytes,
SHA-256
`fad87bb2f50142f3758945c5a2276a2535008aec4f32a5ec29f021faa95f3b3f`, with
its verified full 16 MiB backup. A separately authorized rollback restores
REMOTE first, then TUNER, application-only with independent readback; it never
touches NVS, pairing, bootloaders or partition tables.
