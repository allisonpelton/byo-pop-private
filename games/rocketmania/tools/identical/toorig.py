# toorig.py <port script> <delta> [<last update>]: prints an origrun script for the original from the bot's script file. A line
# "K command args" runs before update K's widgets are updated; the original reads an input posted "at N" before update N+1,
# so a line becomes "at K+delta" with delta -1. The port's "char" is origrun's "text"; the port's "quit" is dropped (origrun
# ends the run with its own quit after the last line).
import sys

src, delta = sys.argv[1], int(sys.argv[2])
for line in open(src):
    line = line.split('#')[0].strip()
    if not line:
        continue
    k, cmd, *rest = line.split(None, 2)
    if cmd == 'quit':
        continue
    if cmd == 'char':
        cmd = 'text'
    print("at %d %s %s" % (int(k) + delta, cmd, ' '.join(rest)))
print("quit")
