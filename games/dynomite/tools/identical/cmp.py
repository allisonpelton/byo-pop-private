# cmp.py <port state log> <origrun log> <names> [max update]: the original's watched values (watch k = the k-th name, a
# comma list) against the port's -statelog, field by field and update by update. A watch line is "watch k: old -> new at
# update N" (the last one of an update counts); a state log line is "N name value". Only updates from 2 to max update are compared (the last input's update:
# origrun quits after it). A name "-" is not compared. Exits 1 if any field differs and prints what each side has there.
import re
import sys

BYTES = ('frozen', 'fell')  # a bool the watch reads inside an int


def norm(v):
    # both as unsigned 32-bit numbers (the watch prints them unsigned, the port's log signed)
    return v if v == '-' else str(int(v) & 0xffffffff)

port_file, orig_file, names = sys.argv[1], sys.argv[2], sys.argv[3].split(',')
max_update = int(sys.argv[4]) if len(sys.argv) > 4 else 10 ** 9

port = {n: [] for n in names}
for line in open(port_file):
    u, n, v = line.split()
    if n in port and 1 < int(u) <= max_update:
        port[n].append((int(u), norm(v)))

orig = {n: [] for n in names}
for line in open(orig_file, errors='replace'):
    m = re.search(r'watch (\d): (\S+) -> (\S+) at update (\d+)', line)
    if not m:
        continue
    k = int(m.group(1)) - 1
    u = int(m.group(4))
    if k >= len(names) or not 1 < u <= max_update:
        continue
    v = norm(m.group(3))
    if names[k] in BYTES and v != '-':
        v = str(int(v) & 255)
    # the watch may see a value in the middle of an update; the port logs it at the update's end
    if orig[names[k]] and orig[names[k]][-1][0] == u:
        orig[names[k]].pop()
    if not (orig[names[k]] and orig[names[k]][-1][1] == v):
        orig[names[k]].append((u, v))

bad = 0
for n in names:
    if n == '-':
        continue
    p, o = port[n], orig[n]
    i = 0
    while i < min(len(p), len(o)) and p[i] == o[i]:
        i += 1
    same = i == len(p) == len(o)
    print("%-10s port %5d  original %5d  %s" % (n, len(p), len(o), "same" if same else "DIFFERS at change %d" % i))
    if not same:
        bad += 1
        print("   port:    ", p[max(0, i - 1):i + 4])
        print("   original:", o[max(0, i - 1):i + 4])
sys.exit(1 if bad else 0)
