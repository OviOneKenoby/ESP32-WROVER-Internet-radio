# BTX/1.0 TUNER implementation report

Status: **SOFTWARE VERIFIED / HARDWARE NOT RUN**  
Date: 2026-10-22

## Scope and baseline

- Contract: owner-approved RTCP/1 Bluetooth Extension v1, BTX/1.0 (D-011).
- TUNER source baseline: `216f5a14d375652543d3f722141acbf526bee8c8`
  (`tuner/a-remote-014-mdns-discovery`, stacked on accepted D-009).
- Implementation source/build commit:
  `bc9c148a942732c890fcba6e6b15769bc4cd9067`.
- Production environment: `esp32-lwip-psram`; board `esp32dev`; 3 MiB
  application slot.
- The accepted D-009 and mDNS worktrees were not reset or modified.

## Implementation

- `src/rtcp_btx.*`: fixed-size BTX codec, negotiation state, validation,
  generation fencing and one-command mailbox.
- `src/rtcp_server.*`: authenticated second-ID-3 opt-in, IDs 12-15, and the
  existing main-loop owner boundary. ID 8 remains the correlated result.
- `src/audio.*`: pinned A2DP connection/audio callbacks, bounded metadata,
  authoritative state, and explicit Play/Pause/Previous/Next submission.
- `src/main.cpp`: registers the Bluetooth main-loop owner.

The first ID 3 response remains the exact frozen revision-2 ID 4. An immediate
second ID 3 returns ID 12. Any intervening application request locks the
session to legacy operation. Disconnect/new session clears negotiation and
the mailbox. IDs 1-11, RTCP crypto/framing/counters, volume behavior, D-009
radio semantics, mDNS, pairing and NVS are unchanged.

Metadata is strict UTF-8 and truncates only at code-point boundaries. Artist
and title maxima are 96 and 127 bytes. Stale fields clear on Bluetooth entry,
source exit, phone connection and disconnection epochs. New diagnostics never
log metadata payloads.

## Focused verification

Eleven directly relevant test programs/policies passed, totaling 183
assertions/checks:

- BTX codec/state/negotiation/command model: 46/46.
- BTX integration and approved vectors: 21/21.
- CCM wire compatibility: 24/24.
- D-009 model and integration policy: 23/23 and 12/12.
- mDNS model and policy: 9/9 and 6/6.
- Bluetooth Classic memory policy: 10/10.
- RTCP latency, display-service and diagnostics policies: 9/9, 9/9 and 14/14.

The first production compile exposed a missing ESP-IDF A2DP type include in
`audio.h`. Adding the defining `esp_a2dp_api.h` include was the only correction;
the directly affected 46, 21 and 10 checks passed again before the successful
production build.

## Build identity and budgets

| Item | Result |
| --- | ---: |
| Embedded build ID | `bc9c148a9427` |
| Application size | 2,072,960 bytes |
| Application SHA-256 | `67fbeb56082fffddb6104f5ed948395bc862262cf5bcb10602292b08cb3ad590` |
| ELF SHA-256 | `e4807c7929941cf0a0255a24b98633342ed9a435d6da31a4973ca31e2eaace99` |
| Static RAM | 89,904 / 327,680 bytes (27.4%) |
| Application-slot headroom | 1,072,768 bytes |
| Static RAM change from accepted baseline | +72 bytes |
| Application size change from accepted baseline | +3,808 bytes |
| New persistent BTX scalar/session state | 60 bytes, guarded at <=64 |
| Maximum BTX plaintext / authenticated record | 255 / 299 bytes |

The existing 512-byte plaintext and 556-byte authenticated-record storage is
reused. No new heap allocation, network task, RTCP transport buffer, unbounded
queue or metadata history was added.

## Rollback and remaining risk

Rollback remains the accepted installed D-009 plus mDNS application with
SHA-256 `d2c89edd1e5aa25a01c7f88ce787ccdcfa2e708a89d6fb402d0326a519ec896e`,
subject to the existing private backup verification procedure. No rollback
artifact is stored in this repository.

Static analysis and host tests cannot prove A2DP runtime heap margin, phone
AVRCP behavior, callback timing, audio coexistence or old-REMOTE behavior on
physical hardware. The candidate must be installed TUNER-first and complete
the separate hardware checklist before acceptance.
