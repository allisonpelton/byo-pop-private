# make.py <tour> <outdir> <port|web|orig|host>: reads a tour ("<update> <command>", "<update> screenshot <name>") and
# writes the port's -script (screenshots to <outdir>/port/<name>.png, or shots/<name>.png for a web run under Node) or
# origrun's script (a wait for the loading, mark, then "at +<update-1-base> ...", shots by name). A port line K runs before update K's widgets are updated; the
# original reads an input posted "at N" before update N+1. "@port"/"@orig" after the update makes a line one side's
# only. A run of "text" goes to the port as "char" lines; on the host (no shots) the window is made active first, as
# the original plays its samples only then.
import sys
tour, out, side = sys.argv[1:4]
lines = []
for l in open(tour):
    l = l.split('#')[0].strip()
    if not l:
        continue
    k, cmd, *rest = l.split(None, 2)
    if cmd in ('@port', '@orig'):
        if cmd[1:] != ('orig' if side == 'host' else ('port' if side == 'web' else side)):
            continue
        cmd, *rest = ' '.join(rest).split(None, 1)
    lines.append((int(k), cmd, ' '.join(rest)))
lines.sort(key=lambda t: t[0])  # origrun waits for each line's update in file order
if side in ('port', 'web'):
    for k, cmd, a in lines:
        if cmd == 'screenshot':
            a = ("%s/port/%s.png" % (out, a)) if side == 'port' else ("shots/%s.png" % a)
            print("%d screenshot %s" % (k, a))
        elif cmd == 'text':
            for c in a:
                print("%d char %s" % (k, 'SPACE' if c == ' ' else c))
        else:
            print("%d %s %s" % (k, cmd, a))
else:
    host = side == 'host'
    # The original unpacks its sounds on a thread at the machine's speed while the virtual clock runs its updates
    # back to back, so its title can take thousands of updates to load (more on a busy machine). The script waits
    # 30 s of real time, marks the update count, and places every line from there, the tour's first line 10
    # updates after the mark: both titles are loaded and at rest by then.
    base = lines[0][0] - 10 if lines else 0
    print("wait %d" % int(__import__('os').environ.get('ORIG_LOAD_MS', '60000')))
    print("mark")
    if host:
        print("at +5 focus regain")
    for k, cmd, a in lines:
        if cmd == 'screenshot':
            if host:
                continue
            cmd = 'shot'
        if cmd == 'quit':
            print("at +%d move 0 0" % (k - 1 - base))
            print("quit")
            break
        if cmd == 'key' and not a.isdigit():
            a = str({'RETURN': 13, 'ESCAPE': 27, 'SPACE': 32, 'BACK': 8}[a])
        print("at +%d %s %s" % (k - 1 - base, cmd, a))
