#!/usr/bin/env python3
"""Carries the port's GT(0x...) text addresses from Bookworm Adventures Deluxe's exe (the addresses the copied sources
use) to Volume 2's: each text is read at its Volume 1 address and looked up as a whole C string in Volume 2's exe.
Prints only counts and addresses, never a text (the sources hold addresses, GameText.h reads the texts from the exe).

    python games/bookwormadv2/tools/gtmap.py [--v1 assets/bookwormadv/bookwormadventures.exe] [--list] [--apply]

--list prints address -> [Volume 2 addresses] per text; --apply rewrites every GT/GTC/GTA(0x...) whose text has exactly
one match in Volume 2 (the rest stay as they are and are listed, to be found in the Volume 2 decompile by hand)."""
import argparse
import glob
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
GAME = os.path.join(ROOT, "games", "bookwormadv2")
sys.path.insert(0, HERE)
from gametext import va_to_off  # noqa: E402

PAT = re.compile(r"\b(GT[CA]?)\(0x([0-9a-fA-F]{6,8})\)")


def cstring(d, va):
    o = va_to_off(d, va)
    e = d.index(b"\0", o)
    return d[o:e]


def find_all(d, s):
    out = []
    start = 0
    needle = s + b"\0"
    while True:
        i = d.find(needle, start)
        if i < 0:
            return out
        if i == 0 or d[i - 1] == 0:
            out.append(i)
        start = i + 1


def off_to_va(d, off):
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    opt = struct.unpack_from("<H", d, pe + 20)[0]
    base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
    s = pe + 24 + opt
    for i in range(nsec):
        vs, rva, rs, ro = struct.unpack_from("<IIII", d, s + i * 40 + 8)
        if ro <= off < ro + rs:
            return base + rva + off - ro
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--v1", default=os.path.join(ROOT, "assets", "bookwormadv", "bookwormadventures.exe"))
    ap.add_argument("--v2", default=os.path.join(ROOT, "re", "bookwormadv2", "bin", "BookwormAdventuresVol2.exe"))
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--apply", action="store_true")
    args = ap.parse_args()
    d1 = open(args.v1, "rb").read()
    d2 = open(args.v2, "rb").read()
    files = [p for pat in ("*.cpp", "*.h") for p in glob.glob(os.path.join(GAME, "src", pat))]
    used = {}
    for path in files:
        for m in PAT.finditer(open(path, encoding="utf-8").read()):
            used[int(m.group(2), 16)] = None
    one, many, none = {}, {}, []
    for va in sorted(used):
        try:
            s = cstring(d1, va)
        except ValueError:
            none.append(va)
            continue
        hits = [off_to_va(d2, o) for o in find_all(d2, s)]
        hits = [h for h in hits if h is not None]
        if len(hits) == 1:
            one[va] = hits[0]
        elif hits:
            many[va] = hits
        else:
            none.append(va)
    # Several matches (the one-character strings of a table): the texts of a run of addresses a few bytes apart that all
    # moved by the same distance
    vas = sorted(many)
    i = 0
    while i < len(vas):
        j = i
        while j + 1 < len(vas) and vas[j + 1] - vas[j] <= 8:
            j += 1
        run = vas[i:j + 1]
        votes = {}
        for va in run:
            for h in many[va]:
                votes[h - va] = votes.get(h - va, 0) + 1
        if len(run) >= 3:
            delta, n = max(votes.items(), key=lambda kv: kv[1])
            if n == len(run):
                for va in run:
                    one[va] = va + delta
                    del many[va]
        i = j + 1
    print("%d addresses: %d with one match in Volume 2, %d with several, %d with none" % (len(used), len(one), len(many), len(none)))
    if args.list:
        for va in sorted(many):
            print("several: 0x%x -> %s" % (va, ", ".join("0x%x" % h for h in many[va])))
        for va in none:
            print("none: 0x%x" % va)
    if args.apply:
        n = 0
        for path in files:
            t = open(path, encoding="utf-8", newline="").read()

            def sub(m):
                nonlocal n
                va = int(m.group(2), 16)
                if va in one:
                    n += 1
                    return "%s(0x%x)" % (m.group(1), one[va])
                return m.group(0)
            t2 = PAT.sub(sub, t)
            if t2 != t:
                open(path, "w", encoding="utf-8", newline="").write(t2)
        todo = sorted(set(many) | set(none))
        text = "# Volume 1 addresses of GT(...) uses whose text has no single match in Volume 2: gametext.py leaves them" + chr(10)
        text += "# out (they read as empty) until each is found in Volume 2's decompile and the source rewritten." + chr(10)
        text += "".join("0x%x" % va + chr(10) for va in todo)
        with open(os.path.join(HERE, "gt_todo.txt"), "w", newline="") as f:
            f.write(text)
        print("rewrote %d uses; %d addresses left in gt_todo.txt" % (n, len(todo)))


if __name__ == "__main__":
    main()
