"""Fail-closed evaluation of final Xtensa classifier operands (no target access).

Usage: python verify_tx_classification_elf.py ELF BINUTILS_DIRECTORY
Only the deliberately small classifier instruction subset is supported.
Unknown instructions, control-flow gaps, non-file-backed reads and loops fail.
"""
import hashlib
import re
import struct
import subprocess
import sys
from pathlib import Path

elf, tool = map(Path, sys.argv[1:3])
data = elf.read_bytes()
assert data[:6] == b"\x7fELF\x01\x01"
sections = [struct.unpack_from("<10I", data, struct.unpack_from("<I", data, 32)[0] +
            i * struct.unpack_from("<H", data, 46)[0])
            for i in range(struct.unpack_from("<H", data, 48)[0])]

def word(address):
    for s in sections:
        if s[1] != 8 and s[3] <= address <= s[3] + s[5] - 4:
            return struct.unpack_from("<I", data, s[4] + address - s[3])[0]
    raise AssertionError(f"non-file-backed operand {address:#x}")

nm = subprocess.check_output([str(tool / "xtensa-esp32-elf-nm.exe"), "-n", str(elf)], text=True)
symbols = {m[2]: int(m[1], 16) for line in nm.splitlines()
           if (m := re.match(r"^([0-9a-f]+)\s+\S\s+(.+)$", line))}
