#!/usr/bin/env python3
"""Prints functions of the Ghidra export by address.

    python games/hammerheads/tools/fn.py 401000 4282b0 ...      # each function's decompilation
    python games/hammerheads/tools/fn.py -r 401000 402000       # every function that starts in a range
    python games/hammerheads/tools/fn.py -s waiting             # the functions whose text holds a string
"""
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
ALL = os.path.join(ROOT, "re", "hammerheads", "ghidra", "all.c")
HEADER = re.compile(r"^// ([0-9a-f]{8})  (.*)$", re.M)


def load():
    text = open(ALL, encoding="utf-8", errors="replace").read()
    marks = list(HEADER.finditer(text))
    result = {}
    for index, mark in enumerate(marks):
        end = marks[index + 1].start() if index + 1 < len(marks) else len(text)
        result[int(mark.group(1), 16)] = text[mark.start():end].rstrip() + "\n"
    return result


def main():
    functions = load()
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    if args[0] == "-r":
        low, high = int(args[1], 16), int(args[2], 16)
        for address in sorted(functions):
            if low <= address < high:
                print(functions[address])
    elif args[0] == "-s":
        for address in sorted(functions):
            if args[1] in functions[address]:
                print("%08x" % address, functions[address].splitlines()[0])
    else:
        for argument in args:
            address = int(argument, 16)
            print(functions.get(address, "// %08x not in the export\n" % address))


if __name__ == "__main__":
    main()
