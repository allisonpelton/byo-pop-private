# Draws per update: an origrun run with the patched rand (snapshots from draw N on) against a port's -statelogcells=c log.
#   python -I games/noahsark/tools/rngc.py <origrun run.log> <port statelog> [seed] [rows shown]
# Prints the updates where the original made another number of draws than the port (the original's counter is the
# port's update + 1) and how many updates were compared.
import re
import sys

seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
shown = int(sys.argv[4]) if len(sys.argv) > 4 else 20
idx = {}
s = seed
for k in range(3000000):
    idx.setdefault(s, k)
    s = (s * 214013 + 2531011) % (1 << 32)
o = {}
for l in open(sys.argv[1], errors='replace'):
    m = re.search(r'watch 1: (\S+) -> (\S+) at update (\d+)', l)
    if m and m.group(2).lstrip('-').isdigit() and (int(m.group(2)) & 0xffffffff) in idx:
        o[int(m.group(3)) - 1] = idx[int(m.group(2)) & 0xffffffff]
p = {}
for l in open(sys.argv[2]):
    m = re.match(r'rngc (\d+) at update (\d+)', l)
    if m:
        p[int(m.group(2))] = int(m.group(1))
n = 0
bad = 0
off = None
for u in sorted(set(o) & set(p)):
    if u - 1 in o and u - 1 in p:
        n += 1
        d = o[u] - o[u - 1]
        e = p[u] - p[u - 1]
        if d != e:
            bad += 1
            if bad <= shown:
                print('update %d: the original made %d draws, the port %d' % (u, d, e))
print('%d updates compared, %d with another number of draws' % (n, bad))
