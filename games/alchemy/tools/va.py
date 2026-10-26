#!/usr/bin/env python3
"""va.py <address>:<d|f|i|s> ...: a double, float, int or C string at a virtual address of re/alchemy/bin/AlchemyGame.exe (constants of the code)."""
import struct, sys
d = open('re/alchemy/bin/AlchemyGame.exe', 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]
opt = struct.unpack_from('<H', d, pe + 20)[0]
base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
secs = []
o = pe + 24 + opt
for i in range(nsec):
    vs, va, rs, ro = struct.unpack_from('<IIII', d, o + 8 + i * 40)
    secs.append((va + base, max(vs, rs), ro))
def off(a):
    for s, n, ro in secs:
        if s <= a < s + n: return ro + a - s
    raise SystemExit('not mapped %x' % a)
for arg in sys.argv[1:]:
    a, t = arg.split(':')
    a = int(a, 16); o = off(a)
    if t == 'd': print('%x double %r' % (a, struct.unpack_from('<d', d, o)[0]))
    elif t == 'f': print('%x float %r' % (a, struct.unpack_from('<f', d, o)[0]))
    elif t == 'i': print('%x int %d' % (a, struct.unpack_from('<i', d, o)[0]))
    elif t == 's': print('%x str %r' % (a, d[o:d.index(b'\0', o)].decode('latin1')))
