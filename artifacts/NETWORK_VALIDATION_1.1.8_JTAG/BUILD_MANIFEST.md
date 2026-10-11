# Repository 1.1.8 JTAG candidate, not a firmware release

- Firmware1.1.8-jtag-validation; embedded build ID e2c5667dd9a1 verified in bin.
- Executable source commit e2c5667dd9a16774f7057dbffbb117c6db16ea3b.
- Explicit env esp32-jtag, initially empty build directory; build PASS351.31s.
- RAM83952/327680, flash2013809/3145728. RAM is488 bytes LESS than1.1.7:
  encoder paths/IRQ removed by this profile. Layout/timing change is a confound,
  not a TX memory fix. Use a fresh baseline; non-reproduction is inconclusive.
- Bin2020384bytes, exactELF29477492bytes, ELFzip11020473bytes.
  SHA256SUMS.txt records bin/ELF/zip identities.
- 2367 host checks recompiled and PASS; exact pinned ArduinoJson used.
- Final ELF five-wrapper routing/IRAM/exactly-one-original checks PASS.
  InputControl::init disassembly only configures34/35/39 (also Bounce attach),
  not13/14; encoderISR absent from final symbols. Encoder polling/click and
  destructor detach are guarded out. Other physical buttons remain available.
- Source changes: conditional version + separate explicit diagnostic env +
  encoder disable guard. No TX/refusal instrumentation, SDK, buffer count,
  retry/recycling, web payload, GPIO assignments for audio/display or automatic
  recovery changes. After this build, default_envs=esp32-dev was added to keep
  unqualified CLI builds/uploads from selecting the JTAG profile. It does not
  alter the explicitly selected esp32-jtag options used for this exact ELF.
- Same espressif32 7.0.1, framework3.20017.241212+sha.dcc1105b,
  Xtensa8.4.0+2021r2-patch5, pinned libraries/AAC patch/partitions.
- Optional tools installed: OpenOCD0.11.0-esp32-20220706
  (tool-openocd-esp32 2.1100.20220706), esp-GDB12.1_20221002/Python3.11
  (tool-xtensa-esp-elf-gdb12.1.0+20221002). No toolchain/framework upgrade.
- OpenOCD configuration parse ONLY, no init/USB/target connection: actual
  scripts confirm ESP_FLASH_SIZE0 -> both flash banks disabled and
  gdb_breakpoint_override hard. FTDI/ESP32 config parse PASS.
- Exact ELF loaded in esp-GDB; both scripts with ready0 print STOP and
  create no breakpoint/watchpoint. Hardware ready1/SMP/breakpoints/backtraces
  NOT RUN, not simulated or claimed validated on hardware.
- Exact-ELF probe locations generated and statically reviewed:
  type9 free-head NULL branch40240abd (head load40240ab1/nonzero branch40240aba),
  output-do NO_MEM return401ff36f;
  conversion result4023cbca (a10 result,a2 cache entry after saved-result reload),
  cache-recycle entry after window entry401fdc33 (a2 cache entry).
  Type9 jump-table target40240a8d checked from literal/table words.
  No private list walk, ABI guess, opcode patch, target calls/data writes.
- Hardware NOT RUN. A CH340 UART is present on COM5; JTAG adapter model and
  actual wiring/SMP/hardware insertion are unverified. Full conditional protocol
  and stop rules in PROTOCOL_RO.md; do not run if prerequisites are missing.
- Incident OPEN; PR unmerged; no flash/reconnect/reboot/merge performed.

This candidate adds private-PC observation feasibility; it is not a build that
just adds/repeats counters, and is not a demonstrated fix. Bin, raw exact ELF,
ELF zip, hashes, probes and protocol are tracked in the investigation branch
for manual review/test. No new firmware release was created.
