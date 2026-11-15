"""Lists the PlaySample calls of a range of code with the sound each one plays (the decompiler drops that argument).

    python samples.py 43e4d0-43e8a0 ...      (addresses in hex, an upper bound after the dash)

Each line is the call's address and the resource global loaded for it ("SOUND_..."), or "?" when it is not a plain global.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import asm

ROOT = os.path.dirname(os.path.abspath(__file__))
NAMES = {}
for line in open(os.path.join(ROOT, "..", "docs", "resglobals.txt"), encoding="utf-8"):
    parts = line.split()
    if len(parts) >= 2:
        NAMES[int(parts[0], 16)] = parts[1]


def main():
    for argument in sys.argv[1:]:
        low, high = (int(x, 16) for x in argument.split("-"))
        last = {}
        for address, text in asm.disassemble(low, high):
            match = re.match(r"mov (e\w\w), dword ptr \[(0x[0-9a-f]+)\]", text)
            if match:
                last[match.group(1)] = int(match.group(2), 16)
            match = re.match(r"push dword ptr \[(0x[0-9a-f]+)\]", text)
            if match:
                last["push"] = int(match.group(1), 16)
            match = re.match(r"call dword ptr \[(e\w\w) \+ 0xc4\]", text)
            if match:
                name = "?"
                for key in ("push", "ecx", "edx", "eax", "edi", "esi", "ebx", "ebp"):
                    if key in last and last[key] in NAMES:
                        name = NAMES[last[key]]
                        break
                print("%x PlaySample(%s)" % (address, name))
                last = {}


if __name__ == "__main__":
    main()
