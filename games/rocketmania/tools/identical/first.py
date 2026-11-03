# first.py <case dir> [names]: the first update at which the port's and the original's value of each name differ (the
# original's last value of each update), for the names given (default: all of them).
import os
import re
import sys

def load(path, original):
    seq = {}
    for line in open(path, errors="replace"):
        if original:
            m = re.search(r"watch (\d+): \S+ -> (\S+) at update (\d+)", line)
            if not m:
                continue
            seq.setdefault(int(m.group(1)), []).append((int(m.group(3)), m.group(2)))
        else:
            parts = line.split()
            if len(parts) == 3:
                seq.setdefault(parts[1], []).append((int(parts[0]), parts[2]))
    return seq

def norm(v, name=""):
    if v == "-":
        return v
    v = int(v) & 0xffffffff
    return str(v & 255 if name == "burn" else v)


def limit_of(path):
    # the original's run is cut short at its last update (see cmp.py)
    for line in open(path, errors="replace"):
        m = re.search(r"update counter (\d+);", line)
        if m:
            return int(m.group(1)) - 1
    return 10 ** 9

def series(changes, name, limit):
    out = {}
    for u, v in changes:
        if u <= limit:
            out[u] = norm(v, name)
    return out

def value_at(s, u, keys):
    last = None
    for k in keys:
        if k > u:
            break
        last = s[k]
    return last

d = sys.argv[1]
want = sys.argv[2].split(",") if len(sys.argv) > 2 else None
names = [l.split()[0] for l in open(__file__.replace("first.py", os.environ.get("VALUES", "values.txt"))) if l.strip() and not l.startswith("#")]
port = load(d + "/state.txt", False)
for i in range(0, len(names), 4):
    orig = load("%s/orig%d.log" % (d, i // 4), True)
    limit = limit_of("%s/orig%d.log" % (d, i // 4))
    for j, name in enumerate(names[i:i + 4]):
        if want and name not in want:
            continue
        a = series(port.get(name, []), name, limit)
        b = series(orig.get(j + 1, []), name, limit)
        if name == "burnt":
            # (the original's burn countdown starts as whatever the heap held, see cmp.py)
            a.pop(min(a), None)
            b.pop(min(b), None)
        ka, kb = sorted(a), sorted(b)
        updates = sorted(set(u for u in ka + kb if u > 1))
        for u in updates:
            va, vb = value_at(a, u, ka), value_at(b, u, kb)
            if os.environ.get('SKIPDASH') and ('-' in (va, vb) or None in (va, vb)):
                continue
            if va != vb:
                print("%-8s first differs at %d: port %s original %s" % (name, u, value_at(a, u, ka), value_at(b, u, kb)))
                break
        else:
            print("%-8s same" % name)
