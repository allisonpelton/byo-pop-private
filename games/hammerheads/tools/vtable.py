#!/usr/bin/env python3
"""Prints a class's primary vtable from the RTTI export: each slot's byte offset, the function's address and, where it
differs from the base's slot, the mark "*". The framework tables of games/bigmoney/docs/FRAMEWORK_12.md name a slot.

    python games/hammerheads/tools/vtable.py Sexy::Cubby                     # every slot
    python games/hammerheads/tools/vtable.py Sexy::Cubby Sexy::SimpleObject  # only the slots that differ from the base's
    python games/hammerheads/tools/vtable.py Sexy::Cubby --table Widget      # name the slots from another table
    python games/hammerheads/tools/vtable.py --list                          # every class, its vtable address and slot count
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decread

RTTI = os.path.join(decread.ROOT, "re", "hammerheads", "rtti.classes.json")


def primary(classes, name):
    for table in classes[name]["vtables"]:
        if table["offset"] == 0:
            return table
    return classes[name]["vtables"][0]


def main():
    arguments = sys.argv[1:]
    classes = json.load(open(RTTI))
    if not arguments:
        sys.exit(__doc__)
    if arguments[0] == "--list":
        for name, info in sorted(classes.items(), key=lambda item: int(primary(classes, item[0])["va"], 16) if item[1]["vtables"] else 0):
            if info["vtables"]:
                table = primary(classes, name)
                print("%s %-40s %3d slots  bases: %s" % (table["va"], name, len(table["slots"]), ", ".join(info["bases"][:2])))
        return
    table_name = None
    if "--table" in arguments:
        at = arguments.index("--table")
        table_name = arguments[at + 1]
        del arguments[at:at + 2]
    name = arguments[0]
    base = arguments[1] if len(arguments) > 1 else None
    slots = primary(classes, name)["slots"]
    base_slots = primary(classes, base)["slots"] if base else []
    names = decread.slot_tables().get(table_name, {}) if table_name else {}
    for index, address in enumerate(slots):
        changed = index >= len(base_slots) or base_slots[index] != address
        if base and not changed:
            continue
        print("+%03x %s %s %s" % (index * 4, address, "*" if changed else " ", names.get(index * 4, "")))


if __name__ == "__main__":
    main()
