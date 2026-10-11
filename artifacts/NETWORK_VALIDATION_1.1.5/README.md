# 1.1.5 allocation-requester attribution candidate

**Diagnostic candidate; hardware NOT RUN; incident OPEN.**

Built on the tested 1.1.4 executable and the new root-refresh failure report.
HTTP output behavior, JSON, audio/BT lifecycle, pins, partitions and all
dependencies remain unchanged. No reconnect/reboot/workaround was added.
See [analysis and proposed correction](../NETWORK_VALIDATION_1.1.4/ALLOCATION_ANALYSIS.md).

New `x` output retains allocator API, core, ISR/cache flags, tick, current task
name/handle and six bounded caller PCs for first four failed allocations plus
latest. It also reports free/largest for INTERNAL|DMA|8BIT and the same arena
at root start/built/end. Callback performs no allocation/logging/network work.
Stack capture is skipped in ISR/cache-off contexts. Omitted records are counted.

Keep the current failed tuner unchanged until its wait-only snapshot and ping
evidence are saved. This candidate is for a later, manually installed test;
no flash is performed by Codex. Verify version and build_git_id from manifest.

Next run: `x` at boot and BT before HTTP; retain continuous ping. Request root
once with the browser, capture `x` immediately afterward and after waiting.
Retain `[WEB_RESPONSE]` and every `[ALLOC_FAILURE]` line. Distinguish existing
HTTP response count from a true no-HTTP stage. Decode PCs with matching ELF:

```text
xtensa-esp32-elf-addr2line -aipfC -e firmware-1.1.5-<build_git_id>.elf 0xPC1 0xPC2 ...
```

`allocator=heap_caps_malloc` alone does not identify the consumer. Caller
frames can identify Wi-Fi's DMA wrapper/esf buffer path, TCP or another user.
The task name is execution context, not proof that the task owns all memory.
Capturing failures is not a claim of network recovery or root-cause proof.
