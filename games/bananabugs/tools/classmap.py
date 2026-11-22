#!/usr/bin/env python3
"""Where each class's code is: the functions of the Ghidra export that store a class's vtable (its constructors and
destructors), as one line a class sorted by address, so the translation units between them can be read off.

    python -I games/bananabugs/tools/classmap.py
"""
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
ALL = os.path.join(ROOT, "re", "bananabugs", "game", "all.c")
HEADER = re.compile(r"^// ([0-9a-f]{8})  ")
STORE = re.compile(r"= (?:\(\w+\s*\*?\))?&?Sexy::(\w+)::vftable")

found = {}
current = None
with open(ALL, encoding="utf-8", errors="replace") as f:
    for line in f:
        m = HEADER.match(line)
        if m:
            current = int(m.group(1), 16)
            continue
        if current is None or current >= 0x476000:
            continue
        for name in STORE.findall(line):
            found.setdefault(name, set()).add(current)
for name, addresses in sorted(found.items(), key=lambda kv: min(kv[1])):
    print("%08x %-22s %s" % (min(addresses), name, " ".join("%x" % a for a in sorted(addresses))))
