"""Compares the original's watched values (orig.sh's origrun log) with the port's -statelog, update by update.

    python games/bigmoney/tools/origcmp/statecmp.py <origrun.log> <state.txt> [--names score,state,mti,level]

Both logs have lines "watch k: old -> new at update N". For each watch the value after each update is compared (the
last change logged at an update counts), from the first update either side logs to the last update both reach. It
prints, per watch, the number of changes on each side and either "same" or the first update where they part, with the
values around it. A watch whose original side hit origrun's 4000-line cap is compared up to its last line.
"""
import re
import sys

PAT = re.compile(r"watch (\d+): (\S+) -> (\S+) at update (\d+)")


def read(path):
    w = {}
    capped = set()
    last = 0
    for ln in open(path, encoding="utf-8", errors="replace"):
        if "changes logged; no more" in ln:
            capped.add(int(re.search(r"watch (\d+)", ln).group(1)))
        m = PAT.search(ln)
        if not m:
            continue
        k, new, u = int(m.group(1)), m.group(3), int(m.group(4))
        last = max(last, u)
        d = w.setdefault(k, {})
        d[u] = new  # the last change logged at an update counts
    return w, capped, last


def series(d):
    out = []
    for u in sorted(d):
        if out and out[-1][1] == d[u]:
            continue
        out.append((u, d[u]))
    return [(u, v) for u, v in out if v not in ("?",)]


def main():
    a = sys.argv[1:]
    names = ["score", "state", "mti", "level"]
    if "--names" in a:
        i = a.index("--names")
        names = a[i + 1].split(",")
        del a[i:i + 2]
    ow, ocap, olast = read(a[0])
    pw, _, plast = read(a[1])
    end = min(olast, plast)
    bad = 0
    if not ow:
        sys.exit("statecmp: the original's log has no watch lines")
    for k in sorted(ow):  # the port logs a fifth value, compared only when the original watched it
        name = names[k - 1] if k - 1 < len(names) else str(k)
        o = series(ow.get(k, {}))
        p = series(pw.get(k, {}))
        lim = end
        if k in ocap and o:
            lim = min(lim, o[-1][0])
        o = [t for t in o if t[0] <= lim]
        p = [t for t in p if t[0] <= lim]
        # drop leading "-" (no board yet) on both sides
        while o and o[0][1] == "-":
            o.pop(0)
        while p and p[0][1] == "-":
            p.pop(0)
        if o == p:
            print("%-6s same: %d changes to update %d%s" % (name, len(o), lim, " (origrun's cap)" if k in ocap else ""))
            continue
        bad += 1
        i = 0
        while i < min(len(o), len(p)) and o[i] == p[i]:
            i += 1
        print("%-6s DIFFER after %d equal changes (to update %d): original %s, port %s" % (
            name, i, lim, o[max(0, i - 2):i + 4], p[max(0, i - 2):i + 4]))
    print("compared to update %d (original %d, port %d): %s" % (end, olast, plast, "SAME" if not bad else "%d DIFFER" % bad))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
