#!/usr/bin/env python3
"""Merges tools/funcmatch.py's two matches of Heavy Weapon (against Big Money's SAF 1.2 and BookWorm's 2004 framework)
into one ApplySymbols file, re/heavyweapon/names_v1.jsonl.

    python games/heavyweapon/tools/merge_matches.py

A match by bytes or calls wins over a fuzzy one; where both builds name a function by bytes, Big Money's name wins for
the application classes (Heavy Weapon has SexyAppBase split out as 1.2 does) and BookWorm's otherwise. Only names, never
code, come from the other ports' exports.
"""
import json
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
MATCH = os.path.join(ROOT, "re", "heavyweapon", "match")


def load(name, label):
    result = {}
    for line in open(os.path.join(MATCH, name), encoding="utf-8"):
        row = json.loads(line)
        # "Class::vfNN" is the other build's slot number, which differs from Heavy Weapon's: the RTTI scan's own names stay
        if row.get("name") and not row["name"].startswith(("FUN_", "thunk_FUN_", "Catch@")) and \
                not re.search(r"::vf\d+$", row["name"]):
            result[row["a"]] = (row["name"], row["how"], "%s %s (%s)" % (label, row["b"], row["how"]))
    return result


def strength(how):
    return 0 if how.startswith("fuzzy") else 1


def main():
    bigmoney = load("bm.jsonl", "Big Money")
    bookworm = load("bw.jsonl", "BookWorm")
    out = []
    for address in sorted(set(bigmoney) | set(bookworm)):
        candidates = [c for c in (bigmoney.get(address), bookworm.get(address)) if c]
        best = max(strength(c[1]) for c in candidates)
        candidates = [c for c in candidates if strength(c[1]) == best]
        if len(candidates) == 2 and "SexyApp" not in candidates[0][0]:
            candidates = [candidates[1]]
        name, how, source = candidates[0]
        comment = "; ".join(c[2] for c in (bigmoney.get(address), bookworm.get(address)) if c)
        out.append({"addr": address, "name": name, "comment": comment})
    path = os.path.join(ROOT, "re", "heavyweapon", "names_v1.jsonl")
    with open(path, "w", encoding="utf-8") as handle:
        for row in out:
            handle.write(json.dumps(row) + "\n")
    print("wrote %d names to %s" % (len(out), path))


if __name__ == "__main__":
    main()
