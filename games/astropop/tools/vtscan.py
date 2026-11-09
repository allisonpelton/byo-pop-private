#!/usr/bin/env python3
"""List the vtables of AstroPopGame.exe (no game RTTI): runs of pointers into .text in .rdata, with the functions that store
their address (constructors) and the slots' names from Ghidra's export.
    python games/astropop/tools/vtscan.py [min_slots] > re/astropop/vtables.txt"""
import bisect, json, struct, sys
GH = 're/astropop/gh/functions.jsonl'
EXE = 're/astropop/bin/AstroPopGame.exe'
minslots = int(sys.argv[1]) if len(sys.argv) > 1 else 4
d = open(EXE, 'rb').read()
fs = [json.loads(l) for l in open(GH)]
fs = [f for f in fs if not f['external']]
fs.sort(key=lambda f: int(f['addr'], 16))
starts = [int(f['addr'], 16) for f in fs]
byaddr = {int(f['addr'], 16): f for f in fs}
pe = struct.unpack_from('<I', d, 0x3c)[0]
n = struct.unpack_from('<H', d, pe + 6)[0]; o = struct.unpack_from('<H', d, pe + 20)[0]
off = pe + 24 + o; secs = {}
for i in range(n):
    nm = d[off:off + 8].rstrip(b'\0').decode(); vs, va, rs, rp = struct.unpack_from('<IIII', d, off + 8)
    secs[nm] = (0x400000 + va, rs, rp); off += 40
TEXT = (0x401000, 0x401000 + secs['.text'][1])
def enclosing(a):
    i = bisect.bisect_right(starts, a) - 1
    return fs[i] if i >= 0 else None
def u32(va):
    for nm, (v, rs, rp) in secs.items():
        if v <= va < v + rs:
            return struct.unpack_from('<I', d, rp + va - v)[0]
text = d[secs['.text'][2]:secs['.text'][2] + secs['.text'][1]]
vts = []
for nm in ('.rdata', '.data'):
    v, rs, rp = secs[nm]
    a = v
    while a < v + rs - 3:
        w = u32(a)
        if w and TEXT[0] <= w < TEXT[1]:
            b = a; sl = []
            while True:
                w = u32(b)
                if w and TEXT[0] <= w < TEXT[1]: sl.append(w); b += 4
                else: break
            if len(sl) >= minslots: vts.append((a, sl))
            a = b if b > a else a + 4
        else: a += 4
# who stores the vtable address
import re
refs = {}
for a, sl in vts:
    pat = struct.pack('<I', a)
    pos = 0; users = set()
    while True:
        i = text.find(pat, pos)
        if i < 0: break
        f = enclosing(0x401000 + i)
        if f: users.add(f['addr'])
        pos = i + 1
    refs[a] = sorted(users)
for a, sl in vts:
    names = [byaddr[s]['name'] if s in byaddr else 'MISSING' for s in sl[:3]]
    print('%08x slots=%d first=%08x ctors=%s %s' % (a, len(sl), sl[0], ','.join(refs[a]), names[0] if not names[0].startswith('FUN_') else ''))
