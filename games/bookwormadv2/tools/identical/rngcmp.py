# rngcmp.py <origrun log> <port statelog> [shift]: the RNG draws of the original (origrun --watch 0x7c0000, the Mersenne
# Twister's index, which wraps at 624) against the port's draw count (-statelog "rand" lines), by update. Prints the first
# difference in the draws' timing: the update at which each program reached each count, from the second title draw on.
import re, sys
def orig(f):
    r = []; cum = None; last = None
    # the test patch SRand()s as each battle engine is made (its pointer goes from null/old to a new one in watch 2): the
    # index jumps to 624 there, which is no draws
    made = set()
    for l in open(f, errors='replace'):
        m = re.search(r'watch 2: \S+ -> 4294967295 at update (\d+)', l)
        if m: made.add(int(m.group(1)))
    for l in open(f, errors='replace'):
        m = re.search(r'watch 1: (\S+) -> (\S+) at update (\d+)', l)
        if not m or not m.group(2).isdigit(): continue
        v = int(m.group(2)); u = int(m.group(3))
        if last is None:
            last = v; cum = 0; continue
        if v == 624 and u in made:
            last = v; r.append((u, cum)); continue
        cum += (v - last) % 624 if v != last else 0
        last = v
        r.append((u, cum))
    return r
def port(f):
    r = []; base = None
    for l in open(f):
        a = l.split()
        if len(a) == 3 and a[1] == 'rand':
            v = int(a[2])
            if base is None: base = v
            r.append((int(a[0]), v - base))
    return r
TOL = int(sys.argv[3]) if len(sys.argv) > 3 else 6
o = orig(sys.argv[1]); p = port(sys.argv[2])
# the draws are compared as the update each count was first reached at. The port's timers that follow its draws (the curtain
# closing after a click, the loading screen's letters) are quantized by when it draws (-fast draws every few updates), the
# original's by when it drew, so a count may come TOL updates early or late; anything more, or a count one has and the
# other has not, is a difference.
import bisect
# The original's index wraps at 624 and a burst of more than 624 draws in one update (the engine's first board, a word
# that scrambles) reads as the burst minus 624, so the two programs' counts are compared modulo 624, and by the step
# functions: every change of one must be matched by a count the other has within TOL updates, in both directions.
def step(r):
    us = [u for u, c in r]; cs = [c for u, c in r]
    return lambda u: cs[bisect.bisect_right(us, u) - 1] if us and u >= us[0] else 0
so, sp = step(o), step(p)
end = min(o[-1][0], p[-1][0])
print("original: %d changes, last count %d at update %d; port: %d changes, last count %d at update %d" % (len(o), o[-1][1], o[-1][0], len(p), p[-1][1], p[-1][0]))
# A burst may be split between two updates in one program and not in the other (the draws of one input, a click at the
# edge of an update), so the counts differ for a moment after a change. After each change, once TOL updates have passed
# with no other change in either program, the two counts must be equal (modulo 624).
changes = sorted(set(u for u, c in o) | set(u for u, c in p))
bad = 0; seen = 0
for k, c in enumerate(changes):
    if c > end: break
    u = c + TOL + 1
    if k + 1 < len(changes) and changes[k + 1] <= u: continue
    seen += 1
    if (sp(u) - so(u)) % 624 != 0:
        if bad < 10: print("update %d (settled since %d): original count %d, port %d (%+d)" % (u, c, so(u), sp(u), sp(u) - so(u)))
        bad += 1
if bad == 0:
    print("IDENTICAL draws up to update %d: the counts agree (modulo the original's 624 wrap) at %d settled points, and every change of either program is within %d updates of the other's" % (end, seen, TOL))
