# The original's generator state at each level's start and at the end of each get-ready, from an origrun run whose four
# watches are the cells of the generator patch (0x49c608, 0x49c60c, 0x49c614, 0x49c618: the state after the first draw after
# a change of the level, the level, the state after the first draw after a change of the board state, the board state),
# written as the lines of the port's -resync file ("n <level> <state> <update>", "r <level> <state> <update>": the state before that draw, the original's update counter).
#   python -I games/noahsark/tools/resync.py <origrun run.log> <out file> [<run.log of a run that watches the board state>]
# With the second log the update of each get-ready end is the one that run's state watch gives (the key watch can come an update late).
import re
import sys

M = 1 << 32
INV = pow(214013, -1, M)


def back(s):
    return ((s - 2531011) * INV) % M


rows = {1: [], 2: [], 3: [], 4: []}
for ln in open(sys.argv[1], errors='replace'):
    m = re.search(r'watch (\d): (\S+) -> (\S+) at update (\d+)', ln)
    if m and m.group(3).isdigit() and int(m.group(4)) > 0:
        rows[int(m.group(1))].append((int(m.group(4)), int(m.group(3))))
out = []
levels = rows[2]
for i, (u, snap) in enumerate(rows[1]):
    if i < len(levels) and levels[i][1] >= 1:
        out.append('n %d %d %d' % (levels[i][1], back(snap), u))
states = rows[4]
prev = None
for i, (u, snap) in enumerate(rows[3]):
    if i >= len(states):
        break
    s = states[i][1]
    lev = 0
    for (lu, lv) in levels:
        if lu <= u + 1:
            lev = lv
    if s == 1 and prev == 0 and lev >= 1:
        out.append('r %d %d %d' % (lev, back(snap), states[i][0]))
    prev = s
if len(sys.argv) > 3:
    ends = []
    for ln in open(sys.argv[3], errors='replace'):
        m = re.search(r'watch 1: 0 -> 1 at update (\d+)', ln)
        if m:
            ends.append(int(m.group(1)) + 1)
    k = 0
    for i, o in enumerate(out):
        if o[0] == 'r':
            if k < len(ends):
                f = o.split()
                out[i] = '%s %s %s %d' % (f[0], f[1], f[2], ends[k])
            k += 1
open(sys.argv[2], 'w').write('\n'.join(out) + '\n')
print('%d events (%d level starts, %d get-ready ends)' % (len(out), sum(1 for o in out if o[0] == 'n'), sum(1 for o in out if o[0] == 'r')))
