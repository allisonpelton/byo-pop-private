#!/usr/bin/env python3
"""Show how a Water Bugs function differs from its Cosmic Bugs twin, in the two Ghidra decompilations with the
addresses of Cosmic Bugs' version carried over to Water Bugs' (re/waterbugs/addrmap.json, made by
remap_addresses.py), so that only real differences remain. Strings are inlined for reading, never written to a file.

    python games/waterbugs/tools/fdiff.py <water addr> [<cosmic addr>]     # the twin is looked up in match.jsonl
    python games/waterbugs/tools/fdiff.py --full <water addr> [<cosmic addr>]   # both sides, not only the changes
"""
import difflib
import glob
import json
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
BS = chr(92)


def esc(s):
    return '"' + s.replace(BS, BS + BS).replace('"', BS + '"').replace("\n", BS + "n").replace("\r", BS + "r") + '"'


def strings(game):
    d = {}
    for l in open(os.path.join(ROOT, "re", game, "gh", "strings.jsonl"), encoding="utf-8"):
        j = json.loads(l)
        if j["type"] == "string":
            d[j["addr"]] = j["value"]
    return d


def decomp(game, a):
    fs = glob.glob(os.path.join(ROOT, "re", game, "gh", "decomp", "*", "%08x_*.c" % a))
    if not fs:
        sys.exit("no decompilation of %s %08x" % (game, a))
    return open(fs[0], encoding="latin1").read()


def lines(t, strs, amap, names):
    def sym(m):
        h = m.group(2)
        if amap is not None:
            h = "%08x" % amap.get(int(h, 16), int(h, 16))
        return m.group(1) + h
    t = re.sub(r"\b((?:thunk_)?FUN_|DAT_|PTR_DAT_|PTR_|LAB_|switchD_|caseD_)([0-9a-f]{8})\b", sym, t)

    def hexc(m):
        v = int(m.group(0), 16)
        return "0x%x" % amap.get(v, v) if amap is not None and 0x401000 <= v < 0x480000 else m.group(0)
    t = re.sub(r"\b0x4[0-7][0-9a-f]{4}\b", hexc, t)
    t = re.sub(r"\b(?:thunk_)?FUN_([0-9a-f]{8})\b", lambda m: names.get(m.group(1), m.group(0)), t)

    def inl(m):
        k = m.group(1)
        return esc(strs[k]) if k in strs else m.group(0)
    t = re.sub(r"s_\w+?_([0-9a-f]{8})\b", inl, t)
    t = re.sub(r"&?(?:DAT|PTR_DAT|PTR)_([0-9a-f]{8})\b", lambda m: ("(str)" + esc(strs[m.group(1)])) if m.group(1) in strs else m.group(0), t)
    seen = {}

    def local(m):
        return seen.setdefault(m.group(0), "v%d" % (len(seen) + 1))
    t = re.sub(r"\b(?:[a-z]{1,3}Var\d+|local_[0-9a-f]+|param_\d+|[a-z]Stack_\w+|extraout_\w+|in_\w+|unaff_\w+|this_\d+)\b", local, t)
    t = re.sub(r"\b(?:LAB|caseD|switchD|switchdataD)_[0-9a-f]{8}\b", "LABEL", t)
    t = re.sub(r"\bDAT_004622e0\b", "gGameFile", t)
    out = []
    for ln in t.split("\n"):
        ln = ln.rstrip()
        if ln.strip() and not re.match(r"^\s*(undefined\d?|int|uint|char|byte|bool|float|double|long|short|ushort|"
                                       r"void|LPVOID|DWORD|code|[A-Za-z_]+ \*)\s*\*?\s*\*?[A-Za-z_]\w*(\[\d+\])?;\s*$", ln):
            out.append(ln)
    return out


def main():
    args = sys.argv[1:]
    full = "--full" in args
    args = [a for a in args if a != "--full"]
    if not args:
        sys.exit(__doc__)
    a = int(args[0].replace("0x", ""), 16)
    if len(args) > 1:
        b = int(args[1].replace("0x", ""), 16)
    else:
        b = None
        for l in open(os.path.join(ROOT, "re", "waterbugs", "match.jsonl"), encoding="utf-8"):
            j = json.loads(l)
            if int(j["a"], 16) == a:
                b = int(j["b"], 16)
        if b is None:
            sys.exit("no twin of %08x in match.jsonl: give the Cosmic Bugs address" % a)
    amap = {int(k): v for k, v in json.load(open(os.path.join(ROOT, "re", "waterbugs", "addrmap.json"))).items()}
    ws, cs = strings("waterbugs"), strings("cosmicbugs")
    cs = {"%08x" % amap.get(int(k, 16), int(k, 16)): v for k, v in cs.items()}
    names = {}
    wl = lines(decomp("waterbugs", a), ws, None, names)
    cl = lines(decomp("cosmicbugs", b), cs, amap, names)
    print("// water %08x vs cosmic %08x" % (a, b))
    if full:
        print("\n".join(wl))
        return
    def norm(s):
        s = re.sub(r"\((?:int|int \*|void \*|uint)\)\s*(v\d+)", r"\1", s)
        s = re.sub(r"\(undefined4 \*\)|\(float \*\)|\(int \*\)|\(char \*\)", "", s)
        s = re.sub(r"(?:DAT|PTR_DAT|_DAT|PTR)_[0-9a-f]{8}", "DAT", re.sub(r"\bv\d+\b", "v", s))
        return re.sub(r"\s+", "", s)
    sm = difflib.SequenceMatcher(None, [norm(x) for x in cl], [norm(x) for x in wl], autojunk=False)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            continue
        print("@@ cosmic %d,%d  water %d,%d" % (i1 + 1, i2 - i1, j1 + 1, j2 - j1))
        for x in cl[i1:i2]:
            print("-" + x)
        for x in wl[j1:j2]:
            print("+" + x)


if __name__ == "__main__":
    main()
