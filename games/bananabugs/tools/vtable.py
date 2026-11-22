#!/usr/bin/env python3
"""Prints a class's vtable from the Ghidra export, each slot with our name for its function (games/bananabugs/docs/symbols.txt):

    python -I games/bananabugs/tools/vtable.py Board ObjBase ...   # slot, offset, address, name

Slots that two classes share (an inherited function) show the same address; a slot that differs from the first class
named after --base is marked with '*'.

    python -I games/bananabugs/tools/vtable.py --base ObjBase Board MainMenu
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decread

VTABLES = os.path.join(decread.ROOT, "re", "bananabugs", "game", "vtables.jsonl")


def main():
    arguments = sys.argv[1:]
    base = None
    if arguments[:1] == ["--base"]:
        base, arguments = arguments[1], arguments[2:]
    names = decread.load_symbols()
    vtables = {}
    for line in open(VTABLES):
        record = json.loads(line)
        if record["symbol"].endswith("::vftable") and record["slots"]:
            vtables.setdefault(record["class"].replace("Sexy::", ""), record)
    base_slots = [s["addr"] for s in vtables[base]["slots"]] if base else []
    for name in ([base] if base else []) + arguments:
        record = vtables.get(name)
        print("== %s (vtable %s)" % (name, record["addr"] if record else "not found"))
        if not record:
            continue
        for slot in record["slots"]:
            own = base and name != base and (slot["i"] >= len(base_slots) or base_slots[slot["i"]] != slot["addr"])
            print("%3d +0x%03x %s%s %s" % (slot["i"], slot["i"] * 4, slot["addr"], "*" if own else " ",
                                          names.get(slot["addr"], "")))


if __name__ == "__main__":
    main()
