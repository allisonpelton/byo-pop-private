# toorig.py <port script> <delta>: prints an origrun script for the original from a port -script file. A port line
# "K command args" is read before update K's widgets are updated; the original reads an input posted "at N" before update
# N+1, so a line becomes "at K+delta-1". The delta is the number of updates the original's loading screen takes more than the
# port's (the first click presses Play once the bar is full): the bot waits for the same number of updates after it in both.
import sys

# The original's first run asks something before the loading screen's Play link works (a dialog of 336x316 at 206,42 with a
# button on each side of y 287, which the port has no counterpart of; its right button is No, as Alchemy's first question).
# Its start varies between runs, and so does the end of the loading, so the answer comes early and the first click of the
# script late (the delta).
ASK_NO = ("at 400 move 436 306", "at 410 click 436 306")

src, delta = sys.argv[1], int(sys.argv[2])
for line in ASK_NO:
    print(line)
out = []
for line in open(src):
    button = '#button' in line
    line = line.split('#')[0].strip()
    if not line:
        continue
    k, cmd, *rest = line.split(None, 2)
    if cmd == 'quit':
        continue
    if cmd == 'char':
        cmd = 'text'
    if cmd == 'click':
        # the port's script moves the mouse to the point first (MouseMoveTo): the original's widgets light and hit-test by it,
        # and a button that has just opened (a dialog's) ignores a click that comes in the same update as the move to it, so
        # the move to a button comes 2 updates earlier
        m = int(k) + delta - (3 if button else 1)
        out.append((m, "at %d move %s" % (m, ' '.join(rest))))
    out.append((int(k) + delta - 1, "at %d %s %s" % (int(k) + delta - 1, cmd, ' '.join(rest))))
for u, l in sorted(out, key=lambda e: e[0]):
    print(l)
