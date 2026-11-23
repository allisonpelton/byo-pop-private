#!/usr/bin/env python3
"""An overview of the classes of FeedingFrenzy.exe for reading the export: each vtable (re/feedingfrenzy/vtables.txt),
its slot count, the functions that store it into an object (constructors and destructors), the object size their
callers allocate (operator new's argument just before the call) and the strings its slots and constructors use.

    python -I games/feedingfrenzy/tools/classmap.py [lo hi] > <out.txt>

lo and hi (hex) limit the vtables to those whose first slot lies in that range of code. Reads and writes only the
main checkout's git-ignored re/feedingfrenzy.
"""
import json
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
RE = os.path.join(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], cwd=HERE, text=True).strip(), "re", "feedingfrenzy")


def main():
    lo, hi = (int(sys.argv[1], 16), int(sys.argv[2], 16)) if len(sys.argv) == 3 else (0x401000, 0x478000)
    data = open(os.path.join(RE, "bin", "FeedingFrenzy.exe"), "rb").read()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    opt = struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    s = pe + 24 + opt
    vs, rva, rs, ro = struct.unpack_from("<IIII", data, s + 8)  # .text is the first section
    text_va, text = base + rva, data[ro:ro + rs]
    funcs = {}
    for line in open(os.path.join(RE, "gh", "functions.jsonl"), encoding="utf-8"):
        f = json.loads(line)
        funcs[int(f["addr"], 16)] = f
    starts = sorted(funcs)
    import bisect

    def owner(va):
        k = bisect.bisect_right(starts, va) - 1
        if k >= 0 and va < starts[k] + max(funcs[starts[k]]["size"], 1):
            return starts[k]
        return None
    names = {}
    names_path = os.path.join(RE, "match", "engine.match.jsonl")
    if os.path.isfile(names_path):
        for r in map(json.loads, open(names_path, encoding="utf-8")):
            if r.get("name"):
                names[int(r["a"], 16)] = r["name"]
    # every place the code holds a vtable's address: "mov dword ptr [reg+n], imm32" (C7 /0) stores it
    tables = []
    for line in open(os.path.join(RE, "vtables.txt")):
        cols = line.split()
        tables.append((int(cols[0], 16), [int(c, 16) for c in cols[1:]]))
    for va, entries in tables:
        if not lo <= entries[0] < hi:
            continue
        needle = struct.pack("<I", va)
        stores = set()
        i = text.find(needle)
        while i >= 0:
            if i >= 2 and text[i - 2] == 0xC7:
                o = owner(text_va + i)
                if o:
                    stores.add(o)
            elif i >= 3 and text[i - 3] == 0xC7:
                o = owner(text_va + i)
                if o:
                    stores.add(o)
            i = text.find(needle, i + 1)
        strs = []
        for a in list(stores) + entries:
            f = funcs.get(a)
            if f:
                for x in f.get("strings") or []:
                    x = str(x)
                    if len(x) > 3 and x not in strs:
                        strs.append(x)
        sizes = set()
        for c in stores:
            for caller in funcs[c].get("callers") or []:
                pass
        slots = " ".join("%x%s" % (e, "=" + names[e] if e in names else "") for e in entries)
        print("vt %08x slots %d stores %s" % (va, len(entries), " ".join("%x" % c for c in sorted(stores))))
        print("   %s" % slots[:400])
        if strs:
            print("   strings: %s" % " | ".join(x[:40] for x in strs[:14]))


if __name__ == "__main__":
    main()
