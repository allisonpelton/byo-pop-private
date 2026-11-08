#!/usr/bin/env python3
"""Generate games/sevenseas/src/GameTextTable.inc: where each of Seven Seas's hard-coded texts is in the exe.

The port carries no game text (games/sevenseas/src/GameText.h): it reads every text from the player's own
WinSS.exe by address. This finds those addresses. The input is Ghidra's string export of the game image
(re/sevenseas/gh/strings.jsonl, git-ignored: it holds game text); the output holds only addresses, lengths and
hashes. Every string between the game's first and last data string is listed, so a translation can name any
of them as GT(0xADDR).

    python games/sevenseas/tools/make_gametext.py [--strings re/sevenseas/gh/strings.jsonl] [--exe re/sevenseas/bin/WinSS.exe]
        [--out games/sevenseas/src/GameTextTable.inc]
"""
import argparse
import glob
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
FIRST = 0x4a0000   # the game's .data strings (dialog texts, tutorial pages, labels)
LAST = 0x4a6c00
GAME_CODE_END = 0x428000   # the game's own functions end here; the framework and the C library follow

EXCLUDE = (0x4a3764, 0x4a3830, 0x4a388c, 0x4a38ec, 0x4a3bf0, 0x4a3f2c, 0x4a3f58, 0x4a3fb0, 0x4a3ff0,
           0x4a59c4, 0x4a59e4, 0x4a59f0, 0x4a5a00, 0x4a5a10, 0x4a5a30, 0x4a5a58, 0x4a5a80, 0x4a621c,
           0x4a6460, 0x4a6468, 0x4a6470, 0x4a6494, 0x4a64a4, 0x4a64cc, 0x4a64d8, 0x4a64e8, 0x4a64f0,
           0x4a6578, 0x4a6588, 0x4a6670, 0x4a6694, 0x4a66bc, 0x4a66d4, 0x4a66ec, 0x4a6700, 0x4a6740,
           0x4a6774, 0x4a67a4, 0x4a67d4, 0x4a67fc, 0x4a6828, 0x4a6854, 0x4a6888, 0x4a68bc, 0x4a68ec,
           0x4a6acc, 0x4a6adc)


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


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--strings", default=os.path.join(ROOT, "re", "sevenseas", "gh", "strings.jsonl"))
    ap.add_argument("--exe", default=os.path.join(ROOT, "re", "sevenseas", "bin", "WinSS.exe"))
    ap.add_argument("--out", default=os.path.join(ROOT, "games", "sevenseas", "src", "GameTextTable.inc"))
    args = ap.parse_args()

    d = open(args.exe, "rb").read()
    strs = {}
    for line in open(args.strings):
        j = json.loads(line)
        if j["type"] == "string":
            strs[int(j["addr"], 16)] = j["value"]
    # the strings that the game's own functions refer to (s_..._<addr> and DAT_<addr> in the decompilation)
    decomp = os.path.join(os.path.dirname(args.strings), "decomp")
    rows = []
    known = set()
    for path in glob.glob(os.path.join(decomp, "*", "*.c")):
        if int(os.path.basename(path)[:8], 16) >= GAME_CODE_END:
            continue
        for m in re.finditer(r"s_\w+?_([0-9a-f]{8})|DAT_([0-9a-f]{8})", open(path, encoding="latin1").read()):
            va = int(m.group(1) or m.group(2), 16)
            if va in known or not (FIRST <= va < LAST):
                continue
            try:
                o = va_to_off(d, va)
            except ValueError:
                continue
            e = d.index(bytes(1), o)
            t = d[o:e]
            if (va in strs) or (1 <= len(t) < 200 and all(32 <= c < 127 or c in (9, 10, 13) for c in t)):
                rows.append(va)
                known.add(va)
    # the hint lines of the tutorial boxes: three tables of 15 strings, one for each tutorial step 0 to 14, 0x100 bytes apart (Board::Draw 0x406400)
    for base in (0x4a0150, 0x4a1050, 0x4a1f50):
        for k in range(15):
            va = base + k * 0x100
            if va in known:
                continue
            o = va_to_off(d, va)
            e = d.index(bytes(1), o)
            if 0 < e - o < 0xff and all(32 <= c < 127 for c in d[o:e]):
                rows.append(va)
                known.add(va)
    # the rank names (Screens.cpp HighScoreWidget::RankName; a table of pointers, so no code refers to them)
    for va in (0x4a3708, 0x4a36f4, 0x4a36e0, 0x4a36cc, 0x4a36b8, 0x4a36a4, 0x4a3684, 0x4a3668, 0x4a3650):
        if va not in known:
            rows.append(va)
            known.add(va)
    # the captions of the beginner's demo (Demo.cpp; two tables of pointers at 0x4a39f0 and 0x4a3a10)
    for va in (0x4a3b64, 0x4a3b48, 0x4a3b28, 0x4a3b08, 0x4a3ae0, 0x4a3ac8, 0x4a3ab0, 0x4a3a90, 0x4a3a78, 0x4a3a58, 0x4a3a34):
        if va not in known:
            rows.append(va)
            known.add(va)
    # the texts of the shareware and registration code, which the port leaves out (the user, 2026-11-07, 2026-11-08): the register
    # dialog and its prompts, the high-score prompts to register, the site and Internet-score texts, the title's registered and
    # unregistered lines, the play counts and the nag texts
    for va in EXCLUDE:
        if va in known:
            rows.remove(va)
            known.discard(va)
    rows.sort()
    out = ["// Generated by games/sevenseas/tools/make_gametext.py. Seven Seas Deluxe 1.1's hard-coded strings:",
           "// address in the game image, length and FNV-1a hash. The texts themselves are read from the",
           "// player's own WinSS.exe at start-up (GameText.h); none are in the repository."]
    for va in rows:
        o = va_to_off(d, va)
        e = d.index(b"\0", o)
        s = d[o:e]
        out.append("GAMETEXT(0x%08x, %d, 0x%08x)" % (va, len(s), fnv1a(s)))
    open(args.out, "w", newline="\n").write("\n".join(out) + "\n")
    print("wrote %d entries to %s" % (len(rows), args.out))


if __name__ == "__main__":
    sys.exit(main())
