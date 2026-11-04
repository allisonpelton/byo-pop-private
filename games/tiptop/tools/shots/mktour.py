# mktour.py <port script> <end update> <header> <shot updates...>: a tour from a phase-one bot script (its lines up to
# <end update>) with "screenshot s<update>" lines at the given updates (a-b/k: every k from a to b), then a quit. The
# header's "|" starts a new comment line.
import sys
src, end, header = sys.argv[1], int(sys.argv[2]), sys.argv[3]
shots = set()
for a in sys.argv[4:]:
    if '/' in a:
        r, k = a.split('/')
        lo, hi = map(int, r.split('-'))
        shots.update(range(lo, hi + 1, int(k)))
    else:
        shots.add(int(a))
out = []
for l in open(src):
    if l.strip():
        k, c = l.split(None, 1)
        if int(k) <= end:
            out.append((int(k), 0, c.strip()))
out += [(u, 1, 'screenshot s%05d' % u) for u in shots if u <= end]
out.sort()
for h in header.split('|'):
    print('# ' + h)
for u, _, c in out:
    print(u, c)
print(end + 10, 'quit')
