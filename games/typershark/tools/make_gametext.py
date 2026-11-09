#!/usr/bin/env python3
"""Generate games/typershark/src/GameTextTable.inc: where each of Typer Shark's hard-coded texts is in the exe.

The port carries no game text (games/typershark/src/GameText.h): it reads every text from the player's own WinTS.exe by
address. This finds those addresses. The input is Ghidra's string export of the game image (re/typershark/gh/strings.jsonl,
git-ignored: it holds game text); the output holds only addresses, lengths and hashes. Every string that a function of
the game's own code or of the Nuclide application layer refers to is listed, so a translation can name any of them as
GT(0xADDR), except the ones in EXCLUDE (strings that only registration, trial, update-check or outbound-link code used).

    python games/typershark/tools/make_gametext.py [--strings re/typershark/gh/strings.jsonl] [--exe re/typershark/bin/TyperSharkGame.exe]
        [--out games/typershark/src/GameTextTable.inc]
"""
import argparse
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
# The game's own code (0x401000-0x4623f0)
CODE_RANGES = ((0x401000, 0x4623f0), (0x527890, 0x527cd0))   # the second: the static initializer of the default high-score names
# Extra texts that no function refers to by a plain address: the tables of string pointers in the data section (the tutorial
# sidebar texts, the helper's speech bubbles, the help pages ...) lie between these addresses
EXTRA = []
POINTER_TABLES = ((0x55f000, 0x55f4f0), (0x55f970, 0x55f988), (0x560078, 0x560128))
# the read-only data where the strings are
IMMEDIATE_STRINGS = (0x529000, 0x55f000)

