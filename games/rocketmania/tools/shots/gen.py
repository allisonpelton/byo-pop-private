# gen.py <bot script> <first shot update> [<every>] [<max shots>]: makes a tour from a phase-one bot script (the port's -script
# format, "<update> <command> <args>"): the script's own lines plus "screenshot <name>" lines: one three updates after the first input
# after a quiet stretch of 40 updates (a new screen or dialog), and one every <every> updates (default 300) from there on, capped at
# <max shots> (default 120). Names are s<update>.
import sys
src, first = sys.argv[1], int(sys.argv[2])
every = int(sys.argv[3]) if len(sys.argv) > 3 else 300
cap = int(sys.argv[4]) if len(sys.argv) > 4 else 120
GAP = int(sys.argv[5]) if len(sys.argv) > 5 else 40
lines = []
for l in open(src):
    l = l.split('#')[0].strip()
    if l:
        k, cmd, *rest = l.split(None, 2)
        lines.append((int(k), cmd, ' '.join(rest)))
shots = set()
last_in = -10**9
last_shot = -10**9
for k, cmd, a in lines:
    if cmd == 'quit':
        continue
    if k - last_in >= GAP and k >= first:
        shots.add(k + 3)
    last_in = k
end = lines[-1][0]
k = first
while k < end:
    shots.add(k)
    k += every
shots = sorted(shots)
if len(shots) > cap:
    shots = shots[:cap]
out = [(k, cmd, a) for k, cmd, a in lines] + [(k, 'screenshot', 's%d' % k) for k in shots]
# a screenshot at an update runs before that update's input; order is by update, inputs first
out.sort(key=lambda t: (t[0], t[1] == 'screenshot'))
for k, cmd, a in out:
    print("%d %s %s" % (k, cmd, a))
