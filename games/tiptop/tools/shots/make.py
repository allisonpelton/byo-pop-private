# make.py <tour> <outdir> <port|orig|web>: reads a tour ("<update> <command> [args]" lines of the port's -script, and
# "<update> screenshot <name>") and writes the port's -script (shots to <outdir>/port/<name>.png; the web run's to
# shots/<name>.png in its working directory) or origrun's script ("at <update - 1>", as in phase one: the original reads
# an input posted at counter N in the pass the port numbers N+1), but a shot stays at its update ("at <update> shot
# <name>": the frame read when the counter shows N is the port's screenshot at update N). A line
# "<update> @port <command>" or "@orig" is for one side only.
import sys
tour, out, side = sys.argv[1:4]
lines = []
for l in open(tour):
    l = l.split('#')[0].strip()
    if l:
        k, cmd, *rest = l.split(None, 2)
        if cmd in ('@port', '@orig'):
            if cmd[1:] != ('port' if side == 'web' else side):
                continue
            cmd, *rest = ' '.join(rest).split(None, 1)
        lines.append((int(k), cmd, ' '.join(rest)))
for k, cmd, a in lines:
    if side in ('port', 'web'):
        if cmd == 'screenshot':
            a = ("%s/port/%s.png" % (out, a)) if side == 'port' else ("shots/%s.png" % a)
        if cmd == 'click':
            # origrun's click posts the move with the press, and the original reads the move a pass before the press
            # (its rollover and column sounds play at K-1), so the port's cursor moves an update early too
            print("%d move %s" % (k - 1, a))
        print("%d %s %s" % (k, cmd, a))
    else:
        if cmd == 'quit':
            continue
        if cmd == 'char':
            cmd = 'text'
        if cmd == 'screenshot':
            # a shot held at counter N shows what the port's screenshot at update N shows
            print("at %d shot %s" % (k, a))
        elif cmd == 'focus':
            # measured (tour 3, the pause picture's zoom): origrun's focus change takes effect a pass sooner than a
            # click posted at the same counter, so it goes one update later
            print("at %d %s %s" % (k, cmd, a))
        else:
            print("at %d %s %s" % (k - 1, cmd, a))
if side == 'orig':
    print("wait 200")
    print("quit")
