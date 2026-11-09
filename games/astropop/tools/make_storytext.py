#!/usr/bin/env python3
"""Write games/astropop/src/StoryText.inc: where the texts of the story screens are in the game's image.

The story of the four pilots (Board::BuildStoryText, 0x47a940) is a long list of strings pushed into one vector per page
of each story. This reads the export's decompile of that function and writes, for every push in the order of the
code, a row STORYLINE(pilot, story, page, address) with the address of the string in the game image (0 for the empty
string); the texts themselves are never copied (they are read from the player's own exe, GameText.h).

    python games/astropop/tools/make_storytext.py [--out games/astropop/src/StoryText.inc]

The titles of the stories (static initialisers at 0x55a931 .. 0x55b01f) and the texts of the finale (0x55a6e7 ..
0x55a8f7) are listed in the same file; their addresses were read from those initialisers (the instructions that load
the global string's address and the address of its literal).
"""
import argparse
import os
import re
import subprocess
import sys

# statements of the pilot's branch in the decompile (line numbers of the decompile that decread.py prints)
EMPTY = 0x55ddb0

# the titles: (pilot, number of the story from 1) -> address of the literal; the global arrays are at 0x59dd58 (pilot 0),
# 0x59df38 (pilot 1), 0x59e018 (pilot 2) and 0x59de58 (pilot 3), 0x1c bytes a string
TITLES = {
    0: [0x56a840, 0x56a830, 0x56a824, 0x56a818, 0x56a808, 0x56a7f8, 0x56a7e4, 0x56a7d0, 0x56a7bc],
    3: [0x56a8d0, 0x56a8c0, 0x56a8ac, 0x56a89c, 0x56a88c, 0x56a878, 0x56a864, 0x56a854],
    1: [0x56a94c, 0x56a934, 0x56a928, 0x56a914, 0x56a904, 0x56a8f8, 0x56a8ec, 0x56a8e0],
    2: [0x56a9d0, 0x56a9c0, 0x56a9b4, 0x56a9a4, 0x56a990, 0x56a980, 0x56a96c, 0x56a960],
}
# the texts of the four scenes of the finale, three each (the globals at 0x59dc08 .. 0x59dd3c, 0x1c bytes a string)
FINALE = [
    [0x56a308, 0x56a358, 0x56a3a0],
    [0x56a428, 0x56a504, 0x56a510],
    [0x56a550, 0x56a650, 0x56a65c],
    [0x56a690, 0x56a780, 0x56a798],
]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default="games/astropop/src/StoryText.inc")
    args = ap.parse_args()

    here = os.path.dirname(os.path.abspath(__file__))
    text = subprocess.run([sys.executable, os.path.join(here, "decread.py"), "47a940"], capture_output=True,
                          text=True, check=True).stdout.split("\n")

    # the start of each pilot's branch and of the loop after them
    starts = {}
    for i, l in enumerate(text):
        s = l.strip()
        if s == "if (iVar7 == 0) {" and 0 not in starts:
            starts[0] = i
        elif s == "if (iVar7 == 1) {" and 1 not in starts:
            starts[1] = i
        elif s == "else if (iVar7 == 2) {" and 2 not in starts:
            starts[2] = i
        elif s == "if (iVar7 != 3) goto LAB_0047ef26;" and 3 not in starts:
            starts[3] = i
        elif s == "LAB_0047ef26:":
            end = i
    order = sorted(starts.items(), key=lambda kv: kv[1])
    bounds = {}
    for k, (pilot, at) in enumerate(order):
        bounds[pilot] = (at, order[k + 1][1] if k + 1 < len(order) else end)

    # statements end at ';', '{' or '}'
    stmts = []
    cur = ""
    cur_start = 0
    for i, l in enumerate(text):
        s = l.strip()
        if not cur:
            cur_start = i
        cur += (" " if cur else "") + s
        if s.endswith(";") or s.endswith("{") or s.endswith("}") or s == "":
            stmts.append((cur_start, re.sub(r"\s+;$", ";", cur)))
            cur = ""

    rows = []
    var = {}
    counts = {}
    for n, st in stmts:
        pilot = None
        for p, (a, b) in bounds.items():
            if a <= n < b:
                pilot = p
        if pilot is None:
            continue
        m = re.match(r'^(local_\w+) = STR\("(.*)"@([0-9a-f]+)\);$', st)
        if m:
            var[m.group(1)] = int(m.group(3), 16)
            continue
        m = re.match(r'^std::string::string_cstr\s*\((local_\w+),\s*"(.*)"@([0-9a-f]+)\s*\);$', st)
        if m:
            var[m.group(1)] = int(m.group(3), 16)
            continue
        m = re.match(r'^std::string::(?:string_cstr|assign_ptr_len)\s*\((local_\w+),\s*&S""@55ddb0\b', st)
        if m:
            var[m.group(1)] = EMPTY
            continue
        m = re.match(r'^std::string::assign_ptr_len\s*\((local_\w+),\s*"(.*)"@([0-9a-f]+)\s*,', st)
        if m:
            var[m.group(1)] = int(m.group(3), 16)
            continue
        m = re.match(r"^std::vector<std_string>::push_back\s*\((local_[12]c)(?: \+ 0x([0-9a-f]+))?,\s*(local_\w+)\s*\);$", st)
        if m:
            page = 0 if m.group(1) == "local_1c" else 1
            story = (int(m.group(2), 16) if m.group(2) else 0) // 0x10
            if m.group(3) not in var:
                raise SystemExit("string not set before its push at line %d: %s" % (n, st))
            va = var[m.group(3)]
            rows.append((pilot, story, page, 0 if va == EMPTY else va))
            counts[pilot] = counts.get(pilot, 0) + 1
            continue
        if "push_back" in st:
            raise SystemExit("unparsed push at line %d: %s" % (n, st))

    total = sum(counts.values())
    if total != 298:
        raise SystemExit("expected 298 strings, found %d" % total)

    out = ["// Generated by games/astropop/tools/make_storytext.py. Where the strings of the story screens are in the",
           "// game image (addresses only, 0 is the empty string); the texts are read from the player's own exe (GameText.h).",
           "// STORYLINE(pilot, story from 0, page 0 = A or 1 = B, address): the lines of a page, in the order the game adds them",
           "// STORYTITLE(pilot, story from 1, address): the name of the story",
           "// FINALETEXT(scene from 1, text from 0, address): the three texts of a scene of the finale"]
    for r in rows:
        out.append("STORYLINE(%d, %d, %d, 0x%08x)" % r)
    for pilot in sorted(TITLES):
        for k, va in enumerate(TITLES[pilot]):
            out.append("STORYTITLE(%d, %d, 0x%08x)" % (pilot, k + 1, va))
    for scene, vas in enumerate(FINALE):
        for k, va in enumerate(vas):
            out.append("FINALETEXT(%d, %d, 0x%08x)" % (scene + 1, k, va))
    open(args.out, "w", newline="\n").write("\n".join(out) + "\n")
    print("wrote %d story lines, %d titles and %d finale texts to %s" % (
        total, sum(len(v) for v in TITLES.values()), sum(len(v) for v in FINALE), args.out))


if __name__ == "__main__":
    sys.exit(main())
