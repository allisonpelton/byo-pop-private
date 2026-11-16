# randseq.py <port state log> <origrun log>: the random state's draws, in order, against the original's. The original's
# log has a peek of the mirrored state (0x541a04) at every update ("peek u=N 0x541a04=V"), the port's log the state
# after each frame ("N rand V"). The frame a value shows at can differ by one between the two (a peek is taken where
# origrun's counter goes up, which is after the mode's update and before the scene's), and a peek can see a frame's
# state between the mode's draws and the particles', so the port's values have to come in the original's order, each at
# most two frames from where the original has it, with the original free to have values in between; a port value the
# peeks step over is let through when the next one is the original's next. Exits 1 at the first port value that does
# not, printing both sides around it.
import re
import sys

PEEK = re.compile(r'peek u=(\d+) 0x541a04=(-?\d+)')
SLACK = 2


def signed(v):
    n = int(v) & 0xffffffff
    return n - (1 << 32) if n >= 1 << 31 else n


def runs(pairs):
    # the values in the order they come, a value held over frames counted once, at its first frame
    out = []
    for frame, value in pairs:
        if not out or out[-1][1] != value:
            out.append((frame, value))
    return out


port = runs((int(w[0]), signed(w[2])) for w in (l.split() for l in open(sys.argv[1])) if len(w) == 3 and w[1] == 'rand')
orig = runs((int(m.group(1)), signed(m.group(2))) for m in (PEEK.search(l) for l in open(sys.argv[2])) if m)
if not orig:
    sys.exit('randseq.py: no peeks of the random state in ' + sys.argv[2])
last = orig[-1][0]
j = 0
for i, (frame, value) in enumerate(port):
    if frame > last:
        break
    k = j
    while k < len(orig) and orig[k][1] != value:
        k += 1
    if k == len(orig) or abs(orig[k][0] - frame) > SLACK:
        # a peek can fall where it skips a value the port's frame ended on (the next frame's mode drew on it before
        # the peek): that is no difference when the port's next value is the original's next
        if i + 1 < len(port) and j < len(orig) and orig[j][1] == port[i + 1][1] and \
                abs(orig[j][0] - port[i + 1][0]) <= SLACK:
            continue
        print('rand        DIFFERS at frame %d: the port has %d there' % (frame, value))
        print('   port:     ' + ' '.join('%d:%d' % p for p in port[max(0, i - 3):i + 4]))
        print('   original: ' + ' '.join('%d:%d' % p for p in orig[max(0, j - 3):j + 6]))
        sys.exit(1)
    j = k + 1
print('rand        port %d values  original %d  same order to frame %d'
      % (len(port), len(orig), min(last, port[-1][0])))
