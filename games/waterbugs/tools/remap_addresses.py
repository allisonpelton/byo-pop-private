#!/usr/bin/env python3
"""Carry the addresses in the Cosmic Bugs sources over to Water Bugs: functions, globals and strings.

Water Bugs and Cosmic Bugs are one engine and almost one game. tools/funcmatch.py pairs 1186 of Water Bugs' 1319
functions with Cosmic Bugs' (re/waterbugs/match.jsonl); in each pair whose code is identical apart from addresses,
the n-th address operand of one is the same global (or string, or function) as the n-th of the other. The pairs
vote for a map from every Cosmic Bugs address to Water Bugs'. This rewrites the `0x...` addresses of the files given
(comments and GT() alike) through that map. An address with no vote becomes `cb:0x...`: it is still the original
game's, and has to be found by hand.

    python games/waterbugs/tools/remap_addresses.py <file>... [--map re/waterbugs/addrmap.json]
"""
import collections
import json
import re
import struct
import sys

sys.path.insert(0, __file__.replace("\\", "/").rsplit("/", 1)[0])
from datadiff import Image  # noqa: E402


def build(match, wexe, cexe, wfun, cfun):
    W = Image(wexe, wfun)
    C = Image(cexe, cfun)
    votes = collections.defaultdict(collections.Counter)
    for line in open(match, encoding="utf-8"):
        m = json.loads(line)
        a, b = int(m["a"], 16), int(m["b"], 16)
        votes[b][a] += 1000  # the function itself
        if a not in W.sizes or b not in C.sizes or W.sizes[a] == 0:
            continue
        wb, wops = W.masked(a)
        cb, cops = C.masked(b)
        if wb != cb:
            continue
        for (_, wv), (_, cv) in zip(wops, cops):
            votes[cv][wv] += 1
    amap = {}
    for cv, c in votes.items():
        wv, n = c.most_common(1)[0]
        amap[cv] = wv
    return amap


def rewrite(text, amap, unmapped):
    def sub(m):
        v = int(m.group(2), 16)
        if not 0x401000 <= v < 0x480000:
            return m.group(0)
        if v in amap:
            return "0x%0*x" % (len(m.group(2)), amap[v])
        unmapped.add(v)
        return "cb:0x" + m.group(2)
    return re.sub(r"(cb:)?\b0x([0-9a-f]{6,8})\b", sub, text)


def main():
    args = sys.argv[1:]
    mp = "re/waterbugs/addrmap.json"
    if "--map" in args:
        i = args.index("--map")
        mp = args[i + 1]
        del args[i:i + 2]
    try:
        amap = {int(k): v for k, v in json.load(open(mp)).items()}
    except OSError:
        amap = build("re/waterbugs/match.jsonl", "re/waterbugs/bin/WaterBugs.exe", "assets/cosmicbugs/CosmicBugs.exe",
                     "re/waterbugs/gh/functions.jsonl", "re/cosmicbugs/gh/functions.jsonl")
        json.dump({str(k): v for k, v in amap.items()}, open(mp, "w"))
        print("map: %d addresses -> %s" % (len(amap), mp))
    unmapped = set()
    for path in args:
        t = open(path, encoding="utf-8", newline="").read()
        n = rewrite(t, amap, unmapped)
        if n != t:
            open(path, "w", encoding="utf-8", newline="").write(n)
    print("unmapped: %d" % len(unmapped))


if __name__ == "__main__":
    main()
