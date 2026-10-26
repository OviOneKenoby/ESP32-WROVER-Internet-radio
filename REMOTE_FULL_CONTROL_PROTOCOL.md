# REMOTE Full Control — TUNER Protocol and Ownership Record

Status: **Milestone 5 complete offline; hardware validation pending**
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
- `+fc4`: bounded cooperative Radio Browser, version 1.3.

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
| 26 | REMOTE -> TUNER | Browser stage/page/filter/generation request, 14 bytes |
| 27 | TUNER -> REMOTE | Browser state/page with at most five items, at most 296 bytes |
| 28 | REMOTE -> TUNER | Correlated browser command with generation/revision fence, 29 bytes |

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

## Radio Browser execution and memory

The existing physical TUNER browser remains independent. REMOTE Browser work
uses `RemoteBrowser`, a cooperative `esp_http_client` service polled from the
application loop. It never enters `StationBrowser::httpsGetJsonOnce()`, never
stops audio and never creates a new task. HTTPS verifies against the pinned
Arduino certificate bundle. Each job has a 30-second absolute deadline and at
most two mirror attempts.

The response body is capped at 32 KiB and the body, JSON document and retained
catalog arrays require PSRAM with no internal-RAM fallback. Catalog limits are
250 countries, 40 genres and 20 station results. Wire pages expose at most five
validated 48-byte labels and opaque catalog tokens; URLs remain TUNER-local.
Session generation and browser revision reject stale queries and mutations.
Disconnect, REMOTE cancel/exit/filter replacement, and physical TUNER browser
entry cancel or invalidate work. The one-slot command mailbox and correlated
result ID 8 preserve no-retry/no-replay behavior.

The REMOTE filter order is Country (Any/A-Z), Name (Popular/A-Z), Genre
(Any/list), Results. Browser operations are start, select country, select name,
select genre, play, add-to-Favorites and cancel. Play and Favorites mutations
execute on the TUNER application loop and remain fenced against physical TUNER
activity.

## Other memory decisions

Catalog wire objects, projection windows and mailbox storage are fixed-size.
Responses remain within the existing 512-byte plaintext bound. StationManager
continues using its verified lifetime PSRAM record block; no collection is
duplicated into permanent internal RAM. Runtime DMA headroom is not inferred
from the static build result and remains a hardware-validation requirement.

## Verification checkpoint

Milestone 5 implementation commits:
`bfcd2574bdf4d221eb96c26fe1444b82c9bcc096` and
`e51e39db9ab4b6e8ba7c6497765393d39a05c594`.

- Browser protocol model: PASS; cooperative Browser policy: PASS, 9/9.
- D-009 integration policy: PASS, 12/12; BTX integration policy: PASS, 21/21;
  StationManager PSRAM policy: PASS, 16/16.
- Previously passing unchanged Full-Control, catalog, D-009 (23/23), BTX
  (46/46) and StationManager PSRAM (62/62) evidence remains applicable.
- Production environment `esp32-lwip-psram`: PASS.
- Static RAM: 79,928 / 327,680 bytes.
- Linked flash: 2,121,041 / 3,145,728 bytes.
- BIN: 2,127,616 bytes; SHA-256
  `4d68d01fd3ccaffbd463c6ff310e54d16bb42d2e9e102426384cf1d4b63e8e6d`.
- ELF: 31,124,020 bytes; SHA-256
  `f77bc0dea52af9328d305ec88ab221f3c7ad39d4861fe757e49825b61bca3020`.
- Exact-image slot headroom: 1,018,112 bytes.
- Hardware: NOT TESTED; no device was accessed.

Installation must be separately authorized. A safe staged hardware procedure
installs TUNER first, verifies legacy D-009/BTX behavior, then installs REMOTE
and validates source, lists, browser filters/paging/play/Favorites, slow-HTTPS
RTCP/audio coexistence, physical-control priority, cancellation, disconnect,
sleep/wake and reconnect. Runtime internal/DMA and PSRAM low-water evidence is
mandatory; static build numbers do not establish runtime safety.
Rollback artifacts must be verified before either write. The installed TUNER
rollback is implementation `dbf9896`, 2,072,416 bytes, SHA-256
`6f02cdfd1576831a5d4a40f54eff88dcc3960fcdf72545d0909b0525cbceabf6`.
The installed REMOTE rollback is implementation `596cb71`, 2,583,008 bytes,
SHA-256
`fad87bb2f50142f3758945c5a2276a2535008aec4f32a5ec29f021faa95f3b3f`, with
its verified full 16 MiB backup. A separately authorized rollback restores
REMOTE first, then TUNER, application-only with independent readback; it never
touches NVS, pairing, bootloaders or partition tables.
