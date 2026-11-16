# cmp.py <port state log> <origrun log> <names> <last frame> [first frame]: the original's watched values (watch k = the
# k-th name, a comma list) against the port's -statelog, frame by frame. A watch line is "watch k: old -> new at update
# N"; a state log line is "N name value". Each side's value at a frame is its last change at or before it, so a value
# the original sets again to what it was (a watch line it logs, the port does not) is no difference. The frames compared
# are from the first frame (default 2) to the last. The game's values are unset until a game starts (all but those in
# SET): they are compared from the first frame, at or after the port's first level starts (MODE_FROM, from case.sh),
# where both sides have the same value. The values in CHANGES (the current state, a class name in the port's log and a
# vtable in the original's) are compared by the frames they change at. Exits 1 when a value differs and prints the first
# frame where it does, with what each side has around it. (From Word Harmony's cmp.py.)
import os
import re
import sys

BYTES = ()
SET = ('rand', 'state')
CHANGES = ('state',)


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
        old, v = [str((int(x) & 255) - (256 if int(x) & 128 else 0)) if x.lstrip('-').isdigit() else x
                  for x in (old, v)]
    if not orig[names[k]]:
        orig[names[k]].append((0, old))
    orig[names[k]].append((u, v))


def frames_only(changes):
    # each frame with a change numbered in turn, so that two logs compare by when their values change; the watch's
    # start ("?" and "-" before the game sets its first value) is left out, and changes within one frame count once
    frames = []
    for u, v in changes:
        if v in ('?', '-'):
            continue
        if not frames or frames[-1] != u:
            frames.append(u)
    return [(u, str(i)) for i, u in enumerate(frames)]


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
    if n in CHANGES:
        p, o = frames_only(p), frames_only(o)
    start = first
    if n not in SET:
        start = max(first, mode_from)
        while start <= last and at(p, start) != at(o, start):
            start += 1
    # a game value the port stops logging ("-": its mode is gone) is compared no further, as the original's memory keeps
    # the old object's
    end = last
    if n not in SET:
        gone = [u for u, v in p if v == '-' and u >= start]
        if gone:
            end = gone[0] - 1
    diff = None
    for f in range(start, end + 1):
        if at(p, f) != at(o, f):
            diff = f
            break
    np = sum(1 for u, _ in p if start <= u <= end)
    no = sum(1 for u, _ in o if start <= u <= end)
    print("%-11s port %4d  original %4d  %s" % (n, np, no, "same" if diff is None else "DIFFERS at frame %d" % diff))
    if diff is not None:
        bad += 1
        print("   port:    ", [c for c in p if diff - 3 <= c[0] <= diff + 3][:6], "at:", at(p, diff))
        print("   original:", [c for c in o if diff - 3 <= c[0] <= diff + 3][:6], "at:", at(o, diff))
sys.exit(1 if bad else 0)
