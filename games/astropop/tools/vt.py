#!/usr/bin/env python3
"""Prints the slots of a vtable of the carved game image, with the base-class function each slot shares:

    python games/astropop/tools/vt.py 5648f8 [count] [baseVtable]    # e.g. Widget's vtable 5648f8 vs the class's

With a third argument (the vtable of the base class, e.g. 0x56??), a slot equal to the base's is marked '='.
"""
import os, struct, sys
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
data = open(os.path.join(ROOT, 're', 'astropop', 'bin', 'AstroPopGame.exe'), 'rb').read()
pe = struct.unpack_from('<I', data, 0x3c)[0]
nsec = struct.unpack_from('<H', data, pe + 6)[0]
opt = struct.unpack_from('<H', data, pe + 20)[0]
base = struct.unpack_from('<I', data, pe + 52)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + opt + i * 40
    vs, va, rs, ro = struct.unpack_from('<IIII', data, o + 8)
    secs.append((va, vs, ro, rs))


def off(va):
    r = va - base
    for sva, vs, ro, rs in secs:
        if sva <= r < sva + max(vs, rs):
            return ro + r - sva
    return None


def slots(va, n):
    o = off(va)
    return [struct.unpack_from('<I', data, o + 4 * i)[0] for i in range(n)]


va = int(sys.argv[1], 16)
n = int(sys.argv[2]) if len(sys.argv) > 2 else 47
b = slots(int(sys.argv[3], 16), n) if len(sys.argv) > 3 else None
for i, s in enumerate(slots(va, n)):
    mark = ' =' if b and b[i] == s else ''
    print('%2d (+%02x) %08x%s' % (i, 4 * i, s, mark))
