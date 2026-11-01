#!/usr/bin/env python3
"""Print functions of the Ghidra export (re/mahjongjapan/gh/decomp) in a compact, readable form: names from
games/mahjongjapan/docs/symbols, strings inlined (for reading only, never written to a file in the repository), local
declarations and blank lines dropped.

    python games/mahjongjapan/tools/decread.py 4092a0 ...            # these functions
    python games/mahjongjapan/tools/decread.py 401000-402000         # every function in the range
    python games/mahjongjapan/tools/decread.py --list 401000-402000  # one line each: address, size, name, strings
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
GH = os.path.join(ROOT, "re", "mahjongjapan", "gh")
strs = {}
for l in open(os.path.join(GH, "strings.jsonl"), encoding="utf-8"):
    j = json.loads(l)
    if j["type"] == "string":
        strs[j["addr"]] = j["value"]
names = {}
for p in sorted(glob.glob(os.path.join(ROOT, "games", "mahjongjapan", "docs", "symbols", "*.jsonl"))):
    for l in open(p, encoding="utf-8"):
        l = l.strip()
        if l:
            j = json.loads(l)
            if "name" in j:
                names[j["addr"].lower().zfill(8)] = j["name"].split("::")[-1]
            elif "label" in j:
                names[j["addr"].lower().zfill(8)] = j["label"]
funcs = {}
for l in open(os.path.join(GH, "functions.jsonl"), encoding="utf-8"):
    j = json.loads(l)
    funcs[int(j["addr"], 16)] = j
decl = re.compile(r"^\s*(undefined\d?|int|uint|char|byte|bool|float|double|long|short|ushort|longlong|ulonglong|"
                  r"void|LPVOID|DWORD|code|HWND|[A-Za-z_]+ \*)\s*\*?\s*(\*)?[A-Za-z_]\w*(\[\d+\])?;\s*$")
BS = chr(92)


def esc(s):
    return '"' + s.replace(BS, BS + BS).replace('"', BS + '"').replace("\n", BS + "n").replace("\r", BS + "r") + '"'


def comp(t):
    t = re.sub(r"\b(?:thunk_)?FUN_([0-9a-f]{8})\b", lambda m: names.get(m.group(1), m.group(0)), t)
    t = re.sub(r"s_\w+?_([0-9a-f]{8})\b", lambda m: esc(strs[m.group(1)]) if m.group(1) in strs else m.group(0), t)
    t = re.sub(r"&?(?:DAT|PTR_DAT|PTR)_([0-9a-f]{8})\b",
               lambda m: ("(str)" + esc(strs[m.group(1)])) if m.group(1) in strs
               else names.get(m.group(1), m.group(0)), t)
    return t


def addrs(arg):
    if "-" in arg:
        lo, hi = (int(x, 16) for x in arg.split("-"))
        return [a for a in sorted(funcs) if lo <= a < hi]
    return [int(arg, 16)]


def show(a):
    key = "%08x" % a
    fs = glob.glob(os.path.join(GH, "decomp", "*", key + "_*.c"))
    if not fs:
        print("// no decomp for", key)
        return
    t = comp(open(fs[0], encoding="latin1").read())
    out = []
    for ln in t.split("\n"):
        if not ln.strip() or decl.match(ln):
            continue
        out.append(ln)
    print("\n".join(out))
    print()


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    listing = args[0] == "--list"
    for arg in args[1:] if listing else args:
        for a in addrs(arg.lower().replace("0x", "")):
            if listing:
                f = funcs.get(a)
                if f:
                    ss = " | ".join(s[:30].replace("\n", " ") for s in f["strings"][:3])
                    print("%08x %5d %3dc %-28s %s" % (a, f["size"], len(f["callers"]),
                                                       names.get("%08x" % a, f["name"]), ss))
            else:
                show(a)


if __name__ == "__main__":
    main()
