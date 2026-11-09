#!/usr/bin/env python3
"""peek_cre.py <tag of a finished typist run> <seed> <list> <first offset> <last offset> <update> [<update> ...]
Reads raw ints of the creatures of one Board list out of the original at the given updates: for each creature, the ints at
offsets first..last (hex, step 4) from the pointer in the list (the part of the complete object that the list holds), played
with the inputs of that run (build/ts-p1/<tag>.steps.json). Prints one row per creature and update."""
import json, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from tsp import *
from cmp import *

tag, seed, lst, first, last = sys.argv[1], int(sys.argv[2]), sys.argv[3], int(sys.argv[4], 16), int(sys.argv[5], 16)
ups = [int(x) for x in sys.argv[6:]]
steps = [tuple(x) for x in json.load(open(os.path.join(OUT, tag + '.steps.json')))]
lo = dict((n, (b, e)) for n, b, e, k in LISTS)[lst]
lo = (lo[0], lo[1] if lo[1] is not None else lo[0])
offs = list(range(first, last + 1, 4))
steps = [s for s in steps if s[1] != 'peek']
for u in ups:
    ex = [lo[0], lo[1]]
    for i in range(6):
        ex += [ind(elem_slot(lo[0], i), o) if o >= 0 else ind(elem_slot(lo[0], i), o) for o in offs]
    steps.append((u, 'peek', ex))
lg = run_orig(orig_script(steps), tag + '.pk', seed, 'users-clean')
d = parse(lg)
for u in ups:
    r = d.get(u)
    if not r:
        print(u, 'no peek')
        continue
    n = ((u32(r.get(lo[1])) or 0) - (u32(r.get(lo[0])) or 0)) // 4
    print('u', u, 'count', n)
    for i in range(min(n, 6)):
        vals = [r.get(ind(elem_slot(lo[0], i), o)) for o in offs]
        print(' #%d' % i, ' '.join('%x:%s' % (o, '-' if v is None else format(v & 0xffffffff, 'x')) for o, v in zip(offs, vals)))
