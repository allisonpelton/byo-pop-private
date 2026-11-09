#!/usr/bin/env python3
"""peek_expr.py <tag of a finished typist run> <seed> <update> [<update> ...] -- <expr> [<expr> ...]
Reads address expressions out of the original at the given updates, played with the inputs of that run (steps.json), and prints the
values (signed; '-' where unreadable). Expressions use the shorthand  APP+0x65c  (an app field's address), LM+0x8c (a field of the
lesson manager), BRD+0x64 (a Board field), PL+0xb8 (a Player field)."""
import json, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from tsp import *
from cmp import *

args = sys.argv[1:]
tag, seed = args[0], int(args[1])
split = args.index('--')
ups = [int(x) for x in args[2:split]]
raw = args[split + 1:]


def expand(e):
    for pre, f in (('APP+', lambda o: app(o)), ('LM+', lambda o: ind(app(0x65c), o)), ('BRD+', lambda o: board(o)),
                   ('PL+', lambda o: player(o))):
        if e.startswith(pre):
            return f(int(e[len(pre):], 16))
    return e


exprs = [expand(e) for e in raw]
steps = [tuple(x) for x in json.load(open(os.path.join(OUT, tag + '.steps.json')))]
steps = [s for s in steps if s[1] != 'peek']
for u in ups:
    steps.append((u, 'peek', exprs))
lg = run_orig(orig_script(steps), tag + '.pe', seed, 'users-clean')
d = parse(lg)
for u in ups:
    r = d.get(u)
    if not r:
        print(u, 'no peek')
        continue
    print(u, ' '.join('%s=%s' % (raw[i], '-' if r.get(exprs[i]) is None else s32(r.get(exprs[i]))) for i in range(len(exprs))))
