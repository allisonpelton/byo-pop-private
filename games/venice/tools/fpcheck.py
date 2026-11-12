#!/usr/bin/env python3
"""Tells which functions of a range do floating-point work the decompile hides: x87 instructions (arithmetic done in double
precision, as in the code around the CRT's sqrt, sin and cos) and double conversions (cvtss2sd).

    python games/venice/tools/fpcheck.py 412800-416560
Each line: address, size, number of x87 arithmetic instructions, number of cvtss2sd/cvtsd2ss."""
import json
import os
import re
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
EXE = os.path.join(ROOT, "re", "venice", "bin", "VeniceGame.exe")
OBJDUMP = "E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe"
X87 = re.compile(r"\t(fadd|fsub|fsubr|fmul|fdiv|fdivr|fild|fld|fst|fstp|fchs|fabs|fcomp|fcompi|fcom|fucom|fxch)\b")
DOUBLE = re.compile(r"\t(cvtss2sd|cvtsd2ss|addsd|subsd|mulsd|divsd|movsd)\b")


def main():
    low, high = (int(x, 16) for x in sys.argv[1].split("-"))
    records = []
    for line in open(os.path.join(ROOT, "re", "venice", "gh", "functions.jsonl"), encoding="utf-8"):
        record = json.loads(line)
        address = int(record["addr"], 16)
        if low <= address < high:
            records.append((address, record["size"]))
    for address, size in sorted(records):
        text = subprocess.run([OBJDUMP, "-d", "-M", "intel", "--no-show-raw-insn", "--start-address=0x%x" % address,
                               "--stop-address=0x%x" % (address + size), EXE], capture_output=True, text=True).stdout
        x87 = len(X87.findall(text))
        double = len(DOUBLE.findall(text))
        print("%08x %5d x87=%-3d double=%d" % (address, size, x87, double))


if __name__ == "__main__":
    main()
