#!/usr/bin/env python3
"""Names the export's constructors and destructors by the vtable they store: a function that stores `&vftable_Sexy_Cubby`
(the class's own, first vtable) is Sexy::Cubby's constructor or destructor. Writes games/hammerheads/docs/autonames.txt in
the format of symbols.txt, "<address> <Class>::ctor_or_dtor", for the functions the export has no name for; decread.py
prints these names where symbols.txt gives none.

    python games/hammerheads/tools/autonames.py
"""
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decread

STORE = re.compile(r"= &vftable_([A-Za-z0-9_]+?)(?:_sub\d+)?;")
FRAMEWORK_BELOW = 0x469000


def class_name(vtable_symbol):
    if vtable_symbol.startswith("Sexy_"):
        return "Sexy::" + vtable_symbol[len("Sexy_"):]
    return vtable_symbol


def main():
    out = os.path.join(decread.ROOT, "games", "hammerheads", "docs", "autonames.txt")
    rtti = json.load(open(os.path.join(decread.ROOT, "re", "hammerheads", "rtti.classes.json")))
    names = {}
    for address, text in decread.load_functions().items():
        if address >= FRAMEWORK_BELOW:
            continue
        header = text.split("\n", 1)[0]
        if "Global::FUN_" not in header:
            continue
        stored = []
        for symbol in STORE.findall(text):
            name = class_name(symbol)
            if name in rtti and name not in stored:
                stored.append(name)
        if stored:
            # a base class's constructor inlined into the function stores its vtable too: the class is the most derived one
            best = max(stored, key=lambda c: (len(set(rtti[c]["bases"]) & set(stored)), stored.index(c)))
            names[address] = best + "::ctor_or_dtor"
    with open(out, "w", encoding="utf-8") as f:
        for address in sorted(names):
            f.write("%08x %s\n" % (address, names[address]))
    print(len(names), "constructors and destructors ->", out)


if __name__ == "__main__":
    main()
