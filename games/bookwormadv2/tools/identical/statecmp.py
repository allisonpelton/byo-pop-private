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
if not o or not p:
    print('engine states: original %d changes, port %d (no battle engine)' % (len(o), len(p))); sys.exit(0)
last = min(o[-1][0], p[-1][0]) + TOL; o = [x for x in o if x[0] <= last]; p = [x for x in p if x[0] <= last]
# The port logs every state BattleEngine::SetState sets (a state left in the update it was entered too, such as a lost
# turn's EndLostTurnRound going 5, then 1, on the grid's click); the original's watch polls every millisecond and sees
# such a passing state only some of the time. So a passing state of the port may go unmatched; every other change of
# either side must be matched, in order, within TOL updates.
passing = [i + 1 < len(p) and p[i + 1][0] == p[i][0] for i in range(len(p))]
i = j = 0; worst = 0; bad = None
while i < len(o) and j < len(p):
    if o[i][1] == p[j][1] and abs(o[i][0] - p[j][0]) <= TOL:
        worst = max(worst, abs(o[i][0] - p[j][0])); i += 1; j += 1
    elif passing[j]:
        j += 1
    else:
        bad = (i, j); break
while bad is None and j < len(p) and passing[j]:
    j += 1
print("engine states: original %d changes, port %d (%d passing)" % (len(o), len(p), sum(passing)))
if bad is None and i == len(o) and j == len(p):
    print("IDENTICAL engine states: all %d changes of the original, none more than %d updates apart" % (len(o), worst))
else:
    if bad is None:
        bad = (i, j)
    print("first difference at change %d: original %s, port %s" % (bad[0], o[bad[0]] if bad[0] < len(o) else None, p[bad[1]] if bad[1] < len(p) else None))
