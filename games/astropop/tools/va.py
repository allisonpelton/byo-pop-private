#!/usr/bin/env python3
"""Prints the C string at each virtual address given (hex) from the carved game image re/astropop/bin/AstroPopGame.exe.

    python games/astropop/tools/va.py 562500 561f8c ...
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


for a in sys.argv[1:]:
    va = int(a, 16)
    o = off(va)
    if o is None:
        print('%x: not mapped' % va)
        continue
    e = data.index(b'\0', o)
    print('%x: %r' % (va, data[o:e].decode('latin-1')))
