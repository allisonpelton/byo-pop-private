# cmp.py <port state log> <origrun log> <names> <last frame> [first frame]: the original's watched values (watch k = the k-th
# name, a comma list) against the port's -statelog, frame by frame. A watch line is "watch k: old -> new at update N"; a state
# log line is "N name value". Each side's value at a frame is its last change at or before it, so a value the original sets
# again to what it was (a watch line it logs, the port does not) is no difference. The frames compared are from the first
# frame (default 2) to the last. The original leaves most of the game's values unset until a game starts (all but those in
# SET): they are compared from the first frame, at or after the port's first level starts (MODE_FROM, from case.sh), where
# both sides have the same value. Exits 1 when a value differs and prints the
# first frame where it does, with what each side has around it.
import os
import re
import sys

BYTES = ('ball0letter',)
SET = ('rand', 'state', 'nextstate')


def norm(v):
    # origrun prints a watch as unsigned, the port's log as signed: both as a signed 32-bit int
    try:
        n = int(v) & 0xffffffff
    except ValueError:
        return v
    return str(n - (1 << 32) if n >= 1 << 31 else n)


port_file, orig_file, names = sys.argv[1], sys.argv[2], sys.argv[3].split(',')
last = int(sys.argv[4])
first = int(sys.argv[5]) if len(sys.argv) > 5 else 2
mode_from = int(os.environ.get('MODE_FROM', first))

port = {n: [] for n in names}
for line in open(port_file):
    u, n, v = line.split()
    if n in port:
        port[n].append((int(u), norm(v)))

orig = {n: [] for n in names}
for line in open(orig_file, errors='replace'):
    m = re.search(r'watch (\d): (\S+) -> (\S+) at update (\d+)', line)
    if not m:
        continue
    k = int(m.group(1)) - 1
    if k >= len(names):
        continue
    old, v, u = norm(m.group(2)), norm(m.group(3)), int(m.group(4))
    if names[k] in BYTES:
        old, v = [str((int(x) & 255) - (256 if int(x) & 128 else 0)) if x.lstrip('-').isdigit() else x for x in (old, v)]
    if not orig[names[k]]:
        orig[names[k]].append((0, old))
    orig[names[k]].append((u, v))


def at(changes, f):
    v = None
    for u, x in changes:
        if u > f:
            break
        v = x
    return v


bad = 0
for n in names:
    p, o = port[n], orig[n]
    start = first
    if n not in SET:
        start = max(first, mode_from)
        while start <= last and at(p, start) != at(o, start):
            start += 1
    diff = None
    for f in range(start, last + 1):
        if at(p, f) != at(o, f):
            diff = f
            break
    np = sum(1 for u, _ in p if start <= u <= last)
    no = sum(1 for u, _ in o if start <= u <= last)
    print("%-11s port %4d  original %4d  %s" % (n, np, no, "same" if diff is None else "DIFFERS at frame %d" % diff))
    if diff is not None:
        bad += 1
        print("   port:    ", [c for c in p if diff - 3 <= c[0] <= diff + 3][:6], "at:", at(p, diff))
        print("   original:", [c for c in o if diff - 3 <= c[0] <= diff + 3][:6], "at:", at(o, diff))
sys.exit(1 if bad else 0)
