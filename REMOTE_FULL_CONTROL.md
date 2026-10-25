# REMOTE Full Control

## Compatibility and ownership

TUNER remains the sole application owner. RTCP/1 authentication, record
counters, replay protection, request correlation, and the existing D-009 and
BTX records are unchanged. A bounded `+fc1` suffix in the existing identity
text tells a new REMOTE that the separate Full-Control capability request is
safe. Older clients already accept and ignore identity text; BTX feature bits
remain unchanged. An older TUNER therefore never receives an unknown record.

## Milestone matrix

| Milestone | Status | Notes |
| --- | --- | --- |
| 1 Source switching | PASS (offline) | Capability/state/select records, generation fence, one-slot owner mailbox, idempotency, Radio/Bluetooth lifecycle. Hardware pending. |
| 2 Read-only menu projection | PENDING | Not implemented. |
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

Static extension storage is compile-time bounded to 48 bytes. Runtime DMA
headroom remains hardware-only evidence and is not inferred from static size.
