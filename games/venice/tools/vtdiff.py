#!/usr/bin/env python3
"""Prints, for each class of re/venice/classes.txt, the vtable slots that differ from the base scene object's.

    python games/venice/tools/vtdiff.py [--out file]

A slot is shown as <slot>:<address>(<size>) with the function's size in bytes from re/venice/game_funcs.txt. The classes that
store the base object's vtable (0x58a9d8) are the reference. A vtable longer than 36 slots shows the slots past the base's end.
"""
import argparse
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
CLASSES = os.path.join(ROOT, "re", "venice", "classes.txt")
FUNCS = os.path.join(ROOT, "re", "venice", "game_funcs.txt")
BASE_VTABLE = 0x58A9D8


def read_sizes():
    sizes = {}
    for line in open(FUNCS, encoding="latin-1"):
        parts = line.split()
        if len(parts) >= 2:
            sizes[int(parts[0], 16)] = int(parts[1])
    return sizes


def read_classes():
    classes = []
    lines = open(CLASSES).read().splitlines()
    i = 0
    while i < len(lines):
        match = re.match(r"vtable 0x([0-9a-f]+) slots (\d+) stored by(.*)", lines[i])
        if match:
            slots = [int(x, 16) for x in lines[i + 1].split()]
            stored = [int(x, 16) for x in match.group(3).split()]
            classes.append((int(match.group(1), 16), int(match.group(2)), stored, slots))
            i += 2
        else:
            i += 1
    return classes


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out")
    args = parser.parse_args()
    sizes = read_sizes()
    classes = read_classes()
    base = next(c[3] for c in classes if c[0] == BASE_VTABLE)
    out = []
    for vtable, count, stored, slots in classes:
        out.append("class vtable 0x%x slots %d stored by %s" % (vtable, count, " ".join("0x%x" % s for s in stored)))
        # a run of several vtables back to back is split by the base's length (36) wherever the first slot repeats a dtor
        position = 0
        while position < len(slots):
            chunk = slots[position:position + 36] if len(slots) > 41 else slots
            shown = []
            for index, address in enumerate(chunk):
                if index >= len(base) or address != base[index]:
                    shown.append("%d:%x(%d)" % (index, address, sizes.get(address, 0)))
            out.append("  +%d: %s" % (position, " ".join(shown)))
            position += 36 if len(slots) > 41 else len(slots)
    text = "\n".join(out) + "\n"
    if args.out:
        open(args.out, "w").write(text)
    else:
        print(text)


if __name__ == "__main__":
    main()
