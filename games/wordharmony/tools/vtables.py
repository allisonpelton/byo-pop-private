#!/usr/bin/env python3
"""Find the vtables of WordHarmonyGame.exe and the virtual functions Ghidra's analysis missed.

Word Harmony is C++ (Sprout's engine and the game on it) with RTTI only for three engine classes and the exceptions,
so Ghidra finds almost no vtables, and a virtual function that is only ever called through its vtable is never
disassembled. A vtable is a run of at least two pointers into the program's own code (0x401000-0x446000, the game and
the engine, and 0x454a40-0x45ee00, the engine's crash handler, display and image loader; the libraries are C) in
.rdata, the first of them referenced from the code (a constructor stores it in the object).

    python games/wordharmony/tools/vtables.py [--exe re/wordharmony/bin/WordHarmonyGame.exe]
        [--functions re/wordharmony/gh/functions.jsonl]
        writes re/wordharmony/vtables.txt (one vtable a line: its address and entries) and
        re/wordharmony/vfuncs.txt (the entries that are not functions yet, for WHCreateFunctions.java)
"""
import argparse
import bisect
import json
import os
import struct

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
CODE = [(0x401000, 0x446000), (0x454a40, 0x45ee00)]


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
    ap.add_argument("--exe", default=os.path.join(ROOT, "re", "wordharmony", "bin", "WordHarmonyGame.exe"))
    ap.add_argument("--functions", default=os.path.join(ROOT, "re", "wordharmony", "gh", "functions.jsonl"))
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
    with open(os.path.join(ROOT, "re", "wordharmony", "vtables.txt"), "w") as f:
        for a, es in tables:
            f.write("%08x %s\n" % (a, " ".join("%x" % e for e in es)))
    with open(os.path.join(ROOT, "re", "wordharmony", "vfuncs.txt"), "w") as f:
        for e in missing:
            f.write("%08x\n" % e)
    print("%d vtables, %d entries not yet functions" % (len(tables), len(missing)))


if __name__ == "__main__":
    main()
