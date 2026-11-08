#!/usr/bin/env python3
"""How much of the game's code the sources in games/typershark/src mention, by class.

    python games/typershark/tools/coverage.py              table: class, functions, bytes, how many are mentioned
    python games/typershark/tools/coverage.py --missing    also list the functions no source mentions (address, size, name)
    python games/typershark/tools/coverage.py --range 428500 437960 --missing    only an address range

A function counts as covered when its address (0x4xxxxx) appears in a comment line (a line that starts with //) of a file under src/ (the convention:
`// 0x4012a0` before the translation of each function, thunks and tiny forwarders count when named in a comment). The
class column is attribute.py's guess, so it is for planning only.
"""
import argparse
import collections
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import attribute as A  # noqa: E402

SRC = os.path.join(A.ROOT, "games", "typershark", "src")


def mentioned():
    seen = set()
    for p in glob.glob(os.path.join(SRC, "**", "*"), recursive=True):
        if os.path.isfile(p) and p.endswith((".cpp", ".h", ".inc")):
            for line in open(p, encoding="utf-8", errors="replace"):
                if not line.lstrip().startswith("//"):
                    continue  # a comment after code names the callee of a call, not a function translated here
                for m in re.finditer(r"0x(4[0-6][0-9a-f]{4})\b", line):
                    seen.add(int(m.group(1), 16))
    return seen


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--missing", action="store_true")
    ap.add_argument("--range", nargs=2, metavar=("LO", "HI"))
    args = ap.parse_args()
    fs = A.load()
    cls, _ = A.attribute(fs)
    seen = mentioned()
    lo, hi = (int(args.range[0], 16), int(args.range[1], 16)) if args.range else (A.LO, A.HI)
    tot = collections.Counter()
    cov = collections.Counter()
    cnt = collections.Counter()
    ccnt = collections.Counter()
    for a, f in fs.items():
        if not lo <= a < hi or cls[a] == "(stl)":
            continue
        tot[cls[a]] += f["size"]
        cnt[cls[a]] += 1
        if a in seen:
            cov[cls[a]] += f["size"]
            ccnt[cls[a]] += 1
    print("%-26s %9s %9s %9s" % ("class", "functions", "bytes", "covered"))
    for c, t in sorted(tot.items(), key=lambda x: -x[1]):
        print("%-26s %4d/%-4d %9d %8d%%" % (c, ccnt[c], cnt[c], t, 100 * cov[c] // t if t else 0))
    all_t, all_c = sum(tot.values()), sum(cov.values())
    print("%-26s %4d/%-4d %9d %8d%%" % ("(all, without STL)", sum(ccnt.values()), sum(cnt.values()), all_t, 100 * all_c // all_t if all_t else 0))
    if args.missing:
        for a in sorted(fs):
            if lo <= a < hi and cls[a] != "(stl)" and a not in seen:
                print("%08x %5d  %-22s %s" % (a, fs[a]["size"], cls[a], fs[a]["name"]))


if __name__ == "__main__":
    main()
