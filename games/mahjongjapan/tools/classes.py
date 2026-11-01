#!/usr/bin/env python3
"""List the classes of MahjongEscapeAJ.exe by their vtables (re/mahjongjapan/vtables.txt, from vtables.py): for
each, the functions that store it (constructors and destructors), its size in entries and the entries that differ
from the vtable it most resembles (its base, usually). Writes re/mahjongjapan/classes.txt.

    python games/mahjongjapan/tools/classes.py
"""
import json
import os
import struct
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pefile_lite import PE  # noqa: E402

p = PE(os.path.join(ROOT, "re", "mahjongjapan", "bin", "MahjongEscapeAJ.exe"))
funcs = {}
for l in open(os.path.join(ROOT, "re", "mahjongjapan", "gh", "functions.jsonl")):
    j = json.loads(l)
    if not j["addr"].startswith("EXT"):
        funcs[int(j["addr"], 16)] = j
starts = sorted(funcs)
names = {}
import glob  # noqa: E402
for f in sorted(glob.glob(os.path.join(ROOT, "games", "mahjongjapan", "docs", "symbols", "*.jsonl"))):
    for l in open(f, encoding="utf-8"):
        if l.strip():
            j = json.loads(l)
            if "name" in j:
                names[int(j["addr"], 16)] = j["name"]
tables = []
for l in open(os.path.join(ROOT, "re", "mahjongjapan", "vtables.txt")):
    a, *es = l.split()
    tables.append((int(a, 16), [int(e, 16) for e in es]))
name, va, vs, raw, rs = p.sections[0]
text = p.data[raw:raw + rs]
refs = {}
import bisect  # noqa: E402
for i in range(len(text) - 3):
    v = struct.unpack_from("<I", text, i)[0]
    refs.setdefault(v, []).append(0x401000 + i)


def owner(a):
    k = bisect.bisect_right(starts, a) - 1
    return starts[k] if k >= 0 else None


def nm(a):
    return names.get(a, "%x" % a)


with open(os.path.join(ROOT, "re", "mahjongjapan", "classes.txt"), "w") as out:
    for a, es in tables:
        users = sorted({owner(r) for r in refs.get(a, []) if owner(r)})
        best, bscore = None, 0
        for b, bes in tables:
            if b == a:
                continue
            n = min(len(es), len(bes))
            s = sum(1 for i in range(n) if es[i] == bes[i])
            if s > bscore and len(bes) <= len(es):
                best, bscore = b, s
        diff = ""
        if best:
            bes = dict(tables)[best]
            diff = " ".join("%d:%s" % (i, nm(e)) for i, e in enumerate(es) if i >= len(bes) or bes[i] != e)
        else:
            diff = " ".join("%d:%s" % (i, nm(e)) for i, e in enumerate(es))
        sz = [funcs[u]["size"] for u in users if u in funcs]
        out.write("vt %x n=%d base~%s users=%s\n    %s\n" % (a, len(es), "%x" % best if best else "-",
                  ",".join("%s(%d)" % (nm(u), funcs[u]["size"]) for u in users if u in funcs), diff))
print("wrote", len(tables), "classes")
