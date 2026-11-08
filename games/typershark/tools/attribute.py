#!/usr/bin/env python3
"""Guess which class each unnamed function of the game code belongs to, for splitting the translation into jobs.

    python games/typershark/tools/attribute.py                 table: class, number of functions, bytes
    python games/typershark/tools/attribute.py --class Board   the functions guessed for that class (address, size, how)
    python games/typershark/tools/attribute.py --range 43fbd0 462290   the same table for an address range

A function is guessed to belong to the class whose named methods call it, repeated until nothing changes (a helper
called only by Board's methods is Board's; a helper called from two classes stays "shared"). The ctor/dtor names
the export gives (`Sexy::X::ctor_or_dtor_...`) and the `Class::vfNN` names are taken as given. This is a guess for
planning only: translations decide by reading the code.
"""
import argparse
import collections
import json
import os

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
GH = os.path.join(ROOT, "re", "typershark", "gh")
LO, HI = 0x401000, 0x4623F0


def load():
    fs = {}
    for line in open(os.path.join(GH, "functions.jsonl"), encoding="utf-8"):
        f = json.loads(line)
        a = int(f["addr"], 16)
        if LO <= a < HI:
            fs[a] = f
    return fs


def is_stl(ns):
    return ns.startswith("std") or ns == "Sexy::SexyString"


def attribute(fs):
    cls = {}
    how = {}
    for a, f in fs.items():
        ns = f.get("ns") or "Global"
        if ns != "Global" and not is_stl(ns):
            cls[a] = ns.replace("Sexy::", "")
            how[a] = "named"
        elif is_stl(ns):
            cls[a] = "(stl)"
            how[a] = "stl"
    changed = True
    while changed:
        changed = False
        for a, f in fs.items():
            if a in cls:
                continue
            owners = set()
            for c in f["callers"]:
                ca = int(c, 16)
                if ca in cls and cls[ca] != "(stl)":
                    owners.add(cls[ca])
                elif ca in fs:
                    owners.add(None)
            known = {o for o in owners if o}
            if len(known) == 1 and None not in owners:
                cls[a] = next(iter(known))
                how[a] = "callers"
                changed = True
    for a in fs:
        if a not in cls:
            cls[a] = "(unknown)"
            how[a] = "?"
    return cls, how


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--class", dest="cls")
    ap.add_argument("--range", nargs=2, metavar=("LO", "HI"))
    args = ap.parse_args()
    fs = load()
    cls, how = attribute(fs)
    lo, hi = (int(args.range[0], 16), int(args.range[1], 16)) if args.range else (LO, HI)
    if args.cls:
        for a in sorted(fs):
            if cls[a] == args.cls and lo <= a < hi:
                print("%08x %5d  %-9s %s" % (a, fs[a]["size"], how[a], fs[a]["name"]))
        return
    tot = collections.Counter()
    cnt = collections.Counter()
    for a in fs:
        if lo <= a < hi:
            tot[cls[a]] += fs[a]["size"]
            cnt[cls[a]] += 1
    for c, t in sorted(tot.items(), key=lambda x: -x[1]):
        print("%-28s %4d functions %7d bytes" % (c, cnt[c], t))


if __name__ == "__main__":
    main()
