"""Produce optional hardware-only GDB probes from the exact candidate ELF."""
import re
import struct
import subprocess
import sys
from pathlib import Path
elf, objdump, output = map(Path, sys.argv[1:4])
text = subprocess.check_output([str(objdump), "-d", str(elf)], text=True)
data = elf.read_bytes()
assert data[:6] == b"\x7fELF\x01\x01"
section_offset = struct.unpack_from("<I", data, 32)[0]
section_size, section_count = struct.unpack_from("<HH", data, 46)
sections = [struct.unpack_from("<10I", data, section_offset + i * section_size)
            for i in range(section_count)]
def word(address):
    for section in sections:
        if section[1] != 8 and section[3] <= address < section[3] + section[5] - 3:
            return struct.unpack_from("<I", data, section[4] + address - section[3])[0]
    raise AssertionError(f"unmapped literal {address:x}")
def body(name):
    match = re.search(r"^[0-9a-f]+ <" + name + r">:\n(.*?)(?=^[0-9a-f]+ <|\Z)", text, re.M | re.S)
    assert match, name
    return match[1].splitlines()
empty = []
esf_lines = body("esf_buf_alloc")
for index, line in enumerate(esf_lines):
    match = re.search(r"^\s*([0-9a-f]+):.*\bl8ui\s+a4, a5, 8\s*$", line)
    if match:
        # The breakpoint is before the first type-register overwrite, after
        # a synchronized free-head load and the nonempty branch.
        assert re.search(r"\bl32i.n\s+a2, a5, 0", esf_lines[index - 5])
        assert re.search(r"\bbnez\s+a2,", esf_lines[index - 1])
        empty.append(int(match[1], 16))
assert len(empty) == 1, "SDK empty-list instruction changed; manual review required"
table_literal = re.search(r"\bl32r\s+a5,\s*([0-9a-f]+)", "\n".join(esf_lines[:12]))
assert table_literal
type9_entry = word(word(int(table_literal[1], 16)) + 8 * 4)
assert type9_entry < empty[0] and empty[0] - type9_entry < 64
type9_line = next(i for i, line in enumerate(esf_lines) if re.match(rf"\s*{type9_entry:x}:", line))
assert re.search(r"\bl32r\s+a11,", esf_lines[type9_line]), "type9 no longer goes straight to static-list path"
assert not any(re.search(r"\b(?:mov|movi|addi|l32i|l8ui|add).*\s+a3,", line)
               for line in esf_lines[type9_line:next(i for i, line in enumerate(esf_lines) if re.match(rf"\s*{empty[0]:x}:", line))]), "type register changed before probe"
lines = body("ieee80211_output_do")
returns = []
for index, line in enumerate(lines):
    if re.search(r"\bmovi\s+a2, 0x101\s*$", line):
        match = re.search(r"^\s*([0-9a-f]+):.*\bretw", lines[index + 1])
        assert match, "NO_MEM return pattern changed"
        returns.append(int(match[1], 16))
assert len(returns) == 1
output.write_text(f"""# OPTIONAL: manually attach a compatible ESP32 SMP OpenOCD/JTAG setup first.
# Use hardware breakpoints only; no software breakpoint/instruction patch.
# Never call target functions. Halts perturb timing; label a separate experiment.
# Verify this ELF is loaded and both cores are halted/resumed appropriately.
set $cache9_empty_hits = 0
set $tx_nomem_returns = 0
hbreak *0x{empty[0]:08x}
commands
silent
if $a3 == 9
set $cache9_empty_hits = $cache9_empty_hits + 1
printf "[JTAG_CACHE9_EMPTY] hits=%u pc=0x%08x type=%u head=0x%08x\\n", $cache9_empty_hits, $pc, $a3, $a2
end
continue
end
hbreak *0x{returns[0]:08x}
commands
silent
set $tx_nomem_returns = $tx_nomem_returns + 1
printf "[JTAG_TX_NOMEM_RETURN] returns=%u cache9_empty_hits=%u pc=0x%08x result=%u\\n", $tx_nomem_returns, $cache9_empty_hits, $pc, $a2
continue
end
""", encoding="utf-8")
print(f"Verified instruction patterns: ESF empty branch 0x{empty[0]:08x}, TX NO_MEM return 0x{returns[0]:08x}")
print("Manual full branch/ABI review still required; no target attached by this tool.")
