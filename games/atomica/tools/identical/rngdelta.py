#!/usr/bin/env python3
"""Where the random number generator's draws per update differ between the original and the port: reads compare.py's
--dump output of rng_j and prints, for a range of updates, the draws of each update on both sides.

    python games/atomica/tools/identical/compare.py <scenario> --names rng_j --dump 99999 | python .../rngdelta.py <from> <to>
"""
import sys


def parse(line):
    pts = [tuple(map(int, x.split(':'))) for x in line.split(': ', 1)[1].split()]
    out = {}
    cur = 0
    for u, v in pts:
        out[u] = v
    return out


def main():
    lo, hi = int(sys.argv[1]), int(sys.argv[2])
    orig = port = None
    for l in sys.stdin:
        if l.startswith('rng_j orig'):
            orig = parse(l)
        elif l.startswith('rng_j port'):
            port = parse(l)

    def draws(d):
        res = {}
        prev = 0
        cur = 0
        for u in range(0, hi + 2):
            if u in d:
                nxt = d[u]
                res[u] = (nxt - cur) % 55
                cur = nxt
            else:
                res[u] = 0
        return res
    o, p = draws(orig), draws(port)
    for u in range(lo, hi + 1):
        print('%d orig %d port %d%s' % (u, o[u], p[u], '' if o[u] == p[u] else '   <--'))


main()
