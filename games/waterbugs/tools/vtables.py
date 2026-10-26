#!/usr/bin/env python3
"""Find the vtables of WaterBugs.exe and the virtual functions Ghidra's analysis missed.

Water Bugs is C++ (Retro64's engine and the game on it) without RTTI, so Ghidra finds no vtables, and a virtual
function that is only ever called through its vtable is never disassembled. A vtable is a run of at least two
pointers into the program's own code (0x401000-0x442e58, the engine and the game; what follows is BASS's
import thunks and the C runtime) in .rdata, the first of them referenced from the code (a constructor stores it in the object).

    python games/waterbugs/tools/vtables.py [--exe re/waterbugs/bin/WaterBugs.exe] [--functions re/waterbugs/gh/functions.jsonl]
        writes re/waterbugs/vtables.txt (one vtable a line: its address and entries) and
        re/waterbugs/vfuncs.txt (the entries that are not functions yet, for CosmicCreateFunctions.java)
"""
import argparse
import json
import os
import re
import struct

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
CODE = [(0x401000, 0x442e58)]


def sections(d):
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    opt = struct.unpack_from("<H", d, pe + 20)[0]
    base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
    s = pe + 24 + opt
    out = {}
    for i in range(nsec):
        name = d[s + i * 40:s + i * 40 + 8].rstrip(b"\0").decode()
        vs, rva, rs, ro = struct.unpack_from("<IIII", d, s + i * 40 + 8)
        out[name] = (base + rva, ro, rs)
    return out


def in_code(v):
    return any(lo <= v < hi for lo, hi in CODE)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", default=os.path.join(ROOT, "re", "waterbugs", "bin", "WaterBugs.exe"))
    ap.add_argument("--functions", default=os.path.join(ROOT, "re", "waterbugs", "gh", "functions.jsonl"))
    args = ap.parse_args()
    d = open(args.exe, "rb").read()
    sec = sections(d)
    funcs = {}
    for l in open(args.functions):
        j = json.loads(l)
        funcs[int(j["addr"], 16)] = j["size"]
    starts = sorted(funcs)

    def inside(v):  # strictly inside a known function: a switch's jump table, not a vtable
        import bisect
        k = bisect.bisect_right(starts, v) - 1
        return k >= 0 and starts[k] < v < starts[k] + funcs[starts[k]]
    tva, tro, trs = sec[".text"]
    text = d[tro:tro + trs]
    # every 32-bit immediate the code mentions (mov [reg], imm32 and push imm32 included)
    refs = set()
    for i in range(len(text) - 3):
        v = struct.unpack_from("<I", text, i)[0]
        if 0x458000 <= v < 0x460000:  # .rdata
            refs.add(v)
    rva, rro, rrs = sec[".rdata"]
    words = [struct.unpack_from("<I", d, rro + i)[0] for i in range(0, rrs - 3, 4)]
    tables = []
    i = 0
    while i < len(words):
        if in_code(words[i]) and (rva + i * 4) in refs:
            j = i
            while j < len(words) and in_code(words[j]) and (j == i or (rva + j * 4) not in refs):
                j += 1
            if j - i >= 2 and not any(inside(e) for e in words[i:j]):
                tables.append((rva + i * 4, words[i:j]))
            i = j
        else:
            i += 1
    missing = sorted({e for _, es in tables for e in es if e not in funcs})
    with open(os.path.join(ROOT, "re", "waterbugs", "vtables.txt"), "w") as f:
        for a, es in tables:
            f.write("%08x %s\n" % (a, " ".join("%x" % e for e in es)))
    with open(os.path.join(ROOT, "re", "waterbugs", "vfuncs.txt"), "w") as f:
        for e in missing:
            f.write("%08x\n" % e)
    print("%d vtables, %d entries not yet functions" % (len(tables), len(missing)))


if __name__ == "__main__":
    main()
