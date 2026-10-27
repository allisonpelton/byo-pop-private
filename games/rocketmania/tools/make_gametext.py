#!/usr/bin/env python3
"""Generate games/rocketmania/src/GameTextTable.inc: where each of Rocket Mania's hard-coded texts is in the exe.

The port carries no game text (games/rocketmania/src/GameText.h): it reads every text from the player's own RocketMania.exe by
address. This finds those addresses. The input is Ghidra's string export of the game image (re/rocketmania/gh/strings.jsonl,
git-ignored: it holds game text); the output holds only addresses, lengths and hashes. Every string that a function of
the game's own code or of the Nuclide application layer refers to is listed, so a translation can name any of them as
GT(0xADDR).

    python games/rocketmania/tools/make_gametext.py [--strings re/rocketmania/gh/strings.jsonl] [--exe re/rocketmania/bin/RocketManiaGame.exe]
        [--out games/rocketmania/src/GameTextTable.inc]
"""
import argparse
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
# The game's own code (0x401000-0x42f900); the framework is linked after it (0x42f900-0x4a0000)
CODE_RANGES = ((0x401000, 0x42f900),)
# Extra texts that no function refers to by a plain address: the tables of string pointers in the data section (the tutorial
# sidebar texts, the helper's speech bubbles, the help pages ...) lie between these addresses
EXTRA = [
    0x4e4ff0, 0x4e5040, 0x4e5078,   # the singular counts on the end-of-level screen ("1 gem", "1 bomb", "1 coin"): Ghidra
]                                   # leaves them as the tails of other strings
POINTER_TABLES = (
    (0x4e44d0, 0x4e44ec),   # GameSelector's hint under the buttons (0x4059d0)
    (0x4e4658, 0x4e468c),   # the names in the high score lists made new (0x4064e0), the first of two tables
    (0x4e4690, 0x4e46d4),   # the second
    (0x4e62e8, 0x4e6354),   # the rank names of the level-up dialog (0x428ca0)
    (0x4e57b0, 0x4e5970),   # the eight lines of each hover tip, for arcade and classic and then for strategy (0x413c00)
)
# the read-only data where the strings are
IMMEDIATE_STRINGS = (0x4e4000, 0x4e8000)


def fnv1a(data):
    h = 2166136261
    for b in data:
        h ^= b
        h = (h * 16777619) & 0xFFFFFFFF
    return h


def va_to_off(d, va):
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    opt = struct.unpack_from("<H", d, pe + 20)[0]
    base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
    s = pe + 24 + opt
    for i in range(nsec):
        vs, rva, rs, ro = struct.unpack_from("<IIII", d, s + i * 40 + 8)
        if rva <= va - base < rva + max(vs, rs):
            return ro + va - base - rva
    raise ValueError(hex(va))


def in_code(addr):
    return any(lo <= addr < hi for lo, hi in CODE_RANGES)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--strings", default=os.path.join(ROOT, "re", "rocketmania", "gh", "strings.jsonl"))
    ap.add_argument("--exe", default=os.path.join(ROOT, "re", "rocketmania", "bin", "RocketManiaGame.exe"))
    ap.add_argument("--out", default=os.path.join(ROOT, "games", "rocketmania", "src", "GameTextTable.inc"))
    args = ap.parse_args()

    d = open(args.exe, "rb").read()
    rows = set(EXTRA)
    for line in open(args.strings, encoding="utf-8", errors="replace"):
        j = json.loads(line)
        if j["type"] != "string":
            continue
        # xrefs are the entry addresses of the functions that use the string ("data:<addr>" for a data reference)
        if any(in_code(int(x, 16)) for x in j.get("xrefs", []) if re.fullmatch(r"[0-9a-f]{8}", x)):
            rows.add(int(j["addr"], 16))
    # texts that only a table of pointers refers to
    known = set()
    for line in open(args.strings, encoding="utf-8", errors="replace"):
        j = json.loads(line)
        if j["type"] == "string":
            known.add(int(j["addr"], 16))
    for lo, hi in POINTER_TABLES:
        for va in range(lo, hi, 4):
            try:
                o = va_to_off(d, va)
            except ValueError:
                continue
            v = struct.unpack_from("<I", d, o)[0]
            if v in known:
                rows.add(v)
                continue
            # Ghidra leaves some short strings (the default high score names) undefined: take a pointer to
            # printable text in a table listed above as it is
            try:
                so = va_to_off(d, v)
            except ValueError:
                continue
            t = d[so:d.index(b"\0", so)]
            if len(t) >= 2 and all(0x20 <= c < 0x7f for c in t):
                rows.add(v)
    # strings Ghidra did not define as strings (it merges or skips some): an address the code pushes or loads
    lo, hi = IMMEDIATE_STRINGS
    for a, b in CODE_RANGES:
        o0 = va_to_off(d, a)
        for o in range(o0 + 4, o0 + (b - a) - 4):
            v = struct.unpack_from("<I", d, o)[0]
            if not (lo <= v < hi):
                continue
            op = d[o - 1]
            if not (op == 0x68 or 0xb8 <= op <= 0xbf or d[o - 2] == 0xc7 or d[o - 3] == 0xc7 or d[o - 4] == 0xc7 or d[o - 6] == 0xc7 or d[o - 7] == 0xc7):
                continue
            try:
                so = va_to_off(d, v)
            except ValueError:
                continue
            e = d.find(b"\0", so)
            t = d[so:e]
            if len(t) >= 2 and all(c in b"\r\n\t" or 0x20 <= c < 0x7f for c in t):
                rows.add(v)
    out = ["// Generated by games/rocketmania/tools/make_gametext.py. Rocket Mania Deluxe 1.0's hard-coded strings:",
           "// address in the game image, length and FNV-1a hash. The texts themselves are read from the",
           "// player's own RocketMania.exe at start-up (GameText.h); none are in the repository."]
    n = 0
    for va in sorted(rows):
        try:
            o = va_to_off(d, va)
        except ValueError:
            continue
        e = d.index(b"\0", o)
        s = d[o:e]
        out.append("GAMETEXT(0x%08x, %d, 0x%08x)" % (va, len(s), fnv1a(s)))
        n += 1
    open(args.out, "w", newline="\n").write("\n".join(out) + "\n")
    print("wrote %d entries to %s" % (n, args.out))


if __name__ == "__main__":
    sys.exit(main())
