# 1.1.5 allocation-requester attribution candidate

**Diagnostic candidate; hardware PARTIAL/network FAIL reproduced; incident OPEN.**
See [owner evidence and exact ELF decoding](OWNER_REPORT_2026-10-11.md): first
root after zero-HTTP baseline reproduces outage; all five 1622-byte failures
belong to wifi/core 0 through the ESF allocator. Persistence is unverified.

Built on the tested 1.1.4 executable and the new root-refresh failure report.
HTTP output behavior, JSON, audio/BT lifecycle, pins, partitions and all
dependencies remain unchanged. No reconnect/reboot/workaround was added.
See [analysis and proposed correction](../NETWORK_VALIDATION_1.1.4/ALLOCATION_ANALYSIS.md).

New `x` output retains allocator API, core, ISR/cache flags, tick, current task
name/handle and six bounded caller PCs for first four failed allocations plus
latest. It also reports free/largest for INTERNAL|DMA|8BIT and the same arena
at root start/built/end. Callback performs no allocation/logging/network work.
Stack capture is skipped in ISR/cache-off contexts. Omitted records are counted.

The 1.1.4 observation is concluded and its wait-only evidence is saved.
For the failed 1.1.5 boot, retain wait-only snapshot/ping evidence before
perturbing it. No flash is performed by Codex. Verify build identity in manifest.

Next run: `x` at boot and BT before HTTP; retain continuous ping. Request root
once with the browser, capture `x` immediately afterward and after waiting.
Retain `[WEB_RESPONSE]` and every `[ALLOC_FAILURE]` line. Distinguish existing
HTTP response count from a true no-HTTP stage. Decode PCs with matching ELF:

```text
xtensa-esp32-elf-addr2line -aipfC -e firmware-1.1.5-f33ef76cfc29.elf 0xPC1 0xPC2 ...
```

`allocator=heap_caps_malloc` alone does not identify the consumer. Caller
frames can identify Wi-Fi's DMA wrapper/esf buffer path, TCP or another user.
The task name is execution context, not proof that the task owns all memory.
Capturing failures is not a claim of network recovery or root-cause proof.

Extract the published `firmware-1.1.5-f33ef76cfc29.elf.zip` first. Do not
decode these PCs with the older 1.1.4 ELF or an unidentified rebuild.
