# make.py <tour> <outdir> <port|orig>: reads a tour ("<update> <command>", "<update> screenshot <name>") and writes the port's
# -script (screenshots to <outdir>/port/<name>.png) or origrun's script (at <update-1> ...; shots by name; the "Updates"
# question answered first). The original reads an input posted "at N" before update N+1; the port runs line K before update K.
import sys
tour, out, side = sys.argv[1:4]
lines = []
for l in open(tour):
    l = l.split('#')[0].strip()
    if l:
        k, cmd, *rest = l.split(None, 2)
        if cmd in ('@port', '@orig'):  # a line for one side only (the options dialog is taller in the original)
            if cmd[1:] != ('orig' if side == 'host' else side):
                continue
            cmd, *rest = ' '.join(rest).split(None, 1)
        lines.append((int(k), cmd, ' '.join(rest)))
if side in ('port', 'web'):
    for k, cmd, a in lines:
        if cmd == 'screenshot':
            a = ("%s/port/%s.png" % (out, a)) if side == 'port' else ("shots/%s.png" % a)  # the web run's folder is its working directory
        elif cmd == 'text':
            cmd, a = 'char', a
        print("%d %s %s" % (k, cmd, a))
else:
    host = side == 'host'  # no shots on the host (origrun takes none there)
    if host:
        print("at 200 focus regain")
    print("at 300 move 390 244")
    print("at 310 click 390 244")
    for k, cmd, a in lines:
        if cmd == 'quit':
            continue
        if cmd == 'screenshot':
            if host:
                continue
            cmd = 'shot'
        if cmd == 'char':
            cmd = 'text'
        print("at %d %s %s" % (k - 1, cmd, a))
