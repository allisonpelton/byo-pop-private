# statecmp.py <origrun log> <port statelog>: the battle engine's state (origrun watch 2, [gBattleEngine]+0x30) against the
# port's "estate" lines: the same states at the same updates, once the engine exists (the original's pointer is null before).
import re, sys
o = []
for l in open(sys.argv[1], errors='replace'):
    m = re.search(r'watch 2: (\S+) -> (\S+) at update (\d+)', l)
    if m and m.group(2) not in ('-', '4294967295'):
        o.append((int(m.group(3)), m.group(2)))
p = [(int(l.split()[0]), l.split()[2]) for l in open(sys.argv[2]) if len(l.split()) == 3 and l.split()[1] == 'estate' and l.split()[2] != '-']
TOL = int(sys.argv[3]) if len(sys.argv) > 3 else 6
last = min(o[-1][0], p[-1][0]) + TOL; o = [x for x in o if x[0] <= last]; p = [x for x in p if x[0] <= last]
n = min(len(o), len(p))
k = next((i for i in range(n) if o[i][1] != p[i][1] or abs(o[i][0] - p[i][0]) > TOL), n)
print("engine states: original %d changes, port %d" % (len(o), len(p)))
if k == n and len(o) == len(p):
    print("IDENTICAL engine states: all %d changes, none more than %d updates apart" % (n, max([abs(a[0] - b[0]) for a, b in zip(o, p)] or [0])))
else:
    print("first difference at change %d: original %s, port %s" % (k, o[k] if k < len(o) else None, p[k] if k < len(p) else None))