# Strings that only registration, trial, update-check or outbound-link code used. The port has none of that code (the user's
# rule, CLAUDE.md "Order of the work": removed, not disabled), so its table does not list them. Each entry: address -> the
# original's function that used it (and what for); the texts themselves are the game's and are not repeated here.
EXCLUDE = {
    # the update check (0x41b030, 0x41b230; the loading thread started it)
    0x0052b3bc: "0x41b030 update-check dialog: header",
    0x0052b3c8: "0x41b030 update-check dialog: question text",
    0x0052b45c: "0x41b230 update-check address: part of the query string",
    0x0052b468: "0x41b230 update-check address: part of the query string",
    0x0052b470: "0x41b230 update-check address (a web page)",
    0x0052b4a4: "0x41b230 update-check dialog: header",
    # the register dialogs (0x41c1c0 the trial-limit dialog, 0x414020 the register dialog, class RegisterDialog 0x438aa0..0x43f640)
    0x0052b4d4: "0x41c1c0 trial-limit dialog: Cancel button (this copy of the label is used by no other function)",
    0x0052b4e8: "0x41c1c0 trial-limit dialog: header",
    0x0052b4f8: "0x41c1c0 trial-limit dialog: register button",
    0x0052ed50: "0x43f640 RegisterDialog: message for a wrong registration code",
    # the unregistered variants of texts whose registered variants the port uses
    0x0052b528: "0x41cc60 ShowLevelIntro: abyss introduction of an unregistered copy (the registered one is 0x52b5e8)",
    0x0052d078: "0x431630 ButtonDepress case 0xc9: end-game confirmation of an unregistered copy (the registered one is 0x52d0f0)",
    # the trial limits (0x41d870, 0x41d9d0, 0x41da80, 0x41d910 and their callers in 0x431630, 0x4495c0)
    0x0052c360: "0x41d870, 0x41d910, 0x41d9d0, 0x41da80, 0x431630, 0x4495c0: the registry value MaxTime (the trial's minutes)",
    0x0052d288: "0x431630 ButtonDepress: message when the trial's minutes are used up",
    0x0052d2f0: "0x431630 ButtonDepress: message when the trial's plays are used up",
    0x0052c854: "0x428500 Init: the registry value MaxPlays",
    0x00530b8c: "0x4544b0 Board constructor: the registry value MaxUnregAbyssDepth",
    0x00531648: "0x45c140 Board::Update: message when an unregistered abyss game reaches its depth limit",
    0x00531970: "0x460900 TypingTutor::ButtonDepress: the registry value MaxUnregTutorialLessons",
    0x00531928: "0x460900 TypingTutor::ButtonDepress: register prompt for the lessons past the first",
    # the register picture and the Options dialog's Register and partner-link buttons
    0x0052c828: "0x428500 Init: message for the register picture failing to load",
    0x0052c844: "0x428500 Init: the register picture's file",
    0x0052e898: "0x43bf10 OptionsDialog constructor: partner-link button label (a link out of the game)",
    0x0052e8ac: "0x43bf10 OptionsDialog constructor: the property NumOptionsLinks",
    0x0052e8bc: "0x43bf10 OptionsDialog constructor: the property RegBtnText",
    0x0052e8c8: "0x43bf10 OptionsDialog constructor: Register button's default label",
    0x0052e8d4: "0x43bf10, 0x448e10, 0x4495c0: the property NoReg",
    0x0052ea98: "0x43cd90 OptionsDialog button handler: the partner link's address (a web page)",
    # the title screen's register button, stamps, minutes text and its continue button (never shown by a registered copy)
    0x0052fbb0: "0x448f40 TitleScreen: the trial stamp's file",
    0x0052fbc4: "0x448f40 TitleScreen: the registered stamp's file",
    0x0052fc10: "0x448f40 TitleScreen: the register button's over picture's file",
    0x0052fc2c: "0x448f40 TitleScreen: the register button's picture's file",
    0x0052fc94: "0x449400 TitleScreen: the register button's label",
    0x0052fcac: "0x4495c0 TitleScreen::Draw: the trial's minutes text (a word of it)",
    0x0052fcb8: "0x4495c0 TitleScreen::Draw: the trial's minutes text (plural)",
    0x0052fcc4: "0x4495c0 TitleScreen::Draw: the trial's minutes text (singular)",
    0x0052fbdc: "0x448f40 TitleScreen: the continue button's over picture's file (a hidden widget: removed)",
    0x0052fbf8: "0x448f40 TitleScreen: the continue button's picture's file (a hidden widget: removed)",
    0x0052fc7c: "0x449400 TitleScreen: the continue button's label (only measured)",
    # the high score screen's register prompt
    0x0052f948: "0x4470c0 HighScoreScreen::Update: register prompt",
}


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
    ap.add_argument("--strings", default=os.path.join(ROOT, "re", "typershark", "gh", "strings.jsonl"))
    ap.add_argument("--exe", default=os.path.join(ROOT, "re", "typershark", "bin", "TyperSharkGame.exe"))
    ap.add_argument("--out", default=os.path.join(ROOT, "games", "typershark", "src", "GameTextTable.inc"))
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
            # Ghidra leaves some short texts ("Chum") undefined: take a printable text the table points to
            if IMMEDIATE_STRINGS[0] <= v < IMMEDIATE_STRINGS[1]:
                try:
                    so = va_to_off(d, v)
                except ValueError:
                    continue
                t = d[so:d.find(b"\0", so)]
                if len(t) >= 2 and all(c in b"\r\n\t" or 0x20 <= c < 0x7f for c in t):
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
    stale = sorted(a for a in EXCLUDE if a not in rows)
    if stale:
        print("EXCLUDE has addresses that are not texts of the table: " + ", ".join("0x%08x" % a for a in stale))
    rows -= set(EXCLUDE)
    out = ["// Generated by games/typershark/tools/make_gametext.py. Typer Shark Deluxe 1.0's hard-coded strings:",
           "// address in the game image, length and FNV-1a hash. The texts themselves are read from the",
           "// player's own WinTS.exe at start-up (GameText.h); none are in the repository."]
    # the vftables share the read-only data with the texts: their first bytes can look like a short text
    vtables = set()
    vt_path = os.path.join(os.path.dirname(args.strings), "vtables.jsonl")
    if os.path.exists(vt_path):
        for line in open(vt_path, encoding="utf-8"):
            vtables.add(int(json.loads(line)["addr"], 16))
    n = 0
    for va in sorted(rows):
        if va in vtables:
            continue
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
