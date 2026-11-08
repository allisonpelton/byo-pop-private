#!/usr/bin/env python3
"""Generate games/astropop/src/GameTextTable.inc: where each of AstroPop's hard-coded texts is in the exe.

The port carries no game text (games/astropop/src/GameText.h): it reads every text from the player's own AstroPop.exe by
address. This finds those addresses. The input is Ghidra's string export of the game image (re/astropop/gh/strings.jsonl,
git-ignored: it holds game text); the output holds only addresses, lengths and hashes. Every string that a function of
the game's own code or of the Nuclide application layer refers to is listed, so a translation can name any of them as
GT(0xADDR).

    python games/astropop/tools/make_gametext.py [--strings re/astropop/gh/strings.jsonl] [--exe re/astropop/bin/AstroPopGame.exe]
        [--out games/astropop/src/GameTextTable.inc]
"""
import argparse
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
# The game's own code (0x401000-0x48bcf0; the framework follows)
CODE_RANGES = ((0x401000, 0x48b000),)
# Extra texts that no function refers to by a plain address: the tables of string pointers in the data section (the tutorial
# sidebar texts, the helper's speech bubbles, the help pages ...) lie between these addresses
# (the names of the high scores that the game starts with: its start-up code, 0x558040, builds them; two short lines of the
# credits)
EXTRA = [0x5639b4, 0x5639b8, 0x5639c0, 0x5639c4, 0x5639cc, 0x5639d4, 0x5639dc, 0x564c58, 0x564d98,
         0x56c388, 0x56c360, 0x56c320, 0x56c2f0, 0x56c2cc, 0x56c2b8, 0x56c28c, 0x56c260, 0x56c23c, 0x56c218, 0x56c1e0,
         0x56b758, 0x56b4c0, 0x56b4e0,
         # the names of the upgrades (the table of Board::GetUpgradeText)
         0x56a098, 0x569ce0, 0x569c98, 0x569c40, 0x569bf0, 0x569ba0, 0x569f80, 0x569f18, 0x569ed0, 0x569e68, 0x569e00, 0x569d98, 0x569d30, 0x56a218, 0x56a148, 0x56a0e0, 0x56a050, 0x569fe8, 0x56a2b8, 0x56a270, 0x56a1c8]
POINTER_TABLES = (0x573000, 0x573000)
# the read-only data where the strings are
IMMEDIATE_STRINGS = (0x55d000, 0x56c000)


# resource ids (IMAGE_..., FONT_..., SOUND_...) are identifiers, not texts (Res.h names them)
RESID = re.compile(r"^(IMAGE|FONT|SOUND)_[A-Z0-9_]+$")
# the game's own tables of string pointers in .data (rank names and the like)
DATA = (0x59c000, 0x5a0000)


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
    ap.add_argument("--strings", default=os.path.join(ROOT, "re", "astropop", "gh", "strings.jsonl"))
    ap.add_argument("--exe", default=os.path.join(ROOT, "re", "astropop", "bin", "AstroPopGame.exe"))
    ap.add_argument("--out", default=os.path.join(ROOT, "games", "astropop", "src", "GameTextTable.inc"))
    args = ap.parse_args()

    d = open(args.exe, "rb").read()
    rows = set(EXTRA)
    for line in open(args.strings, encoding="utf-8", errors="replace"):
        j = json.loads(line)
        if j["type"] != "string":
            continue
        # xrefs are the entry addresses of the functions that use the string ("data:<addr>" for a data reference)
        if RESID.match(j["value"]):
            continue
        if any(in_code(int(x, 16)) for x in j.get("xrefs", []) if re.fullmatch(r"[0-9a-f]{8}", x)) or                 any(x.startswith("data:") and DATA[0] <= int(x[5:], 16) < DATA[1] for x in j.get("xrefs", [])):
            rows.add(int(j["addr"], 16))
    # texts that only a table of pointers refers to
    known = set()
    for line in open(args.strings, encoding="utf-8", errors="replace"):
        j = json.loads(line)
        if j["type"] == "string":
            known.add(int(j["addr"], 16))
    lo, hi = POINTER_TABLES
    for va in range(lo, hi, 4):
        try:
            o = va_to_off(d, va)
        except ValueError:
            continue
        v = struct.unpack_from("<I", d, o)[0]
        if v in known:
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
            if not (op == 0x68 or 0xb8 <= op <= 0xbf or d[o - 2] == 0xc7 or d[o - 3] == 0xc7 or d[o - 4] == 0xc7):
                continue
            try:
                so = va_to_off(d, v)
            except ValueError:
                continue
            e = d.find(b"\0", so)
            t = d[so:e]
            if len(t) >= 2 and all(c in b"\r\n\t" or 0x20 <= c < 0x7f for c in t):
                rows.add(v)
    out = ["// Generated by games/astropop/tools/make_gametext.py. AstroPop Deluxe 1.0's hard-coded strings:",
           "// address in the game image, length and FNV-1a hash. The texts themselves are read from the",
           "// player's own AstroPop.exe at start-up (GameText.h); none are in the repository."]
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
