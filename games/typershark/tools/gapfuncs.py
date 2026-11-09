#!/usr/bin/env python3
"""List the function starts that Ghidra's analysis missed in TyperSharkGame.exe, as ApplySymbols.java lines.

TyperSharkGame.exe has RTTI only for the framework's classes, so the virtual functions of the game's own classes (reached
only through vtables) were not recognised as functions: some of the game's functions. They show as gaps in the
function list of the export. A candidate is an address that starts a 16-byte aligned run of code after a ret or
padding inside a gap; applying them (ApplySymbols creates the function) and exporting again fills the gaps.

    python games/typershark/tools/gapfuncs.py [--exe re/typershark/bin/TyperSharkGame.exe] [--functions re/typershark/gh/functions.jsonl]
        [--out re/typershark/gapfuncs.jsonl]
    GHIDRA_PROJECT=typershark_game GHIDRA_PROGRAM=TyperSharkGame.exe EXPORT_DIR=$(pwd -W)/re/typershark/gh \
        build/native/rundesk.exe bash tools/ghidra_headless.sh apply re/typershark/gapfuncs.jsonl
"""
import argparse
import json
import struct

GAME_END = 0x440000      # the game's own code is 0x401000-0x43f000; the framework follows


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", default="re/typershark/bin/TyperSharkGame.exe")
    ap.add_argument("--functions", default="re/typershark/gh/functions.jsonl")
    ap.add_argument("--out", default="re/typershark/gapfuncs.jsonl")
    args = ap.parse_args()
    d = open(args.exe, "rb").read()
    base = 0x400000
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    opt = struct.unpack_from("<H", d, pe + 20)[0]
    off = pe + 24 + opt
    tva = trp = 0
    for _ in range(nsec):
        name = d[off:off + 8].rstrip(b"\0")
        vsz, va, rsz, rp = struct.unpack_from("<IIII", d, off + 8)
        if name == b".text":
            tva, trp = base + va, rp
        off += 40

    def byte(a):
        return d[trp + a - tva]

    fs = sorted((json.loads(l) for l in open(args.functions)), key=lambda f: int(f["addr"], 16))
    prev = 0x401000
    gaps = []
    for f in fs:
        a = int(f["addr"], 16)
        if a > GAME_END:
            break
        if a - prev > 0x10 and prev >= 0x401000:
            gaps.append((prev, a))
        prev = max(prev, a + f["size"])
    cands = []
    for g0, g1 in gaps:
        for a in range(g0, g1):
            if a % 16 != 0 or byte(a) in (0xCC, 0x90):
                continue
            if byte(a - 1) in (0xCC, 0x90, 0xC3) or byte(a - 3) == 0xC2:
                cands.append(a)
    with open(args.out, "w", newline="\n") as out:
        for a in cands:
            out.write(json.dumps({"addr": "%08x" % a, "name": "FUN_%08x" % a}) + "\n")
    print("%d gaps, %d candidates -> %s" % (len(gaps), len(cands), args.out))


if __name__ == "__main__":
    main()
