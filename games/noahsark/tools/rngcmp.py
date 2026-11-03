# Compare the C generator's progress in an origrun run with a port's -statelogcells=r log: the original's patched rand
# keeps a draw count and a snapshot of its state every few draws (the patch list is git-ignored); each snapshot is turned
# into its draw number by running the generator from the seed. Both log the latest snapshot at each update; the runs
# agree where the draw numbers are the same at the same update (the original's counter is the port's update + 1, and
# its poll can be one update late).
#   python -I games/noahsark/tools/rngcmp.py <origrun run.log> <port statelog> [seed] [rows shown]
import re
import sys

seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
shown = int(sys.argv[4]) if len(sys.argv) > 4 else 8
idx = {}
s = seed
for k in range(3000000):
    idx.setdefault(s, k)
    s = (s * 214013 + 2531011) % (1 << 32)


def orig_rows(path):
    r = []
    for l in open(path, errors='replace'):
        m = re.search(r'watch 1: (\S+) -> (\S+) at update (\d+)', l)
        if m and m.group(2).lstrip('-').isdigit() and int(m.group(3)) > 1:
            r.append((int(m.group(3)) - 1, idx.get(int(m.group(2)) & 0xffffffff)))
    return r


def port_rows(path):
    r = []
    for l in open(path):
        m = re.match(r'rng (\d+) -> (\d+) at update (\d+)', l)
        if m:
            r.append((int(m.group(3)), idx.get(int(m.group(2)))))
    return r


# the update at which each snapshot (draw number) was first seen
o = {}
for u, v in orig_rows(sys.argv[1]):
    o.setdefault(v, u)
p = {}
for u, v in port_rows(sys.argv[2]):
    p.setdefault(v, u)
bad = 0
n = 0
for v in sorted(o, key=lambda k: o[k]):
    if v is None or v not in p:
        continue
    n += 1
    if abs(p[v] - o[v]) > 1:
        bad += 1
        if bad <= shown:
            print('draw %d: orig update %d, port update %d' % (v, o[v], p[v]))
print('%d draw numbers reached by both, %d reached at updates more than one apart' % (n, bad))
