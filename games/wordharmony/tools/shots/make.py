# make.py <tour> <outdir> <port|orig|web>: reads a tour ("<update> <command> [args]" lines of the port's -script, and
# "<update> screenshot <name>") and writes the port's -script (shots to <outdir>/port/<name>.png; the web run's to
# shots/<name>.png in its working directory) or origrun's script: an input at "at <update - 1>", as in phase one (origrun
# holds the original after frame N, so an input posted there is read before frame N+1, where the port reads its line K
# before frame K), and a shot at "at <update - 1> shot <name>" too: the original held after frame N-1 shows what the
# port's screenshot at line N shows (measured on TSUNAMI's falling balls: the port's shots at 754-766 against the
# original's "at 760", only the port's 761 matched). A line "<update> @port
# <command>" or "@orig" is for one side only.
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
lines.sort(key=lambda l: l[0])
for k, cmd, a in lines:
    if side in ('port', 'web'):
        if cmd == 'screenshot':
            a = ("%s/port/%s.png" % (out, a)) if side == 'port' else ("shots/%s.png" % a)
        print("%d %s %s" % (k, cmd, a))
    else:
        if cmd == 'quit':
            print("at %d move 0 0" % (k - 1))
            continue
        if cmd == 'char':
            cmd = 'text'
        if cmd == 'screenshot':
            print("at %d shot %s" % (k - 1, a))
        else:
            print("at %d %s %s" % (k - 1, cmd, a))
if side == 'orig':
    print("quit")
