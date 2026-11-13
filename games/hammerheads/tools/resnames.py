#!/usr/bin/env python3
"""Maps the original's resource globals to their ids. The generated extraction code of the exe (0x426710, 0x428fd0,
0x42c1f0, 0x42c790, 0x42cb90) looks up "IMAGE_X", "FONT_X" or "SOUND_X" and stores the result in a global (0x597964 and so
on). The scan reads the machine code (Ghidra's decompiler times out on 0x428fd0): a `push <address of an id string>` names
the next `mov [<global>], eax`. Writes "<address> <id>" lines in the format of games/hammerheads/docs/symbols.txt, which
decread.py puts in place of the DAT_ names.

    python games/hammerheads/tools/resnames.py [--out games/hammerheads/docs/resglobals.txt]
"""
import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pe

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
EXTRACTORS = [0x426710, 0x428fd0, 0x42c1f0, 0x42c790, 0x42cb90]
PUSH_IMM32 = 0x68
MOV_MOFFS32_EAX = 0xA3
ID_STRING = re.compile(rb"((?:IMAGE|FONT|SOUND)_[A-Z0-9_]+)\0")


def main():
    out = os.path.join(ROOT, "games", "hammerheads", "docs", "resglobals.txt")
    if "--out" in sys.argv:
        out = sys.argv[sys.argv.index("--out") + 1]
    sizes = {}
    for line in open(os.path.join(ROOT, "re", "hammerheads", "ghidra", "functions.jsonl"), encoding="utf-8"):
        record = json.loads(line)
        sizes[int(record["addr"], 16)] = record["size"]
    image = pe.Image()
    found = {}
    for start in EXTRACTORS:
        code = image.bytes(start, sizes[start])
        last = None
        i = 0
        while i < len(code) - 5:
            if code[i] == PUSH_IMM32:
                value = struct.unpack_from("<I", code, i + 1)[0]
                text = image.bytes(value, 48) if image.section(value) == ".rdata" else b""
                match = ID_STRING.match(text)
                if match:
                    last = match.group(1).decode()
                i += 5
            elif code[i] == MOV_MOFFS32_EAX:
                value = struct.unpack_from("<I", code, i + 1)[0]
                if image.section(value) == ".data" and last:
                    found[value] = last
                    last = None
                i += 5
            else:
                i += 1
    with open(out, "w", encoding="utf-8") as f:
        for address in sorted(found):
            f.write("%08x %s\n" % (address, found[address]))
    print(len(found), "resource globals ->", out)


if __name__ == "__main__":
    main()
