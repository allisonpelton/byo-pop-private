# toorig.py <port script> <delta>: prints an origrun script for the original from a port -script file. A port line
# "K command args" is read before update K's widgets are updated; the original reads an input posted "at N" before update
# N+1, so a line becomes "at K+delta" with delta -1. The port's quit becomes origrun's quit after the last input (origrun
# cannot quit at an update; cmp.py compares up to the last input's update). Test-only port commands (board, combo,
# screenshot and the like) are left out.
import sys

INPUT = ('move', 'click', 'rclick', 'down', 'up', 'rdown', 'rup', 'key', 'keydown', 'keyup', 'text', 'char')

src, delta = sys.argv[1], int(sys.argv[2])
for line in open(src):
    line = line.split('#')[0].strip()
    if not line:
        continue
    k, cmd, *rest = line.split(None, 2)
    if cmd not in INPUT:
        continue
    if cmd == 'char':
        cmd = 'text'
    print("at %d %s %s" % (int(k) + delta, cmd, ' '.join(rest)))
print("quit")
