# cmp.py <port state log> <origrun log> <names> <delta> [max update]: the original's watched values (watch k = the k-th
# name, a comma list) against the port's -statelog, field by field and update by update, the port's update numbers
# plus delta. A watch line is "watch k: old -> new at update N"; a state log line is "N name value". The values before
# the first update are left out (the original's memory does not exist yet). Screens are pointers in the original: any
# value but 0 is 1; the bytes the watch reads inside an int are masked. Exits 1 on the first field that differs and
# prints what each side has there.
import re
import sys

port_file, orig_file, names = sys.argv[1], sys.argv[2], sys.argv[3].split(',')
delta = int(sys.argv[4])
max_update = int(sys.argv[5]) if len(sys.argv) > 5 else 10 ** 9
SCREENS = ('load', 'menu', 'story', 'island', 'temple', 'board', 'completed', 'help')
BYTES = ('done', 'moving')

port = {n: [] for n in names}
for line in open(port_file):
    u, n, v = line.split()
    if int(u) <= 1:
        continue
    u = int(u) + delta
    if n in port and u <= max_update:
        port[n].append((u, v))

orig = {n: [] for n in names}
for line in open(orig_file, errors='replace'):
    m = re.search(r'watch (\d): (\S+) -> (\S+) at update (\d+)', line)
    if not m:
        continue
    k = int(m.group(1)) - 1
    u = int(m.group(4))
    if k >= len(names) or not 1 < u <= max_update:
        continue
    n = names[k]
    v = m.group(3)
    if v != '-':
        if n in SCREENS:
            v = '1' if v != '0' else '0'
        elif n in BYTES:
            v = str(int(v) & 255)
    if orig[n] and orig[n][-1][0] == u:   # collapse: the last change inside an update is the value the port logs
        orig[n].pop()
    if not (orig[n] and orig[n][-1][1] == v):
        orig[n].append((u, v))

bad = 0
for n in names:
    p, o = port[n], orig[n]
    i = 0
    while i < min(len(p), len(o)) and p[i] == o[i]:
        i += 1
    same = i == len(p) == len(o)
    print("%-10s port %4d  original %4d  %s" % (n, len(p), len(o), "same" if same else "DIFFERS at change %d" % i))
    if not same:
        bad += 1
        print("   port:    ", p[max(0, i - 1):i + 4])
        print("   original:", o[max(0, i - 1):i + 4])
sys.exit(1 if bad else 0)
