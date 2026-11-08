# make.py <tour> <outdir> <port|orig|web>: reads a tour ("<update> <command>", "<update> screenshot <name>") and writes the port's
# -script (screenshots to <outdir>/port/<name>.png) or origrun's script (at <update-1> ...; shots by name). The original reads an
# input posted "at N" before update N+1; the port runs line K before update K.
import sys
tour, out, side = sys.argv[1:4]
lines = []
for l in open(tour):
    l = l.split('#')[0].strip()
    if l:
        k, cmd, *rest = l.split(None, 2)
        lines.append((int(k), cmd, ' '.join(rest)))
if side in ('port', 'web'):
    for k, cmd, a in lines:
        if cmd == 'screenshot':
            a = ("%s/port/%s.png" % (out, a)) if side == 'port' else ("shots/%s.png" % a)
        print("%d %s %s" % (k, cmd, a))
else:
    if side == 'host':
        print("at 200 focus regain")  # an original that never got the focus plays no sound
    else:
        # the original's loading takes real time (its threads' waits are not skipped), so inside the sandbox the counter crawls at first;
        # each of these waits gives up after 30 s, and the first real command comes after the title is there
        for k in (1000, 2000, 3000, 4000):
            print("at %d move 5 5" % k)
    for k, cmd, a in lines:
        if cmd == 'quit':
            continue
        if cmd == 'screenshot':
            if side == 'host':  # no shots on the host (origrun takes none there)
                continue
            cmd = 'shot'
        if cmd == 'char':
            cmd = 'text'
        print("at %d %s %s" % (k - 1, cmd, a))
    print("quit")
