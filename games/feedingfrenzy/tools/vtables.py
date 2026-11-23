#!/usr/bin/env python3
"""Find the vtables of FeedingFrenzy.exe and the virtual functions Ghidra's analysis missed.

Feeding Frenzy is C++ (Sprout's engine and the game on it) with RTTI only for three engine classes and the exceptions,
so Ghidra finds almost no vtables, and a virtual function that is only ever called through its vtable is never
disassembled. A vtable is a run of at least two pointers into the code section in .rdata, the first of them referenced
from the code (a constructor stores it in the object). Its slots may point anywhere in the code: to _purecall in the C
runtime, or to tiny bodies the linker folded into one that lies among the libraries' code (0x4d0ca0 among libpng's).
The export is in the main checkout's re/feedingfrenzy (git-ignored).

    python -I games/feedingfrenzy/tools/vtables.py [--exe re/feedingfrenzy/bin/FeedingFrenzy.exe]
        [--functions re/feedingfrenzy/gh/functions.jsonl]
        writes re/feedingfrenzy/vtables.txt (one vtable a line: its address and entries) and
        re/feedingfrenzy/vfuncs.txt (the entries that are not functions yet, for FFCreateFunctions.java)
"""
import argparse
import bisect
import json
import os
import struct
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))


def top_level():
    """The checkout that holds re/ (git-ignored): this worktree's."""
    return subprocess.check_output(["git", "rev-parse", "--show-toplevel"], cwd=HERE, text=True).strip()


ROOT = top_level()
CODE = [(0x401000, 0x49d000)]


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
    ap.add_argument("--exe", default=os.path.join(ROOT, "re", "feedingfrenzy", "bin", "FeedingFrenzy.exe"))
    ap.add_argument("--functions", default=os.path.join(ROOT, "re", "feedingfrenzy", "gh", "functions.jsonl"))
    args = ap.parse_args()
    d = open(args.exe, "rb").read()
    sec = sections(d)
    funcs = {}
    for l in open(args.functions):
        j = json.loads(l)
        funcs[int(j["addr"], 16)] = j["size"]
    starts = sorted(funcs)

    def inside(v):  # strictly inside a known function: a switch's jump table, not a vtable
        k = bisect.bisect_right(starts, v) - 1
        return k >= 0 and starts[k] < v < starts[k] + funcs[starts[k]]
    rva, rro, rrs = sec[".rdata"]
    tva, tro, trs = sec[".text"]
    text = d[tro:tro + trs]
    # every 32-bit immediate the code mentions (mov [reg], imm32 and push imm32 included)
    refs = set()
    for i in range(len(text) - 3):
        v = struct.unpack_from("<I", text, i)[0]
        if rva <= v < rva + rrs:
            refs.add(v)
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
    with open(os.path.join(ROOT, "re", "feedingfrenzy", "vtables.txt"), "w") as f:
        for a, es in tables:
            f.write("%08x %s\n" % (a, " ".join("%x" % e for e in es)))
    with open(os.path.join(ROOT, "re", "feedingfrenzy", "vfuncs.txt"), "w") as f:
        for e in missing:
            f.write("%08x\n" % e)
    print("%d vtables, %d entries not yet functions" % (len(tables), len(missing)))


if __name__ == "__main__":
    main()
