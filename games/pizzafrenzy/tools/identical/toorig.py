# toorig.py <port -realinput file> [delta]: prints an origrun script for the original from the port's input file. With
# -fast the port runs one frame an update, and its line "K command args" is pushed when update K starts and handled
# before the next frame; origrun's "at N" holds the original once its frame counter reads N, after frame N, so the input
# is handled before frame N+1. A line becomes "at K+delta" (delta 0 by default: the first case showed the input's
# effects on the same frames so). Points are game pixels, which are the original's window pixels (800x600, windowed).
# Keys become their virtual-key codes, and typed letters "text".
import sys

VK = {'Escape': 27, 'Backspace': 8, 'Return': 13, 'Enter': 13, 'Tab': 9, 'space': 32, 'Space': 32}

src = sys.argv[1]
delta = int(sys.argv[2]) if len(sys.argv) > 2 else 0
for line in open(src):
    line = line.split('#')[0].strip()
    if not line:
        continue
    words = line.split(' ')
    n = int(words[0]) + delta
    cmd, rest = words[1], words[2:]
    if cmd == 'quit':
        print("at %d move 0 0" % n)
        print("quit")
        break
    if rest and rest[0] == 'game':
        rest = rest[1:]
    if cmd == 'key':
        if rest[0] not in VK:
            sys.exit("toorig.py: no virtual-key code for " + rest[0])
        rest = [str(VK[rest[0]])]
    elif cmd == 'type':
        cmd = 'text'
    print("at %d %s %s" % (n, cmd, ' '.join(rest)))
