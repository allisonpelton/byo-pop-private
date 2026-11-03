# toorig.py <port script> [delta]: prints an origrun script for the original from a port -script file. With -fast the port
# runs one frame an update, and its line "K command args" is handled before frame K; its state log puts what the input
# changed under frame K-1 (it logs once more before each frame). origrun's "at N" holds the original at its first wait once
# the counter (counted at the start of a frame) reads N, which is after frame N, so the input is handled before frame N+1
# and what it changes is logged under N. So a line becomes "at K+delta" with delta -1.
import sys

src = sys.argv[1]
delta = int(sys.argv[2]) if len(sys.argv) > 2 else -1
for line in open(src):
    line = line.split('#')[0].strip()
    if not line:
        continue
    k, cmd, *rest = line.split(None, 2)
    n = int(k) + delta
    if cmd == 'quit':
        print("at %d move 0 0" % n)
        print("quit")
        break
    if cmd == 'char':
        cmd = 'text'
    print("at %d %s %s" % (n, cmd, ' '.join(rest)))
