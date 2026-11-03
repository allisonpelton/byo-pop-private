# cmp.py <port state log> <origrun log> <name> [<name> ...]: the original's watched values (origrun's watch k is the k-th
# name) against the port's -statelog (testhooks/StateLog.h), value by value and update by update. Both logs have lines
# "watch <k or name>: old -> new at update N"; the value after each update counts (the last change logged at it), "-"
# (no board yet) is left out at the start, and the comparison runs to the last update both logs reach (for a value that
# hit origrun's change cap, to its last line). Exits 1 if any value differs and prints where, with the values around it.
import re
import sys

PAT = re.compile(r"watch (\S+): (\S+) -> (\S+) at update (\d+)")


def read(path, names=None):
    w, capped, last = {}, set(), 0
    for ln in open(path, encoding="utf-8", errors="replace"):
        if "changes logged; no more" in ln:
            capped.add(re.search(r"watch (\S+)", ln).group(1).rstrip(':'))
        m = PAT.search(ln)
        if not m:
            continue
        k, new, u = m.group(1), m.group(3), int(m.group(4))
        if names is not None:
            k = names[int(k) - 1]
        if k == "praise" and new != '-':
            new = str(int(new) & 0xff)  # the bool alone: the original leaves the padding after it as the heap had it
        u = max(u, 1)  # origrun logs a value's first reading at update 0, the port its first at update 1
        last = max(last, u)
        w.setdefault(k, {})[u] = new
    if names is not None:
        capped = {names[int(k) - 1] for k in capped}
    return w, capped, last


def series(d, lim):
    out = []
    for u in sorted(d):
        if u > lim:
            break
        if out and out[-1][1] == d[u]:
            continue
        out.append((u, d[u]))
    while out and out[0][1] == '-':
        out.pop(0)
    if out and out[-1] == (lim, '-'):
        out.pop()  # the original's board deleted as it quits, at its last update; the port logs no update after that
    return out


port_file, orig_file, names = sys.argv[1], sys.argv[2], sys.argv[3:]
ow, ocap, olast = read(orig_file, names)
pw, _, plast = read(port_file)
end = min(olast, plast)
bad = 0
for n in names:
    lim = end
    if n in ocap and ow.get(n):
        lim = min(lim, max(ow[n]))
    o, p = series(ow.get(n, {}), lim), series(pw.get(n, {}), lim)
    if o == p:
        print("%-10s same: %d changes to update %d%s" % (n, len(o), lim, " (origrun's cap)" if n in ocap else ""))
        continue
    bad += 1
    i = 0
    while i < min(len(o), len(p)) and o[i] == p[i]:
        i += 1
    print("%-10s DIFFERS after %d equal changes: original %s, port %s" % (n, i, o[max(0, i - 2):i + 4], p[max(0, i - 2):i + 4]))
print("compared to update %d (original %d, port %d): %s" % (end, olast, plast, "SAME" if not bad else "%d DIFFER" % bad))
sys.exit(1 if bad else 0)
