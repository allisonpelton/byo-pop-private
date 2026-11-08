# The input of perfmin.py's minute of play, written once for the original (origrun's script: wait and click lines) and once
# for the port (-script: one line per board update, 100 a second). Both click Play at 15 s, then swap a pair of neighbouring
# gems every 1.5 s from 17 s to 75.5 s (40 swaps, walking along the rows of the board), and quit at 90 s.
# The two games deal different boards (the original's seed is its clock's), so a swap makes a match in some and not in
# others; what the two share is the input, the 60 seconds of playing and the drawing of the board.
#   python perfscript.py orig <out.txt>      python perfscript.py port <out.txt>
import sys

kind, out = sys.argv[1], sys.argv[2]
xs = [227 + 51 * k for k in range(7)]
ys = [40 + 51 * j for j in range(8)]
events = [(15.0, 'click 320 398')]   # the title screen's play link
n = 0
for j in range(8):
    for k in range(6):
        if n >= 40:
            break
        t = 17.0 + 1.5 * n
        events.append((t, 'click %d %d' % (xs[k], ys[j])))
        events.append((t + 0.05, 'click %d %d' % (xs[k + 1], ys[j])))
        n += 1
events.append((90.0, 'quit'))
lines = []
if kind == 'orig':
    at = 0.0
    for t, cmd in events:
        if t > at:
            lines.append('wait %d' % round((t - at) * 1000))
            at = t
        lines.append(cmd)
else:
    lines = ['%d %s' % (round(t * 100), cmd) for t, cmd in events]
with open(out, 'w', newline='\n') as f:
    f.write('\n'.join(lines) + '\n')
print(kind, len(lines), 'lines')
