# The input of one measured run, written once for the port and once for the original from the same list of events: the
# script that tools/check.sh plays (a new player, escape stage 1 of Ancient Japan or stage 1 of Ancient China won with the
# solver's clicks, the map). The port's -script lines count updates of 5 ms (mFrameTime); the original's origrun script is
# wait and click lines in real time. Each click of the port's is a move, a pause of 150 ms and a click in the original, so
# that the hover is seen first, the way adv-1 (tools/coverage/mkscript.py) does it; the clicks fall at the same times in
# both. The original's first click is 2 s later than the port's: the port counts from the first frame of its loop (about
# 1.3 s into its process) and the original's script from the start of its process, whose menu is up at about 8 s, so a
# window in process seconds is the same stretch of play in both to within about a second.
#
#   python perfscript.py <check script.txt> port <out.txt> [offset s]    the port's script without the screenshot lines
#   python perfscript.py <check script.txt> orig <out.txt> [offset s]    origrun's script, the first click offset s later
import sys

src, kind, out = sys.argv[1], sys.argv[2], sys.argv[3]
offset = float(sys.argv[4]) if len(sys.argv) > 4 else 2.0
UPDATE_MS = 5
events = []
for line in open(src).read().splitlines():
    parts = line.split()
    if len(parts) < 2 or parts[1] == 'screenshot':
        continue
    events.append((int(parts[0]) * UPDATE_MS, parts[1:]))
if kind == 'port':
    lines = ['%d %s' % (t // UPDATE_MS, ' '.join(a)) for t, a in events]
else:
    lines = []
    at = 0
    base = int(offset * 1000)
    VK = {'RETURN': 13}
    for t, a in events:
        t += base
        if a[0] == 'click':
            ts = max(t - 150, at)
            if ts > at:
                lines.append('wait %d' % (ts - at))
                at = ts
            lines.append('move %s %s' % (a[1], a[2]))
            lines.append('wait %d' % (t - at))
            at = t
            lines.append('click %s %s' % (a[1], a[2]))
        else:
            if t > at:
                lines.append('wait %d' % (t - at))
                at = t
            if a[0] == 'char':
                lines.append('text %s' % a[1])
            elif a[0] == 'key':
                lines.append('key %d' % VK[a[1]])
            elif a[0] == 'quit':
                lines.append('quit')
with open(out, 'w', newline='\n') as f:
    f.write('\n'.join(lines) + '\n')
print(kind, len(lines), 'lines')
