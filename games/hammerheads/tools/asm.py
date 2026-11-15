#!/usr/bin/env python3
"""Disassembles functions of the original exe (llvm-objdump, Intel syntax), for the places the decompiler gets wrong: the
arguments of a variadic call (FUN_004ba940, the sprintf that returns a std::string), values passed on the stack, and
x87 code. Calls and string addresses are named from docs/symbols.txt and the exe's strings.

    python games/hammerheads/tools/asm.py 421480              # the whole function
    python games/hammerheads/tools/asm.py 421480 +40          # 40 instructions from there
    python games/hammerheads/tools/asm.py 421480 --calls      # only the call sites, each with the pushes before it
"""
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decread

OBJDUMP = None
for folder in sorted(os.listdir("E:/retools")) if os.path.isdir("E:/retools") else []:
    candidate = os.path.join("E:/retools", folder, "bin", "llvm-objdump.exe")
    if folder.startswith("llvm-mingw") and os.path.exists(candidate):
        OBJDUMP = candidate
EXE = os.path.join(decread.ROOT, "re", "hammerheads", "bin", "HammerHeadsGame.exe")
LINE = re.compile(r"^\s+([0-9a-f]+):\s+(.*)$")


def sizes():
    result = {}
    for line in open(decread.FUNCTIONS, encoding="utf-8"):
        record = json.loads(line)
        result[int(record["addr"], 16)] = record["size"]
    return result


def disassemble(start, stop):
    out = subprocess.run([OBJDUMP, "-d", "--no-show-raw-insn", "--x86-asm-syntax=intel", "--start-address=0x%x" % start,
                          "--stop-address=0x%x" % stop, EXE], capture_output=True, text=True).stdout
    lines = []
    for text in out.splitlines():
        match = LINE.match(text)
        if match:
            lines.append((int(match.group(1), 16), re.sub(r" <\.text\+0x[0-9a-f]+>", "", match.group(2).replace("\t", " "))))
    return lines


def annotate(text, names):
    def replace(match):
        value = int(match.group(0), 16)
        if value in names:
            return "%s<%s>" % (match.group(0), names[value])
        string = decread.string_at(value) if 0x551000 <= value < 0x590000 else None
        if string:
            return '%s<"%s">' % (match.group(0), string[:60].replace("\n", "\\n"))
        return match.group(0)

    return re.sub(r"\b0x[0-9a-f]{6}\b", replace, text)


def main():
    arguments = sys.argv[1:]
    if not arguments:
        sys.exit(__doc__)
    only_calls = "--calls" in arguments
    arguments = [a for a in arguments if a != "--calls"]
    start = int(arguments[0], 16)
    names = {int(address, 16): name for address, name in decread.load_symbols().items()}
    if len(arguments) > 1 and arguments[1].startswith("+"):
        lines = disassemble(start, start + int(arguments[1][1:]) * 8)[:int(arguments[1][1:])]
    else:
        lines = disassemble(start, start + sizes()[start])
    pending = []
    for address, text in lines:
        text = annotate(text, names)
        if only_calls:
            if text.startswith("push"):
                pending.append(text)
            elif text.startswith("call"):
                print("%x: %s   <- %s" % (address, text, "; ".join(pending[-8:])))
                pending = []
            elif text.startswith(("ret", "j")):
                pending = []
        else:
            print("%x: %s" % (address, text))


if __name__ == "__main__":
    main()
