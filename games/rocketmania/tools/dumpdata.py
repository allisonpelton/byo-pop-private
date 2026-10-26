#!/usr/bin/env python3
"""Dump data of the carved game exe (re/rocketmania/bin/RocketManiaGame.exe) by virtual address.

    python games/rocketmania/tools/dumpdata.py 4c78b8 20        # 20 signed 32-bit words
    python games/rocketmania/tools/dumpdata.py 4e6a54 1 s       # a C string
    python games/rocketmania/tools/dumpdata.py 4e6a54 4 d       # doubles ('f' floats, 'b' bytes, 'u' unsigned words)
"""
import struct, sys
from ptrtable import d, off

va = int(sys.argv[1], 16)
n = int(sys.argv[2]) if len(sys.argv) > 2 else 1
kind = sys.argv[3] if len(sys.argv) > 3 else 'i'
if kind == 's':
    o = off(va)
    print(d[o:d.index(b'\0', o)].decode('latin1'))
    sys.exit(0)
size = {'i': 4, 'u': 4, 'f': 4, 'd': 8, 'b': 1}[kind]
fmt = {'i': '<i', 'u': '<I', 'f': '<f', 'd': '<d', 'b': '<B'}[kind]
for i in range(n):
    v = struct.unpack_from(fmt, d, off(va + i * size))[0]
    print('%x: %s' % (va + i * size, ('0x%x' % v) if kind == 'u' else v))
