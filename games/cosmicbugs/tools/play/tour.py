# tour.py <tour.txt> <name>: a tour of timed events (runner time in ms) as a runner script (build/cbw/<name>.txt) and an
# origrun script (build/origrun/<name>.txt). Lines: "T click x y", "T rclick", "T move x y", "T key vk", "T shot name",
# "T end" ('#' starts a comment). The runner's frame ending at T ms has run the updates up to T-9, which origrun's
# "at T-9" holds, so a shot at T is "at T-9 shot" and a move "at T-9 move"; a click at T (its move at T-10) is the
# original's "at T-19 click x y 30", read by its message pump at the start of the frame, and a key the same, held 30
# updates. A right click does not move the cursor: origrun's rclick is sent at the last click's or move's place.
import sys

run, orig = [], []
x, y = 400, 300
end = None
for line in open(sys.argv[1]):
    f = line.split('#')[0].split()
    if not f:
        continue
    t, what, args = int(f[0]), f[1], f[2:]
    if what == 'click':
        x, y = int(args[0]), int(args[1])
        run += [(t - 10, "move %d %d" % (x, y)), (t, "click %d %d" % (x, y))]
        orig.append((t - 19, "click %d %d 30" % (x, y)))
    elif what == 'move':
        x, y = int(args[0]), int(args[1])
        run.append((t, "move %d %d" % (x, y)))
        orig.append((t - 9, "move %d %d" % (x, y)))
    elif what == 'rclick':
        run += [(t, "down 2"), (t + 30, "up 2")]
        orig.append((t - 19, "rclick %d %d 30" % (x, y)))
    elif what == 'key':
        run += [(t, "hold %s" % args[0]), (t + 30, "release %s" % args[0])]
        orig.append((t - 19, "key %s 30" % args[0]))
    elif what == 'shot':
        run.append((t, "shot %s" % args[0]))
        orig.append((t - 9, "shot %s" % args[0]))
    elif what == 'end':
        end = t
    else:
        sys.exit("unknown event: " + line)
run.sort(key=lambda e: e[0])
orig.sort(key=lambda e: e[0])
name = sys.argv[2]
with open('build/cbw/%s.txt' % name, 'w', newline='\n') as o:
    o.write("".join("%d %s\n" % e for e in run) + "%d end\n" % end)
with open('build/origrun/%s.txt' % name, 'w', newline='\n') as o:
    o.write("".join("at %d %s\n" % e for e in orig) + "at %d move 0 0\nquit\n" % (end - 9))
