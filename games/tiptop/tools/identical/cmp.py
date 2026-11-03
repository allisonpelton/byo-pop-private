# cmp.py <port state log> <origrun log> <names> <shift> [max update] [from]: the original's watched values (watch k = the k-th
# name, a comma list) against the port's -statelog, field by field and update by update. A watch line is "watch k: old ->
# new at update N"; a state log line is "N name value". The original's update N is the port's N - shift. The values
# before the port's update 2 are left out (the original's watches start when its window is up). The board's values are
# compared from update <from> (the first game's start), each side's value then first: before a game the board holds
# leftovers (the original's uninitialised heap, the port's zeroes) that the new game overwrites. Floats are compared by
# their bits. A watch that reached origrun's 4000 changes is compared up to its last. Exits 1 if a field differs and prints what each side has there.
import re
import sys

port_file, orig_file, names, shift = sys.argv[1], sys.argv[2], sys.argv[3].split(','), int(sys.argv[4])
max_update = int(sys.argv[5]) if len(sys.argv) > 5 else 10 ** 9
start = int(sys.argv[6]) if len(sys.argv) > 6 else 0
GLOBAL = ('screen', 'mode', 'rng', 'draws', 'paused', 'bonus', 'busy')
# the mode's high score table: its changes after the first game's start (the port logs the table it reads when the mode is
# chosen; origrun logs a watch's changes only)
HIGH = ('hs0', 'hs1', 'hs2', 'hs3', 'hsname')
# board values the original leaves uninitialised until the game first sets them: the original's value at the game's
# start is taken as the port's
UNSET = ('spawndelay', 'queueindex', 'shifting')


def norm(v):
    if v == '-':
        return v
    return str(int(v) & 0xffffffff)


port = {n: [] for n in names}
for line in open(port_file):
    u, n, v = line.split()
    if n in port and int(u) <= max_update:
        v = norm(v)
        if not (port[n] and port[n][-1][1] == v):
            port[n].append((int(u), v))

orig = {n: [] for n in names}
for line in open(orig_file, errors='replace'):
    m = re.search(r'watch (\d): (\S+) -> (\S+) at update (\d+)', line)
    if not m:
        continue
    k = int(m.group(1)) - 1
    u = int(m.group(4)) - shift
    if k >= len(names) or u > max_update:
        continue
    v = norm(m.group(3))
    if names[k] == 'paused' and v != '-':
        v = str(int(v) & 0xffffff)  # three bytes; the watch reads four
    if names[k] in ('bonus', 'busy') and v != '-':
        v = str(int(v) & 0xff)  # a byte
    if not (orig[names[k]] and orig[names[k]][-1][1] == v):
        orig[names[k]].append((u, v))



def from_start(changes):
    # the value at update start, then the changes after it
    before = [c for c in changes if c[0] <= start]
    after = [c for c in changes if c[0] > start]
    out = [(start, before[-1][1])] if before else []
    for c in after:
        if not (out and out[-1][1] == c[1]):
            out.append(c)
    return out


def collapse(changes):
    out = []
    for c in changes:
        if not (out and out[-1][1] == c[1]):
            out.append(c)
    return out


bad = 0
for n in names:
    p, o = port[n], orig[n]
    if n in HIGH:
        p, o = [c for c in p if c[0] > start], [c for c in o if c[0] > start]
    elif start and n not in GLOBAL:
        p, o = from_start(p), from_start(o)
        if n in UNSET and p and o:
            # the original's leftover is taken as the port's value then, so a later write of the port's value is no change
            o = collapse([(o[0][0], p[0][1])] + o[1:])
    else:
        p, o = [c for c in p if c[0] > 1], [c for c in o if c[0] > 1]
    if len(o) >= 3990:
        # origrun logs at most 4000 changes of a watch: compare up to the last it logged
        p = [x for x in p if x[0] <= o[-1][0]]
    i = 0
    while i < min(len(p), len(o)) and p[i] == o[i]:
        i += 1
    same = i == len(p) == len(o)
    print("%-13s port %5d  original %5d  %s" % (n, len(p), len(o), "same" if same else "DIFFERS at change %d" % i))
    if not same:
        bad += 1
        print("   port:    ", p[max(0, i - 1):i + 4])
        print("   original:", o[max(0, i - 1):i + 4])
sys.exit(1 if bad else 0)
