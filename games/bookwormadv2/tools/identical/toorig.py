# toorig.py <port script> <delta>: prints an origrun script for the original from a port -script file. A port line
# "K command args" is read before update K's widgets are updated; the original reads an input posted "at N" before update
# N+1, so a line becomes "at K+delta" with delta -1. Port-only commands (lua, word, battle, state, ...) are dropped;
# 'char' becomes 'text'.
#
# toorig.py <port script> <delta> <anchor> <expr>: the lines from update <anchor> on (where the port's first run made its
# battle engine, at the end of the chapter load) wait for the original's own load: "waitfor <expr> != 0" (the original's
# engine pointer), "mark", then "at +N" counted from the anchor. The original's loading thread runs on the wall clock, so
# its load spans any number of updates; the case shifts the port's replay by the update that mark reads.
import sys

src, delta = sys.argv[1], int(sys.argv[2])
anchor = int(sys.argv[3]) if len(sys.argv) > 4 else None
expr = sys.argv[4] if len(sys.argv) > 4 else None
KEEP = ('click', 'move', 'key', 'text', 'rclick', 'focus')
marked = False
for line in open(src):
    line = line.split('#')[0].strip()
    if not line:
        continue
    k, cmd, *rest = line.split(None, 2)
    if cmd == 'char':
        cmd = 'text'
    if cmd not in KEEP:
        continue
    if anchor is not None and int(k) >= anchor:
        if not marked:
            print("waitfor %s != 0 600000" % expr)
            print("mark")
            marked = True
        print("at +%d %s %s" % (int(k) - anchor + delta, cmd, ' '.join(rest)))
        continue
    print("at %d %s %s" % (int(k) + delta, cmd, ' '.join(rest)))
