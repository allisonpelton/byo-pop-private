#!/usr/bin/env python3
"""Find luaL_reg arrays ({name, function} pairs ending in {0, 0}) in BookwormAdventures.exe's data sections and
print them: the C functions each PopLua library (and each binding table) registers.

    python games/bookwormadv/tools/regtables.py [name ...]     # only the tables that contain one of the names
"""
import struct
import sys

d = open('re/bookwormadv/bin/BookwormAdventures.exe', 'rb').read()
SECS = [(0x797000, 0x397000, 0x7b5f2), (0x813000, 0x413000, 0x5a460)]


def cstr(va):
    for start, raw, size in SECS:
        if start <= va < start + size:
            o = raw + va - start
            e = d.index(b'\0', o)
            s = d[o:e]
            if 0 < len(s) < 80 and all(32 <= c < 127 for c in s):
                return s.decode()
    return None


def is_code(va):
    return 0x401000 <= va < 0x797000


want = set(sys.argv[1:])
for start, raw, size in SECS:
    i = 0
    while i < size - 8:
        n, f = struct.unpack_from('<II', d, raw + i)
        if cstr(n) and is_code(f):
            j = i
            rows = []
            while j < size - 8:
                n, f = struct.unpack_from('<II', d, raw + j)
                if n == 0 and f == 0:
                    break
                s = cstr(n)
                if not s or not is_code(f):
                    rows = None
                    break
                rows.append((s, f))
                j += 8
            if rows and len(rows) >= 3 and (not want or want & {r[0] for r in rows}):
                print('%#x  %d entries' % (start + i, len(rows)))
                print('   ' + ' '.join('%s=%x' % r for r in rows))
            i = j + 8 if rows else i + 4
        else:
            i += 4
