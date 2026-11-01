#!/usr/bin/env python3
"""List Bonnie's Bookstore's vtables: immediates in .rdata that the game code stores through a register (the
constructors' `mov dword ptr [reg], vtable`), each with its run of code pointers. Reads the carved exe and an
llvm-objdump disassembly of the game's code (games/bonnies/docs/README.md).
    python games/bonnies/tools/vtables.py re/bonnies/bin/BonniesGame.exe re/bonnies/game_disasm.txt > re/bonnies/vtables.txt
"""
import re
import struct
import sys

exe = open(sys.argv[1], "rb").read()
pe = struct.unpack_from("<I", exe, 0x3C)[0]
nsec = struct.unpack_from("<H", exe, pe + 6)[0]
opt = struct.unpack_from("<H", exe, pe + 20)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + opt + 40 * i
    name = exe[o:o + 8].rstrip(b"\0").decode()
    vs, va, rs, rp = struct.unpack_from("<IIII", exe, o + 8)
    secs.append((name, 0x400000 + va, vs, rp))
text = next(s for s in secs if s[0] == ".text")
rdata = next(s for s in secs if s[0] == ".rdata")


def dword(a):
    for _, va, vs, rp in secs:
        if va <= a < va + vs:
            return struct.unpack_from("<I", exe, rp + a - va)[0]
    return None


def is_code(p):
    return p is not None and text[1] <= p < text[1] + text[2]


users = {}
store = re.compile(r"^\s*([0-9a-f]+):\s+movl\s+\$0x([0-9a-f]+), (?:-?0x[0-9a-f]+)?\(%e[a-z]{2}\)")
func = None
for line in open(sys.argv[2]):
    m = store.match(line)
    if m:
        v = int(m.group(2), 16)
        if rdata[1] <= v < rdata[1] + rdata[2] and is_code(dword(v)):
            users.setdefault(v, set()).add(int(m.group(1), 16))
starts = sorted(users)
for i, v in enumerate(starts):
    slots = []
    a = v
    while is_code(dword(a)) and (a == v or a not in users):
        slots.append(dword(a))
        a += 4
    print("%x %d stored_at=%s slots=%s" % (v, len(slots), ",".join("%x" % u for u in sorted(users[v])[:6]),
                                          " ".join("%x" % s for s in slots)))
