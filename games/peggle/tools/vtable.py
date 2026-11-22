#!/usr/bin/env python3
"""Prints a class's vtable from the Ghidra export, each slot with the Mac build's name for its function when there is one:

    python games/peggle/tools/vtable.py Poly Ball ...     # slot, offset, address, name (a name from the Mac pairing, or FUN_)

Slots that two classes share (an inherited function) show the same address."""
import json
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
GH = os.path.join(ROOT, "re", "peggle", "deluxe", "gh", "vtables.jsonl")
MATCH = os.path.join(ROOT, "re", "peggle", "deluxe", "macmatch", "peggle.match.jsonl")


def main():
    names = {}
    for line in open(MATCH):
        d = json.loads(line)
        names[d["a"]] = d["name"].replace("Sexy::", "")
    vtables = {}
    for line in open(GH):
        d = json.loads(line)
        if d["symbol"].endswith("::vftable"):
            vtables[d["class"].replace("Sexy::", "")] = d
    for arg in sys.argv[1:]:
        d = vtables.get(arg)
        print("== %s (vtable %s)" % (arg, d["addr"] if d else "not found"))
        if not d:
            continue
        for s in d["slots"]:
            print("%3d +0x%03x %s  %s" % (s["i"], s["i"] * 4, s["addr"], names.get(s["addr"], "")))


if __name__ == "__main__":
    main()
