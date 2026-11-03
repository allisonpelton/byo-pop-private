# toorig.py <port script> <delta>: prints an origrun script for the original from a port -script file. A port line
# "K command args" is read before update K's widgets are updated; the original reads an input posted "at N" before update
# N+1, so a line becomes "at K+delta" with delta -1. Port-only commands (lua, word, battle, state, ...) are dropped;
# 'char' becomes 'text'.
import sys

src, delta = sys.argv[1], int(sys.argv[2])
KEEP = ('click', 'move', 'key', 'text', 'rclick', 'focus')
for line in open(src):
    line = line.split('#')[0].strip()
    if not line:
        continue
    k, cmd, *rest = line.split(None, 2)
    if cmd == 'char':
        cmd = 'text'
    if cmd not in KEEP:
        continue
    print("at %d %s %s" % (int(k) + delta, cmd, ' '.join(rest)))
