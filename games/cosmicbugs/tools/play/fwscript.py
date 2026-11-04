# fwscript.py <runner script> <shot folder> <out>: a runner script (games/cosmicbugs/test/gamerun.cpp) as a -script
# file for the exe and the web build (framework/SexyAppBase.cpp's RunScript, with the app's rdown, rup and state). The
# runner's event at T ms is handled before its frame ending at the first multiple of 10 from T, its frame
# ceil(T/10); the exe runs a command at update N after that update's frame, so the event is the exe's update
# ceil(T/10) - 1 (a shot then shows the same frame as the runner's). Its click and key hold 30 ms (3 updates);
# "down 2"/"up 2" press the right button where the pointer is (moved by the game's warps too); a shot is a screenshot
# into <shot folder>; "end" quits.
import sys

x, y = 400, 300
out = []
for line in open(sys.argv[1]):
    f = line.split('#')[0].split()
    if not f:
        continue
    t, what, args = int(f[0]), f[1], f[2:]
    u = (t + 9) // 10 - 1
    if what == 'move':
        x, y = int(args[0]), int(args[1])
        out.append((u, "move %d %d" % (x, y)))
    elif what == 'click':
        x, y = int(args[0]), int(args[1])
        out += [(u, "down %d %d" % (x, y)), (u + 3, "up %d %d" % (x, y))]
    elif what in ('down', 'up'):
        if args and args[0] == '2':
            out.append((u, "r" + what))
        else:
            out.append((u, "%s %d %d" % (what, x, y)))
    elif what in ('hold', 'release'):
        out.append((u, "%s %s" % ("keydown" if what == 'hold' else "keyup", args[0])))
    elif what == 'key':
        out += [(u, "keydown %s" % args[0]), (u + 3, "keyup %s" % args[0])]
    elif what == 'shot':
        out.append((u, "screenshot %s/%s.png" % (sys.argv[2], args[0])))
    elif what == 'focus':
        out.append((u, "focus %s" % args[0]))
    elif what == 'state':
        out.append((u, "state"))
    elif what == 'end':
        out.append((u, "quit"))
out.sort(key=lambda e: e[0])
with open(sys.argv[3], 'w', newline='\n') as f:
    for u, c in out:
        f.write("%d %s\n" % (u, c))
