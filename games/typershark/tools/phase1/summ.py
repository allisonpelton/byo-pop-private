#!/usr/bin/env python3
"""summ.py <port statelog>: what a run covered: update range, final score, lives, kinds of creatures seen, diver states, level numbers."""
import re, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cmp import read_port
p = read_port(sys.argv[1])
us = sorted(u for u in p if 'bupd' in p[u][0])
kinds = {}
dst = {}
lv = {}
for u in us:
    h, c = p[u]
    for k in c:
        kinds[k[0]] = kinds.get(k[0], 0) + 1
    dst[h['dstate']] = dst.get(h['dstate'], 0) + 1
    lv[h['lvl']] = lv.get(h['lvl'], 0) + 1
h, c = p[us[-1]]
print(f"board updates {len(us)} ({us[0]}..{us[-1]}); last: score={h['score']} bscore={h['bscore']} lives={h['lives']} lvl={h['lvl']} dstate={h['dstate']}")
print('kinds', kinds)
print('dstates', dst)
print('levels', lv)
