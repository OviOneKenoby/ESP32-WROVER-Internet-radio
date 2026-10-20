# A-REMOTE-013 / D-009 TUNER hardware-validation checkpoint

Date: 2026-10-20
Status: PARTIAL PASS / RTCP VALIDATION BLOCKED

## Preflight and installation

- Intended TUNER identified on COM4 as a classic dual-core ESP32 with 4 MiB
  flash.
- Verified application layout: offset `0x10000`, capacity 3 MiB.
- Candidate: 2,040,704 bytes, SHA-256
  `1e6c5968f539170656e833d023086c57986b1aa73a6c2fe982dee2b31b1ff1f6a`.
- Known-good rollback v1.1.12: 2,037,632 bytes, SHA-256
  `6d7f2ebfa833bd09f287b8a7024071d736dfb2bf02356f889de94d7b980fdf6a`.
- Four rollback-critical files were verified in both the private local set and
  the separate USB set (8/8 file/hash comparisons passed).
- Only the application image was written at `0x10000`. The independent exact
  2,040,704-byte readback matched the candidate SHA-256.
- Boot reported firmware `1.1.12`, build `6a58c036e848`; retained Wi-Fi joined
  normally and setup completed without observed panic or watchdog output.

## Observations

| Check | Result | Evidence |
| --- | --- | --- |
| Application flash/readback | PASS | Exact size and SHA-256 match |
| Boot/build identity | PASS | Embedded build `6a58c036e848` |
| Retained Wi-Fi | PASS | Connected after boot; measured RSSI -46 to -48 dBm |
| MP3 playback | PASS | Owner heard smooth playback; diagnostics reported MP3/playing |
| HTTP manager/diagnostics | PASS | 10/10 bounded requests; 25-192 ms |
| Internal heap baseline | PASS | 89,424-106,328 bytes free; largest block 86,004-102,388 bytes |
| RTCP IDs 1-8 / volume | BLOCKED | REMOTE attempted the former stored IPv4 endpoint |
| Station switching | NOT TESTED | Awaiting RTCP endpoint resolution and bounded physical check |
| Bluetooth A2DP/reconnect | NOT TESTED | Awaiting same checkpoint |
| D-009 IDs 9-11 end to end | NOT TESTED | Existing REMOTE has no D-009 UI/adapter |

## Blocking condition

The preserved association contains a literal IPv4 address. After DHCP assigned
new addresses, REMOTE remained Wi-Fi-connected but repeatedly timed out in TCP
connect before authentication, while the TUNER RTCP listener was reachable at
its new address. Source inspection confirms that REMOTE connects directly to
the persisted four-byte IPv4 value and TUNER advertises no discovery service.
Pairing material remains present and no authentication failure was observed.

Automatic endpoint discovery requires a separately reviewed source change.
It must retain the existing PSK/mutual-authentication proof, use a bounded
runtime endpoint, avoid storing or publishing secrets, and leave CCM/D-009
message semantics unchanged. No pairing, NVS, or endpoint rewrite was made in
this checkpoint.

## Recommendation

**INVESTIGATE / EXTEND ENDPOINT DISCOVERY.** Preserve the verified installation
and rollback artifacts. Do not classify the blocked RTCP checks as D-009
failures, and do not merge PR #7 until dynamic-address operation is resolved
and the remaining hardware checks are run.
