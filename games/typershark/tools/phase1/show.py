#!/usr/bin/env python3
"""show.py <tag> <update> [<update2>]: the original's and the port's state side by side at an update (or a range), from a
comparison run's cache (build/ts-p1/<tag>.orig.json and <tag>.port.sl)."""
import json, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cmp import read_port, OUT


def line(h, c, keys):
    hs = ' '.join(f'{k}={h[k]:.4f}' if isinstance(h.get(k), float) else f'{k}={h.get(k)}' for k in keys if k in h)
    cs = ' '.join(f'{k}:{w}@{x:.2f},{y:.2f},{s},{t}' for k, w, x, y, s, t in c)
    return hs + ' | ' + cs


if __name__ == '__main__':
    tag = sys.argv[1]
    lo = int(sys.argv[2]); hi = int(sys.argv[3]) if len(sys.argv) > 3 else lo
    o = json.load(open(os.path.join(OUT, tag + '.orig.json')))
    p = read_port(os.path.join(OUT, tag + '.port.sl'))
    keys = ['bupd', 'score', 'bscore', 'lives', 'time', 'hint', 'typed', 'ty', 'dstate', 'dive', 'emp', 'rng']
    for u in range(lo, hi + 1):
        if str(u) in o:
            h, c = o[str(u)]
            print(f'{u} O', line(h, [tuple(x) for x in c], keys))
        if u in p:
            h, c = p[u]
            print(f'{u} P', line(h, c, keys))
