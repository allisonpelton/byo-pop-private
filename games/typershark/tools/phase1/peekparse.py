#!/usr/bin/env python3
"""Parse an origrun log's `peek u=N expr=value ...` lines into {update: {expr: int or None}} (several lines of one update merge)."""
import re, sys


def parse(path):
    out = {}
    for line in open(path, errors='replace'):
        m = re.search(r'\bpeek u=(-?\d+) (.*)$', line)
        if not m:
            continue
        u = int(m.group(1))
        d = out.setdefault(u, {})
        for e in m.group(2).split(' '):
            if '=' not in e:
                continue
            k, v = e.rsplit('=', 1)
            d[k] = None if v == '-' else int(v)
    return out


def dbl(lo, hi):
    """a double from its two peeked ints (low word, high word)"""
    import struct
    if lo is None or hi is None:
        return None
    return struct.unpack('<d', struct.pack('<II', lo & 0xffffffff, hi & 0xffffffff))[0]


if __name__ == '__main__':
    d = parse(sys.argv[1])
    print(len(d), 'updates')
