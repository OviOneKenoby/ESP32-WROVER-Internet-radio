# BTX/1.0 TUNER-first hardware validation

Status: **NOT AUTHORIZED / NOT RUN**

1. Verify the TUNER identity, partition layout, exact private rollback image
   and independent backup hashes.
2. Verify the candidate is exactly 2,072,960 bytes with SHA-256
   `67fbeb56082fffddb6104f5ed948395bc862262cf5bcb10602292b08cb3ad590`.
3. Write only the application at the verified application offset, then read
   back exactly the candidate span and require the same SHA-256.
4. Verify normal Wi-Fi, radio playback, web manager, mDNS and retained pairing.
5. With the existing Round 5 REMOTE, verify legacy identity/capability,
   volume, D-009 Play/Pause/Previous/Next and HOME metadata remain unchanged.
6. Verify Bluetooth A2DP audio and the TUNER physical AVRCP controls before any
   BTX REMOTE is introduced.
7. During A2DP plus RTCP plus web-manager operation, record internal free heap,
   minimum heap, largest allocatable internal block and allocation-failure
   counters; stop on reset, watchdog, audio regression or unsafe margin.
8. After a separately verified BTX REMOTE exists, verify second-ID-3
   negotiation, IDs 13/14, Play/Pause/Previous/Next results, callback-confirmed
   state, metadata bounds, reconnect clearing and source/phone generation
   fencing.

On failure, stop without automatic retry. Restore only through the separately
authorized application rollback procedure. Preserve NVS, pairing, bootloader,
partition table and unrelated flash regions.
