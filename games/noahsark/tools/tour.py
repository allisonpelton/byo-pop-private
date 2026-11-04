"""Write the port's and the original's scripts for one tour of screens (phase two of the identical-play pass).

    python games/noahsark/tools/tour.py <tour.txt> <out dir>

<tour.txt> holds lines "<update> <command> [args]": click x y, key n, char c, text s, focus lose|regain, shot <name>,
quit. The port's script (<out>/port.txt) gets "screenshot <out>/port/<name>.png" for a shot, at the update; the
original's (<out>/orig.txt) "at <update> shot <name>" (the original's capture shows the state its counter's number names, one update before the port's at the same number, measured on the puzzle swap slide), the other commands at <update + 1>, so
that a click reaches the game in the same update as the port's. Shots are named like the original's captures (shots/<name>.png), so shotdiff pairs them.
"""
import os
import sys

tour, out = sys.argv[1], sys.argv[2]
os.makedirs(os.path.join(out, 'port'), exist_ok=True)
port, orig = [], []
last = 0
for ln in open(tour):
    ln = ln.split('#')[0].strip()
    if not ln:
        continue
    u, cmd, *rest = ln.split(None, 2)
    arg = rest[0] if rest else ''
    u = int(u)
    last = max(last, u)
    if cmd == 'shot':
        port.append('%d screenshot %s/port/%s.png' % (u, os.path.abspath(out).replace(os.sep, '/'), arg))
        orig.append('at %d shot %s' % (u, arg))
    elif cmd == 'quit':
        pass
    else:
        port.append('%d %s %s' % (u, cmd, arg))
        orig.append('at %d %s %s' % (u + 1, cmd, arg))
port.append('%d quit' % (last + 20))
orig.append('at %d move 1 1' % (last + 20))
orig.append('quit')
open(os.path.join(out, 'port.txt'), 'w').write('\n'.join(port) + '\n')
open(os.path.join(out, 'orig.txt'), 'w').write('\n'.join(orig) + '\n')
