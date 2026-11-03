# cmp.py <port state log> <origrun log> <names> [max update]: the original's watched values (watch k = the k-th name, a
# comma list) against the port's -statelog, field by field and update by update. A watch line is "watch k: old -> new at
# update N"; a state log line is "N name value". The values before the first update are left out (the original's board
# does not exist yet, the port logs "-" at update 1). Exits 1 on the first field that differs and prints what each side
# has there.
import re
import sys

port_file, orig_file, names = sys.argv[1], sys.argv[2], sys.argv[3].split(',')
max_update = int(sys.argv[4]) if len(sys.argv) > 4 else 10 ** 9
# the original's run ends with "quit" right after the last input, so its last update is cut short (a change that input makes
# between updates N and N+1 is logged at N+1, which never comes): the last update is left out of the comparison
for line in open(orig_file, errors='replace'):
    m = re.search(r'update counter (\d+);', line)
    if m:
        max_update = min(max_update, int(m.group(1)) - 1)
# origrun logs at most 4000 changes of a watch ("watch k: 4000 changes logged; no more"): a value is compared only up to the
# last change that was logged
watch_end = {}
last_logged = {}
for line in open(orig_file, errors='replace'):
    m = re.search(r'watch (\d): (\S+) -> (\S+) at update (\d+)', line)
    if m:
        last_logged[int(m.group(1)) - 1] = int(m.group(4))
    m = re.search(r'watch (\d): \d+ changes logged; no more', line)
    if m and int(m.group(1)) - 1 < len(names):
        watch_end[names[int(m.group(1)) - 1]] = last_logged.get(int(m.group(1)) - 1, 0)
BOOLS = ('burn',)


def norm(name, v):
    if v == '-':
        return v
    v = int(v) & 0xffffffff
    if name in BOOLS:
        v &= 255
    return str(v)


port = {n: [] for n in names}
for line in open(port_file):
    u, n, v = line.split()
    if n in port and 1 < int(u) <= min(max_update, watch_end.get(n, 10 ** 9)):
        v = norm(n, v)
        if not (port[n] and port[n][-1][1] == v):
            port[n].append((int(u), v))

orig = {n: [] for n in names}
for line in open(orig_file, errors='replace'):
    m = re.search(r'watch (\d): (\S+) -> (\S+) at update (\d+)', line)
    if not m:
        continue
    k = int(m.group(1)) - 1
    u = int(m.group(4))
    if k >= len(names) or not 1 < u <= min(max_update, watch_end.get(names[k], 10 ** 9)):
        continue
    v = norm(names[k], m.group(3))
    if orig[names[k]] and orig[names[k]][-1][0] == u:
        orig[names[k]].pop()    # the state log has each update's last value only
    if not (orig[names[k]] and orig[names[k]][-1][1] == v):
        orig[names[k]].append((u, v))

# the original's board does not set its burn countdown before the first launch (it keeps what the heap held)
for d in (port, orig):
    if 'burnt' in d and d['burnt']:
        d['burnt'].pop(0)

bad = 0
for n in names:
    p, o = port[n], orig[n]
    i = 0
    while i < min(len(p), len(o)) and p[i] == o[i]:
        i += 1
    same = i == len(p) == len(o)
    print("%-8s port %5d  original %5d  %s" % (n, len(p), len(o), "same" if same else "DIFFERS at change %d" % i))
    if not same:
        bad += 1
        print("   port:    ", p[max(0, i - 1):i + 4])
        print("   original:", o[max(0, i - 1):i + 4])
sys.exit(1 if bad else 0)
