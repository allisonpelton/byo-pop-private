#!/usr/bin/env python3
"""peek_more.py <tag of a finished typist run> <seed> <extra inputs> <update> [<update> ...] -- <expr> [<expr> ...]
Like peek_expr.py, with extra letters typed after the recorded run's last input. <extra inputs> is  U:c,U:c  (the original's update
number and the letter; "-" for none): the recorded run stops at its `quit`, and its inputs end there, so a stretch past it (to see
what the original does after the last recorded letter) needs the letters the port's bot would have typed next."""
import json, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from tsp import *
from cmp import *

args = sys.argv[1:]
tag, seed, extra = args[0], int(args[1]), args[2]
split = args.index('--')
ups = [int(x) for x in args[3:split]]
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
if extra != '-':
    for item in extra.split(','):
        u, c = item.split(':')
        steps.append((int(u), 'text1', c))
for u in ups:
    steps.append((u, 'peek', exprs))
# OFFSET=<n> moves every input and peek n updates later: the original's loading thread runs on the wall clock, and on a busy machine
# the menu clicks can come before it has loaded (the menu draws no random numbers, so a later start changes nothing else)
OFF = int(os.environ.get('OFFSET', '0'))
steps = [(s[0] + OFF,) + tuple(s[1:]) for s in steps]
d = {}
for attempt in range(3):
    lg = run_orig(orig_script(steps), tag + '.pm', seed, 'users-clean')
    d = parse(lg)
    if any(r.get(exprs[0]) is not None for r in d.values()):
        break
    print('the original never had a board (late loading), run %d of 3' % (attempt + 1), flush=True)
d = dict((u - OFF, r) for u, r in d.items())
for u in ups:
    r = d.get(u)
    if not r:
        print(u, 'no peek')
        continue
    print(u, ' '.join('%s=%s' % (raw[i], '-' if r.get(exprs[i]) is None else s32(r.get(exprs[i]))) for i in range(len(exprs))))
