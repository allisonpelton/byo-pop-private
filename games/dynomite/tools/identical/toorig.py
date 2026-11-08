# toorig.py <port script> <delta> [shots]: prints an origrun script for the original from a port -script file. A port line
# "K command args" is read before update K's widgets are updated; the original reads an input posted "at N" before update
# N+1, so a line becomes "at K+delta" with delta -1. The port's quit becomes origrun's quit after the last input (origrun
# cannot quit at an update; cmp.py compares up to the last input's update). Test-only port commands (board, combo and
# the like) are left out; a screenshot becomes a move to where the cursor already is, so the original runs on to it and
# the comparison covers what the input before it did. With "shots" (a run inside Windows Sandbox, where origrun takes
# them) a screenshot becomes "at K+delta shot <name>" instead.
import sys

INPUT = ('move', 'click', 'rclick', 'down', 'up', 'rdown', 'rup', 'key', 'keydown', 'keyup', 'text', 'char', 'focus')

src, delta = sys.argv[1], int(sys.argv[2])
shots = len(sys.argv) > 3 and sys.argv[3] == 'shots'
if shots:
    # in the sandbox the loading thread is slower than the virtual clock: hold the game early for 45 s of real time,
    # in which the loader (which runs on the wall clock) finishes
    print("at 100 move 0 0")
    print("wait 45000")
pos = None
for line in open(src):
    # mkscript.py's one-side lines: "# orig <line>" is the original's alone, a line ending "# port" the port's
    if line.startswith('# orig '):
        line = line[7:]
    elif line.rstrip().endswith('# port'):
        continue
    line = line.split('#')[0].strip()
    if not line:
        continue
    k, cmd, *rest = line.split(None, 2)
    if cmd == 'screenshot' and shots:
        name = rest[0].replace(chr(92), '/').rsplit('/', 1)[-1]
        print("at %d shot %s" % (int(k) + delta, name[:-4] if name.endswith('.png') else name))
        continue
    if cmd == 'screenshot' and pos is not None:
        cmd, rest = 'move', [pos]
    if cmd not in INPUT:
        continue
    if cmd in ('move', 'click', 'rclick', 'down', 'up', 'rdown', 'rup'):
        pos = ' '.join(rest)
    if cmd == 'char':
        cmd = 'text'
    print("at %d %s %s" % (int(k) + delta, cmd, ' '.join(rest)))
print("quit")
