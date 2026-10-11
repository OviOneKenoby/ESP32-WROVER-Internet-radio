# 1.1.9 UART diagnostic candidate — incident OPEN

- Firmware: **1.1.9-uart-validation**, embedded build ID **fe228ca737b9**.
- Executable source commit: fe228ca737b9100cd220da3facb314de192eb060.
  Later artifact/document/verifier-only commits do not change executable sources.
- Environment: explicit `esp32-uart-probe`; default remains `esp32-dev`.
- Fixed espressif32@7.0.1, Arduino2.0.17/IDF4.4.7 framework package
  3.20017.241212+sha.dcc1105b, Xtensa8.4.0+2021r2-patch5, existing pinned
  libraries and AAC patch. No dependency/SDK/toolchain upgrade.
- Build SUCCESS, 187.647s. PIO data RAM85840B, flash2021617B.
  Compared with1.1.7: **+1400B DRAM data, +1648B IRAM text**; BSS unchanged.
  This overhead and callback/unwind/UART timing can alter reproduction/heap.
- Binary: firmware-1.1.9-uart-fe228ca737b9.bin, 2028192 bytes.
  SHA256 E1B6C879B28A57C8983125AB03F1FB148418FED795738A0BC8B1E49F86B28520.
- Exact ELF: firmware-1.1.9-uart-fe228ca737b9.elf, 29556532 bytes.
  SHA256 2495FC047B66DE639B25440744A0920F5B7FB91984441D1DDDDE5E1EB4A51406.
- esptool4.11.0 image_info: five segments, checksum valid, validation hash valid;
  exact version/build ID checked in binary bytes. Flash partition app0 remains
  0x10000/0x300000 from huge_app.csv. No flash/readback/target command executed.

## Verification on host

- 2367 existing regression checks recompiled with the candidate's pinned
  ArduinoJson: correctness266, bounded writer2024, retention10, refusal67 PASS.
  These are regressions, not runtime tests of the new OSI callback coverage.
- Final artifact ELF five public wrapper routes, IRAM placement and
  exactly-one-original-call checks PASS.
- New final-ELF verifier compares six complete normalized private function
  instruction bodies against the hash-verified exact1.1.7 ELF, checks six
  OSI mutex-unlock sites and the dynamic malloc site, and confirms the private
  allocator uses the OSI pointer stored by registration. PASS.
- First call in each new forwarding callback is the original via its DRAM
  pointer: PC normalization forced always_inline, no pre-cache flash helper.
  Callback IRAM and no added logger/heap queries; bounded serial fragments
  under64B even at UINT32_MAX. PASS. Details in ELF_VERIFICATION.txt.
- Build guard: libpp.a SHA256
  A4356534D066DA8C1CE08B52E3E614EE4DA1A3E9B3B4BFC4285D083F46B75937,
  libnet80211.a SHA256
  6DB8C03E58385CAE462871647C680489BA84B5C29E86ABCA9C89DCA9D8BB7BA1.
  SDK ABI assertions: OSI version8, size468, mutex_unlock88,
  malloc_internal348, wifi_malloc364. No binary patching/private ABI calls.
- Source src/main.cpp unchanged from06011be; branch main remains
  112d68a49a053d302a87fc5658d497617eedda88. Encoder remains enabled.

New final instruction return addresses (use this ELF for decoding):

| Observation | Return address |
|---|---|
|Static-list empty|4024160e|
|Pending dequeue|4023d68f|
|NULL conversion requeue|4023d748|
|Pending empty|4023d866|
|Dynamic quota passed|40240e80|
|Dynamic NULL exit|40240fb0|
|Dynamic malloc returned|40240eb0|

Hardware **NOT RUN**. No fix validated, no release designation, no merge or
automatic flash/reconnect/reboot. Runtime positive control and complete
[UART protocol](PROTOCOL_RO.md) are required. Direct pool inventory remains
unobserved; full causality/persistence mechanism remains under investigation.
