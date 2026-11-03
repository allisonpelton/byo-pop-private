# gen.py <seed> <end_ms> <name> [prefix-lines-file]: random play (a line every ~2.5 s at a random cell, the aim switched at
# random) as a runner script (build/cbw/<name>.txt) and an origrun script (build/origrun/<name>.txt). A runner click at
# T (its move at T-10) is the original's "at T-19 click", read by the original's message pump in the same frame.
import sys, random
seed, end, name = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
r = random.Random(seed)
ev = [(3000, 'click', 175, 313), (4000, 'click', 400, 563)]   # Start Game, the instructions' Play
if len(sys.argv) > 4:
    ev = [tuple([int(f[0]), f[1]] + [int(x) for x in f[2:]]) for f in (l.split() for l in open(sys.argv[4]) if l.strip())]
t = max(e[0] for e in ev) + 3500
x, y = next(((e[2], e[3]) for e in sorted(ev, reverse=True) if e[1] == 'click'), (400, 300))   # the cursor's last place: a right click does not move it
while t < end:
    if r.random() < 0.4:
        ev.append((t - 500, 'rclick', x, y))
    x, y = r.randint(120, 680), r.randint(130, 500)
    ev.append((t, 'click', x, y))
    t += r.randint(1500, 3500)
run, orig = [], []
for T, what, a, b in sorted(ev):
    if what == 'click':
        run += ["%d move %d %d" % (T - 10, a, b), "%d click %d %d" % (T, a, b)]
        orig.append("at %d click %d %d 30" % (T - 19, a, b))
    elif what == 'rclick':
        run += ["%d down 2" % T, "%d up 2" % (T + 30)]
        orig.append("at %d rclick %d %d 30" % (T - 19, a, b))
    elif what == 'key':
        run += ["%d hold %d" % (T, a), "%d release %d" % (T + 30, a)]
        orig.append("at %d key %d 30" % (T - 19, a))
run.append("%d end" % (end + 10))
orig += ["at %d move 0 0" % end, "quit"]
open('build/cbw/%s.txt' % name, 'w').write("\n".join(run) + "\n")
open('build/origrun/%s.txt' % name, 'w').write("\n".join(orig) + "\n")
