# cmp.py <origrun out> <runner out>: compares each watch's (value, update) changes after the first value of each
import sys, re
def load(p, pat):
    w = {}
    for l in open(p, errors='replace'):
        m = re.search(pat, l)
        if m: w.setdefault(int(m.group(1)), []).append((int(m.group(2)), int(m.group(3))))
    return w
o = load(sys.argv[1], r'watch (\d): \S+ -> (-?\d+) at update (-?\d+)')
p = load(sys.argv[2], r'^watch (\d): (\d+) at update (-?\d+)')
names = {1: 'seed', 2: 'score', 3: 'lives', 4: 'filled'}
for i in range(1, 5):
    a = [x for x in o.get(i, []) if x[1] > 1]; b = [x for x in p.get(i, []) if x[1] > 1]
    if i == 3 or i == 2: a = [((x[0] & 0xffffffff) if x[0] < 0 else x[0], x[1]) for x in a]
    n = min(len(a), len(b)); d = next((k for k in range(n) if a[k] != b[k]), None)
    print(f"{names[i]}: original {len(a)}, port {len(b)}, " + ("same over %d" % n if d is None else f"first difference at #{d}: original {a[max(0,d-1):d+2]} port {b[max(0,d-1):d+2]}"))
