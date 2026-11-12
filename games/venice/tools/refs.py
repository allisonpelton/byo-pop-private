#!/usr/bin/env python3
"""Lists every place in the exe that refers to an address: absolute 4-byte operands, and call or jump targets.

    python games/venice/tools/refs.py 58cbb0 4319c0 ...
Each hit is printed with the Ghidra function that contains it.
"""
import json
import os
import struct
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pe  # noqa: E402

IMAGE_FILE = pe.Image()
IMAGE = IMAGE_FILE.data


def main():
    functions = sorted(
        (int(r["addr"], 16), r["size"], r["name"])
        for r in (json.loads(l) for l in open(os.path.join(ROOT, "re", "venice", "gh", "functions.jsonl"), encoding="utf-8"))
        if not r["external"])

    def container(va):
        best = None
        for start, size, name in functions:
            if start <= va:
                best = (start, size, name)
            else:
                break
        return "%08x" % best[0] if best else "?"

    for argument in sys.argv[1:]:
        target = int(argument, 16)
        pattern = struct.pack("<I", target)
        position = 0
        while True:
            found = IMAGE.find(pattern, position)
            if found < 0:
                break
            position = found + 1
            va = IMAGE_FILE.virtual_address(found)
            print("%08x  abs   %08x in %s" % (target, va or 0, container(va) if va and va < 0x585000 else "data"))
        text = IMAGE_FILE.bytes(0x401000, 0x183596)
        for offset in range(len(text) - 5):
            if text[offset] in (0xE8, 0xE9):
                relative = struct.unpack_from("<i", text, offset + 1)[0]
                if (0x401000 + offset + 5 + relative) & 0xFFFFFFFF == target:
                    print("%08x  call  %08x in %s" % (target, 0x401000 + offset, container(0x401000 + offset)))


if __name__ == "__main__":
    main()
