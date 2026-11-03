#!/usr/bin/env python3
"""Print a table of pointers to C strings in the carved game exe (re/rocketmania/bin/RocketManiaGame.exe).

    python games/rocketmania/tools/ptrtable.py 4e6a80 [count]     # until an entry is not a pointer to a string
"""
import struct, sys

EXE = 're/rocketmania/bin/RocketManiaGame.exe'
d = open(EXE, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]
opt = struct.unpack_from('<H', d, pe + 20)[0]
base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + opt + i * 40
    vsize, va, rsize, raw = struct.unpack_from('<IIII', d, o + 8)
    secs.append((base + va, max(vsize, rsize), raw))


def off(va):
    for s, n, raw in secs:
        if s <= va < s + n:
            return raw + va - s
    return None


def cstr(va):
    o = off(va)
    if o is None:
        return None
    e = d.index(b'\0', o)
    s = d[o:e]
    return s.decode('latin1') if all(32 <= c < 127 or c in (9, 10, 13) for c in s) and s else None


if __name__ == '__main__':
    va = int(sys.argv[1], 16)
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 100000
    for i in range(n):
        p = struct.unpack_from('<I', d, off(va + 4 * i))[0]
        s = cstr(p)
        if s is None:
            if len(sys.argv) <= 2:
                break
            print(i, hex(p))
        else:
            print(i, s)
