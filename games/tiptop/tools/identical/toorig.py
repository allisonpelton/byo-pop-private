# toorig.py <port script> <delta>: prints an origrun script for the original from a port -script file. A port line
# "K command args" is run before update K's pass; the original reads an input posted "at N" in pass N, the pass its
# counter (0x48cff0, counted at the loop's end) reads N in, so a line becomes "at K+delta". Typed characters become
# origrun's text, a click its held click; key lines are left out (the original reads its keys with DirectInput, which
# posted input does not reach; -autoplayui makes none). The script ends with a quit.
import sys

# the end: a little game time after the last input, then the run ends (else the game runs free to the timeout)
END = ("wait 200", "quit")

src, delta = sys.argv[1], int(sys.argv[2])
for line in open(src):
    line = line.split('#')[0].strip()
    if not line:
        continue
    k, cmd, *rest = line.split(None, 2)
    if cmd in ('quit', 'key'):
        continue
    if cmd == 'char':
        cmd = 'text'
    print("at %d %s %s" % (int(k) + delta, cmd, ' '.join(rest)))
for line in END:
    print(line)
