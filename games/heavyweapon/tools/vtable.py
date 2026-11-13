#!/usr/bin/env python3
"""Prints a vtable of the Heavy Weapon image slot by slot, next to a framework base class's (from the RTTI dump), so the
slots a game class overrides stand out.

    python games/heavyweapon/tools/vtable.py 52b840 48 Sexy::Widget     # 48 slots against Widget's
    python games/heavyweapon/tools/vtable.py 52c888 48                  # the slots alone
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pe

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


def main():
    image = pe.Image()
    va, count = int(sys.argv[1], 16), int(sys.argv[2])
    base = []
    if len(sys.argv) > 3:
        classes = json.load(open(os.path.join(ROOT, "re", "heavyweapon", "rtti.classes.json")))
        base = [int(s, 16) for s in classes[sys.argv[3]]["vtables"][0]["slots"]]
    for index in range(count):
        slot = image.u32(va + 4 * index)
        mark = ""
        if index < len(base):
            mark = "  =base" if base[index] == slot else "  OWN"
        print("%2d +%03x  %08x%s" % (index, 4 * index, slot, mark))


if __name__ == "__main__":
    main()
