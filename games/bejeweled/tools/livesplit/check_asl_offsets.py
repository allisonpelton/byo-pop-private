#!/usr/bin/env python3
"""Checks that the auto splitter reads the block the game writes (games/bejeweled/docs/CODE_MODS_DESIGN.md
4.15, test 6).

  check_asl_offsets.py <script.asl> <layout> [<game exe>...]

  <script.asl>  games/bejeweled/tools/livesplit/BejeweledDeluxe-byopop.asl
  <layout>      the output of "lsprobe -dumplayout": the magic, the header and every field's offset and type,
                from games/bejeweled/src/LiveSplitBlock.h (offsetof, compiled)
  <game exe>    builds of the game: the magic must occur exactly once in the file, inside the raw data of its
                .data section (the block is constant-initialised, so nothing else spells it)

The script's magic, game id, layout version, block size, flag values and its field table ("name offset
type") must equal the layout's; every field the script reads must exist in the layout. Exit status 1 on any
difference.
"""

import re
import struct
import sys


def read_script(path):
    text = open(path, encoding="utf-8").read()
    script = {}
    m = re.search(r"vars\.Magic\s*=\s*new byte\[\]\s*\{([^}]*)\}", text)
    script["magic"] = bytes(int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(1))) if m else None
    for name in ("GameId", "LayoutVersion", "BlockSize", "FlagTimed", "FlagVanilla", "FlagReplay", "FlagGameOver"):
        m = re.search(r"vars\.%s\s*=\s*(0x[0-9A-Fa-f]+|\d+)L?;" % name, text)
        script[name] = int(m.group(1), 0) if m else None
    m = re.search(r"vars\.Fields\s*=\s*new string\[\]\s*\{(.*?)\};", text, re.S)
    script["fields"] = {}
    if m:
        for entry in re.findall(r'"([^"]+)"', m.group(1)):
            name, offset, kind = entry.split()
            script["fields"][name] = (int(offset, 16), kind)
    return script


def read_layout(path):
    layout = {"fields": {}, "flags": {}}
    for line in open(path, encoding="latin-1"):
        w = line.split()
        if not w:
            continue
        if w[0] == "magic":
            layout["magic"] = bytes(int(x, 16) for x in w[1:])
        elif w[0] in ("gameId", "size"):
            layout[w[0]] = int(w[1], 16)
        elif w[0] == "layoutVersion":
            layout[w[0]] = int(w[1])
        elif w[0] == "field":
            layout["fields"][w[1]] = (int(w[2], 16), w[3])
        elif w[0] == "flags":
            layout["flags"] = {k: int(v, 16) for k, v in (x.split("=") for x in w[1:])}
    return layout


def magic_in_exe(path, magic):
    data = open(path, "rb").read()
    hits = [m.start() for m in re.finditer(re.escape(magic), data)]
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    sections = struct.unpack_from("<H", data, pe + 6)[0]
    table = pe + 24 + struct.unpack_from("<H", data, pe + 20)[0]
    raw = None
    for i in range(sections):
        o = table + 40 * i
        if data[o:o + 8].rstrip(b"\0") == b".data":
            size, pointer = struct.unpack_from("<II", data, o + 16)
            raw = (pointer, pointer + size)
    return hits, raw


def main(argv):
    if len(argv) < 3:
        sys.stderr.write(__doc__)
        return 2
    script, layout = read_script(argv[1]), read_layout(argv[2])
    problems = []
    if script["magic"] != layout.get("magic"):
        problems.append("magic: script %s, game %s" % (script["magic"], layout.get("magic")))
    for name, key in (("GameId", "gameId"), ("LayoutVersion", "layoutVersion"), ("BlockSize", "size")):
        if script[name] != layout.get(key):
            problems.append("%s: script %s, game %s" % (name, script[name], layout.get(key)))
    for name, key in (("FlagTimed", "timed"), ("FlagVanilla", "vanilla"), ("FlagReplay", "replay"), ("FlagGameOver", "gameOver")):
        if script[name] != layout["flags"].get(key):
            problems.append("%s: script %s, game %s" % (name, script[name], layout["flags"].get(key)))
    if not script["fields"]:
        problems.append("no vars.Fields table in the script")
    for name, (offset, kind) in sorted(script["fields"].items(), key=lambda x: x[1][0]):
        if name not in layout["fields"]:
            problems.append("field %s: not in the game's layout" % name)
        elif layout["fields"][name] != (offset, kind):
            problems.append("field %s: script 0x%X %s, game 0x%X %s" % ((name, offset, kind) + layout["fields"][name]))
    unread = sorted(set(layout["fields"]) - set(script["fields"]))
    for exe in argv[3:]:
        hits, raw = magic_in_exe(exe, layout.get("magic", b"?"))
        if raw is None:
            problems.append("%s: no .data section" % exe)
        elif len(hits) != 1 or not raw[0] <= hits[0] < raw[1]:
            problems.append("%s: the magic at %s, .data's raw data at 0x%x..0x%x" % (exe, [hex(h) for h in hits], raw[0], raw[1]))
        else:
            print("%s: the magic once, at file offset 0x%x in .data (0x%x..0x%x)" % (exe, hits[0], raw[0], raw[1]))
    for p in problems:
        print("DIFF  " + p)
    print("%s: %d script fields match the game's layout 1 (magic, game id 0x%X, version %s, size 0x%X, flags); not read by"
          " the script: %s" % ("FAIL" if problems else "ok", len(script["fields"]) - len(problems), layout.get("gameId", 0),
                              layout.get("layoutVersion"), layout.get("size", 0), ", ".join(unread) or "-"))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
