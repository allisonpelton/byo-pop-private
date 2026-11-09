#!/usr/bin/env python3
"""Generate games/typershark/docs/symbols/20_game.jsonl: the names of the game's own functions, taken from the address comments
the translation carries.

Every function of the port that translates one of the exe's says so in a comment ("// 0x40e1a0" above its definition, or after
its declaration in the header: "void CloseHelp(); // 0x432c20"). This reads them back, so Ghidra's export can be given the
port's names (tools/ghidra_headless.sh apply ... games/typershark/docs/symbols/*.jsonl) and the next reader of the decompilation
finds Board::Undo where it was FUN_00403fb0.

    python games/typershark/tools/make_symbols.py [--out games/typershark/docs/symbols/20_game.jsonl]
"""
import argparse
import glob
import json
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.normpath(os.path.join(HERE, "..", "src"))
ADDR = re.compile(r"//.*?\b0x([0-9a-fA-F]{6})\b")
DEFINITION = re.compile(r"^[\w:<>\*&,\s~]*?\b(\w+)::(~?\w+)\s*\(")
DECLARATION = re.compile(r"\b(~?\w+)\s*\([^;{]*\)\s*(?:const\s*)?(?:override\s*)?(?:=\s*0\s*)?;\s*(//.*)$")
CLASS = re.compile(r"^(?:class|struct)\s+(\w+)\b(?!.*;\s*$)")


def from_header(path, symbols):
    cls = None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = CLASS.match(line)
        if m:
            cls = m.group(1)
            continue
        m = DECLARATION.search(line)
        if not m or cls is None:
            continue
        a = ADDR.search(m.group(2))
        if a:
            symbols.setdefault(a.group(1).lower().rjust(8, "0"), "%s::%s" % (cls, m.group(1)))


def from_source(path, symbols):
    pending = None
    for line in open(path, encoding="utf-8", errors="replace"):
        s = line.rstrip("\n")
        if s.startswith("//"):
            a = ADDR.search(s)
            if a:
                pending = a.group(1).lower().rjust(8, "0")
            continue
        if pending is not None:
            m = DEFINITION.match(s)
            if m:
                symbols.setdefault(pending, "%s::%s" % (m.group(1), m.group(2)))
            if s.strip() and not s.startswith("//"):
                pending = None


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--out", default=os.path.join(HERE, "..", "docs", "symbols", "20_game.jsonl"))
    args = p.parse_args()
    symbols = {}
    for path in sorted(glob.glob(os.path.join(SRC, "*.h"))):
        from_header(path, symbols)
    for path in sorted(glob.glob(os.path.join(SRC, "*.cpp"))):
        from_source(path, symbols)
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", newline="\n") as f:
        for addr in sorted(symbols):
            f.write(json.dumps({"addr": addr, "name": symbols[addr]}) + "\n")
    print("%d symbols -> %s" % (len(symbols), args.out))


if __name__ == "__main__":
    main()
