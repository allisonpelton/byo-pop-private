#!/usr/bin/env python3
"""Print the double (and float) stored at virtual addresses of the original exe: python games/typershark/tools/readdbl.py 529aa0 529ab8 ...
The x87 code loads its constants from the data section; Ghidra shows them only as addresses."""
import struct, sys
d = open('re/typershark/bin/TyperSharkGame.exe', 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]
opt = struct.unpack_from('<H', d, pe + 20)[0]
base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
secs = []
o = pe + 24 + opt
for i in range(nsec):
    name = d[o:o+8].rstrip(b'\0'); vs, va, rs, rp = struct.unpack_from('<IIII', d, o + 8)
    secs.append((va + base, vs, rp, rs)); o += 40
def rd(a, n):
    for va, vs, rp, rs in secs:
        if va <= a < va + max(vs, rs):
            return d[rp + a - va: rp + a - va + n]
for t in sys.argv[1:]:
    a = int(t, 16)
    print(t, struct.unpack('<d', rd(a, 8))[0], struct.unpack('<f', rd(a, 4))[0])
