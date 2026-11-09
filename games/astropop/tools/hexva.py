#!/usr/bin/env python3
"""python hexva.py <va hex> [length]: hex bytes of the carved game image at a virtual address."""
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
va = int(sys.argv[1], 16)
n = int(sys.argv[2]) if len(sys.argv) > 2 else 64
r = va - base
for sva, vs, ro, rs in secs:
    if sva <= r < sva + max(vs, rs):
        o = ro + r - sva
        for i in range(0, n, 16):
            print('%08x  %s' % (va + i, data[o + i:o + i + 16].hex(' ')))
        break
