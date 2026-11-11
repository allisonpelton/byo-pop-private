"""Compares two PE files section by section: for each section its size and the number of differing bytes, and the file
ranges that differ outside the sections (the Collection check, GAME_NOTES.md "The Collection version").
    python cmpexe.py <a.exe> <b.exe>"""
import struct
import sys


def sections(data):
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    count = struct.unpack_from('<H', data, pe + 6)[0]
    opt = struct.unpack_from('<H', data, pe + 20)[0]
    table = pe + 24 + opt
    out = []
    for i in range(count):
        name, vsize, va, rsize, raw = struct.unpack_from('<8sIIII', data, table + 40 * i)
        out.append((name.rstrip(b'\0').decode(), va, vsize, raw, rsize))
    return out


a, b = (open(p, 'rb').read() for p in sys.argv[1:3])
print('sizes', len(a), len(b))
sa, sb = sections(a), sections(b)
for (name, va, vsize, raw, rsize), other in zip(sa, sb):
    da, db = a[raw:raw + rsize], b[other[3]:other[3] + other[4]]
    n = min(len(da), len(db))
    diff = [i for i in range(n) if da[i] != db[i]]
    span = ''
    if diff:
        span = ' first at va 0x%x, last at va 0x%x' % (0x400000 + va + diff[0], 0x400000 + va + diff[-1])
    print('%-8s raw %d / %d, %d bytes differ%s' % (name, rsize, other[4], len(diff) + abs(len(da) - len(db)), span))
head = min(sa[0][3], sb[0][3])
print('headers: %d bytes differ' % sum(1 for i in range(head) if a[i] != b[i]))
enda, endb = max(s[3] + s[4] for s in sa), max(s[3] + s[4] for s in sb)
print('after the last section: %d and %d bytes' % (len(a) - enda, len(b) - endb))
