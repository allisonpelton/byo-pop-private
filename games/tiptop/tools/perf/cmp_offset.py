#!/usr/bin/env python
"""cmp_offset.py <state log A> <state log B>: do two state logs (-statelog) hold the same play, apart from a constant shift of the
update numbers? The loading change (GAME_NOTES.md, "Deviations from the original") makes the title screen end earlier, so every
later update comes that many updates sooner; with the blocks decided by the seed alone (the fixes mod's seededBlocks) the games
are the same. The shift is read from the menu's first appearance (screen 2); the values of the title phase are compared as they
stand when the menu appears; the meter (uninitialised heap outside the timed mode) is left out.

Prints one line: "identical (shift N, M changes)" or the first differences, and exits 1 on a difference.
"""
import sys


def read(path):
    rows = []
    for line in open(path, encoding="utf-8", errors="replace"):
        parts = line.rstrip("\r\n").split(" ", 2)
        if len(parts) == 3 and parts[0].isdigit() and parts[1] != "meter":
            rows.append((int(parts[0]), parts[1], parts[2]))
    return rows


def menu_update(rows):
    for (u, n, v) in rows:
        if n == "screen" and v == "2":
            return u
    return None


def main():
    a, b = read(sys.argv[1]), read(sys.argv[2])
    ma, mb = menu_update(a), menu_update(b)
    if ma is None or mb is None or not a or not b:
        print("no menu in a log (%s, %s)" % (ma, mb))
        return 1
    shift = mb - ma

    def title_state(rows, m):
        st = {}
        for (u, n, v) in rows:
            if u < m:
                st[n] = v
        return st

    ta, tb = title_state(a, ma), title_state(b, mb)
    diffs = [("title " + n, ta.get(n), tb.get(n)) for n in sorted(set(ta) | set(tb)) if ta.get(n) != tb.get(n)]
    ra = [(u + shift, n, v) for (u, n, v) in a if u >= ma]
    rb = [(u, n, v) for (u, n, v) in b if u >= mb]
    if len(ra) != len(rb):
        diffs.append(("changes after the menu", len(ra), len(rb)))
    for i in range(min(len(ra), len(rb))):
        if ra[i] != rb[i]:
            diffs.append(("change %d" % i, ra[i], rb[i]))
            if len(diffs) > 6:
                break
    if diffs:
        for d in diffs:
            print("DIFFERS: %s: %s | %s" % d)
        return 1
    print("identical (shift %d, %d changes after the menu)" % (shift, len(ra)))
    return 0


sys.exit(main())
