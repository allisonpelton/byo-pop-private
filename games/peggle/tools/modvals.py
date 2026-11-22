#!/usr/bin/env python3
"""Lists the tweakable constants (ModValInt / ModValDouble / ModValFloat / ModValString) one function reads, in the order the
code executes them, with the file and line the tag names and the default the shipped game uses. The decompiler drops some of
these defaults; the assembly pushes them in plain sight.

    python -I games/peggle/tools/modvals.py <function address> [<function address> ...]

Each line: address of the call, kind, tag ("file.cpp<line>,<column>"), default. Needs llvm-objdump (E:/retools/llvm-mingw).
"""
import json
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pe

ROOT = pe.ROOT
OBJDUMP = "E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe"
KINDS = {0x523c70: "int", 0x523ca0: "double", 0x523cd0: "float", 0x523d00: "string"}


def function_size(address):
    with open(os.path.join(ROOT, "re", "peggle", "deluxe", "gh", "functions.jsonl")) as f:
        for line in f:
            j = json.loads(line)
            if int(j["addr"], 16) == address:
                return j["size"]
    raise SystemExit("function %x not found" % address)


def c_string(image, address):
    out = bytearray()
    while True:
        b = image.bytes(address, 1)[0]
        if b == 0:
            return out.decode("latin-1")
        out += bytes([b])
        address += 1


def main():
    image = pe.Image()
    for text in sys.argv[1:]:
        start = int(text, 16)
        size = function_size(start)
        dump = subprocess.run(
            [OBJDUMP, "-d", "--x86-asm-syntax=intel", "--start-address=0x%x" % start, "--stop-address=0x%x" % (start + size),
             os.path.join(ROOT, "re", "peggle", "deluxe", "bin", "PeggleGame.exe")],
            capture_output=True, text=True).stdout
        instructions = []
        for line in dump.splitlines():
            m = re.match(r"\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*\t(.*)$", line)
            if m:
                instructions.append((int(m.group(1), 16), m.group(2).strip()))
        print("// function %x" % start)
        for i, (address, text_) in enumerate(instructions):
            m = re.match(r"call\s+0x([0-9a-f]+)", text_)
            if not m or int(m.group(1), 16) not in KINDS:
                continue
            kind = KINDS[int(m.group(1), 16)]
            # the pushes before the call, nearest first: 0, the tag, the default
            pushes = []
            j = i - 1
            while j >= 0 and len(pushes) < 3 and i - j < 14:
                mm = re.match(r"push\s+(-?0x[0-9a-f]+)$", instructions[j][1])
                if mm:
                    pushes.append(int(mm.group(1), 16))
                j -= 1
            tag = default = None
            # the arguments are pushed last first: the default (an int, or the address of a string), the tag's address, then 0
            if len(pushes) >= 3:
                try:
                    tag = re.sub(r"^(SEXY_SEXYMODVAL)?c:.*\\", "", c_string(image, pushes[1] & 0xffffffff))
                except Exception:
                    tag = "?"
                if kind == "int":
                    default = pushes[2]
                    if default >= 0x80000000:
                        default -= 1 << 32
                elif kind == "string":
                    try:
                        default = repr(c_string(image, pushes[2] & 0xffffffff))
                    except Exception:
                        default = "?"
                else:
                    default = "(an x87 constant: see the code)"
            print("%08x %-6s %-44s default %s" % (address, kind, tag, default))


main()
