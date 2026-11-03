# window.py <case dir> <first update> <last update> [names]: every change of every value (or of the listed ones) in that
# range, the port's and the original's side by side, update by update (the original's last value of each update).
import re
import sys

d, lo, hi = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
want = sys.argv[4].split(',') if len(sys.argv) > 4 else None
names = [l.split()[0] for l in open(__file__.replace('window.py', __import__('os').environ.get('VALUES','values.txt'))) if l.strip() and not l.startswith('#')]
port, orig = {}, {}
for line in open(d + '/state.txt'):
    u, n, v = line.split()
    port.setdefault(n, {})[int(u)] = v
for k in range((len(names) + 3) // 4):
    try:
        lines = open('%s/orig%d.log' % (d, k), errors='replace').read().split('\n')
    except OSError:
        continue
    for line in lines:
        m = re.search(r'watch (\d): (\S+) -> (\S+) at update (\d+)', line)
        if m:
            i = k * 4 + int(m.group(1)) - 1
            if i < len(names):
                orig.setdefault(names[i], {})[int(m.group(4))] = m.group(3)
rows = []
for n in names:
    if want and n not in want:
        continue
    for u in range(lo, hi + 1):
        a, b = port.get(n, {}).get(u), orig.get(n, {}).get(u)
        if a is not None or b is not None:
            rows.append((u, n, a, b))
for u, n, a, b in sorted(rows):
    print('%d %-8s port %-12s orig %-12s %s' % (u, n, a, b, '' if a == b else '<<<'))