dump = subprocess.check_output([str(tool / "xtensa-esp32-elf-objdump.exe"), "-d", str(elf)], text=True)
instructions = {}
for line in dump.splitlines():
    m = re.match(r"\s*([0-9a-f]+):\s+([0-9a-f]+)\s+([a-z][a-z0-9_.]*)\s*(.*)", line)
    if m:
        instructions[int(m[1], 16)] = (len(m[2]) // 2, m[3], m[4].split(" <")[0])

def symbol(fragment):
    hits = [n for n in symbols if fragment in n and "GLOBAL" in n]
    assert len(hits) == 1, (fragment, hits)
    return symbols[hits[0]]

# Historical negative fixture is immutable: explicitly reject the actual bad
# resolved operand, rather than trusting the C++ spelling of the expression.
if hashlib.sha256(data).hexdigest() == "2495fc047b66de639b25440744a0920f5b7fb91984441d1dddde5e1eb4a51406":
    actual = word(0x400804d0)
    expected = symbols["ieee80211_output_process"] + 0xb7
    assert actual == expected, (f"REJECT fe228ca737b9 unlock normal comparison: "
                                f"actual={actual:#x}, expected={expected:#x}")

sites = [("esf_buf_alloc", 0x7a), ("ieee80211_output_process", 0x4f),
         ("ieee80211_output_process", 0x108), ("ieee80211_output_process", 0x226),
         ("esf_buf_alloc_dynamic", 0x78), ("esf_buf_alloc_dynamic", 0x1a8),
         ("ieee80211_output_process", 0xb7), ("ieee80211_output_process", 0xa9),
         ("esf_buf_alloc_dynamic", 0xa8)]
table = symbol("classificationSites")
assert 0x3ffb0000 <= table < 0x40000000, "classification operands must be in DRAM"
expected_sites = []
for index, (name, offset) in enumerate(sites):
    assert word(table + index * 8) == symbols[name], f"wrong final base operand for site {index}"
    assert word(table + index * 8 + 4) == offset, f"wrong final offset operand for site {index}"
    expected_sites.append(symbols[name] + offset)

for index, target in ((6, "ic_ebuf_alloc"), (7, "ieee80211_wapi_alloc_tx_buf")):
    _, op, args = instructions[expected_sites[index]]
    assert op == "call8" and int(args.split()[0], 16) == symbols[target], "wrong context call PC"

address_fn = symbol("classificationAddress")
kind_fn = symbol("kindFor")
conversion_fn = symbol("conversionPc")
malloc_fn = symbol("mallocPc")
allowed = {address_fn, kind_fn, conversion_fn, malloc_fn}
assert all(0x40080000 <= a < 0x400c0000 for a in allowed), "classifier must be IRAM"

def evaluate(start, argument, depth=0):
    assert depth < 4
    regs = {f"a{i}": None for i in range(16)}
    regs["a2"] = argument & 0xffffffff
    pc = start
    reads, comparisons = [], []
    def val(x):
        if x in regs:
            assert regs[x] is not None, f"unknown register {x} at {pc:#x}"
            return regs[x]
        return int(x, 0)
    def put(r, value):
        regs[r] = value & 0xffffffff
    for _ in range(400):
        assert pc in instructions, f"control-flow gap at {pc:#x}"
        length, op, args = instructions[pc]
        p = [x.strip() for x in args.split(",") if x.strip()]
        following = pc + length
        if op in ("entry", "nop", "nop.n", "memw"):
            pass
        elif op in ("retw", "retw.n"):
            return val("a2"), reads, comparisons
        elif op in ("mov.n", "mov"):
            put(p[0], val(p[1]))
        elif op in ("movi", "movi.n"):
            put(p[0], val(p[1]))
        elif op == "l32r":
            location = int(p[1], 16)
            value = word(location)
            reads.append((location, value))
            put(p[0], value)
        elif op in ("l32i", "l32i.n"):
            location = val(p[1]) + val(p[2])
            value = word(location)
            reads.append((location, value))
            put(p[0], value)
        elif op in ("add", "add.n", "addi", "addi.n"):
            put(p[0], val(p[1]) + val(p[2]))
        elif op == "sub":
            put(p[0], val(p[1]) - val(p[2]))
        elif op in ("addx2", "addx4", "addx8"):
            put(p[0], val(p[1]) * int(op[-1]) + val(p[2]))
        elif op == "slli":
            put(p[0], val(p[1]) << val(p[2]))
        elif op == "extui":
            put(p[0], (val(p[1]) >> val(p[2])) & ((1 << val(p[3])) - 1))
        elif op in ("or", "and", "xor"):
            x, y = val(p[1]), val(p[2])
            put(p[0], x | y if op == "or" else x & y if op == "and" else x ^ y)
        elif op in ("moveqz", "movnez"):
            if (val(p[2]) == 0) == (op == "moveqz"):
                put(p[0], val(p[1]))
        elif op == "call8":
            target = int(p[0], 16)
            assert target in allowed, f"unexpected classifier call {target:#x}"
            result, subreads, subcomparisons = evaluate(target, val("a10"), depth + 1)
            # Windowed ABI: callee a2 aliases caller a10; local a2..a7 survive.
            for r in range(8, 16):
                regs[f"a{r}"] = None
            put("a10", result)
            reads.extend(subreads)
            comparisons.extend(subcomparisons)
        elif op in ("beq", "bne", "bltu", "bgeu", "bltui", "bgeui", "beqi", "bnei"):
            x, y = val(p[0]), val(p[1])
            comparisons.append((pc, op, x, y))
            condition = x == y if op in ("beq", "beqi") else x != y if op in ("bne", "bnei") else x < y if op in ("bltu", "bltui") else x >= y
            if condition:
                following = int(p[2], 16)
        elif op in ("beqz", "beqz.n", "bnez", "bnez.n"):
            comparisons.append((pc, op, val(p[0]), 0))
            if (val(p[0]) == 0) == op.startswith("beq"):
                following = int(p[1], 16)
        elif op == "j":
            following = int(p[0], 16)
        else:
            raise AssertionError(f"unsupported instruction {pc:#x}: {op} {args}")
        pc = following
    raise AssertionError("classifier instruction limit")

for index, expected in enumerate(expected_sites):
    result, reads, _ = evaluate(address_fn, index)
    assert result == expected, f"wrong effective address for site {index}"
    assert (table + index * 8, symbols[sites[index][0]]) in reads
    assert (table + index * 8 + 4, sites[index][1]) in reads
    print(f"PASS final load/add operands site={index} effective={result:#x}")

for index, pc in enumerate(expected_sites[:6]):
    result, _, comparisons = evaluate(kind_fn, pc)
    assert result == index
    assert any(x == pc and y == pc for _, _, x, y in comparisons), "no effective equality comparison"
    for delta in (-1, 1):
        assert evaluate(kind_fn, pc + delta)[0] == 0xffffffff
for pc in expected_sites[6:8]:
    assert evaluate(conversion_fn, pc)[0] == 1
    assert evaluate(conversion_fn, pc - 1)[0] == 0
    assert evaluate(conversion_fn, pc + 1)[0] == 0
assert evaluate(malloc_fn, expected_sites[8])[0] == 1
assert evaluate(malloc_fn, expected_sites[8] - 1)[0] == 0
assert evaluate(malloc_fn, expected_sites[8] + 1)[0] == 0
bad_pc = symbols["ieee80211_output_process"] + 0xa2
assert evaluate(conversion_fn, bad_pc)[0] == 0, "accepts fe228 wrong classification offset"
for index, pc in enumerate(expected_sites):
    assert evaluate(kind_fn, pc)[0] == (index if index < 6 else 0xffffffff)
    assert evaluate(conversion_fn, pc)[0] == int(index in (6, 7))
    assert evaluate(malloc_fn, pc)[0] == int(index == 8)

def function_body(fragment):
    address = symbol(fragment)
    match = re.search(rf"^{address:08x} <[^>]+>:\n(.*?)(?=^[0-9a-f]+ <|\Z)", dump, re.M | re.S)
    assert match
    return match[1]

for caller in ("unlockEPv", "observeMalloc"):
    code = function_body(caller)
    assert re.search(rf"\bcall8\s+{conversion_fn:08x}\b", code), "caller bypasses shared conversion predicate"
    assert not re.search(r"\b(?:l32r|addi|movi).*ieee80211_output_process", code), "inline context predicate remains"
assert re.search(rf"\bcall8\s+{kind_fn:08x}\b", function_body("unlockEPv"))
assert re.search(rf"\bcall8\s+{malloc_fn:08x}\b", function_body("observeMalloc"))
print("PASS six site predicates, normal/WAPI agreement between unlock and malloc, malloc-site predicate")
print("PASS explicitly rejects historical normal offset +0xa2; static evaluation only, not hardware unwind coverage")
