#!/usr/bin/env python3
"""Lines up the vtable slots of Heavy Weapon's framework classes with Big Money's (SAF 1.2) and BookWorm's (the 2004
framework), through tools/funcmatch.py's function matches, and names each slot by Big Money's slot list
(games/bigmoney/docs/FRAMEWORK_12.md) or BookWorm's symbol files.

    python games/heavyweapon/tools/slotmap.py Sexy::Widget [Sexy::Dialog ...]
    python games/heavyweapon/tools/slotmap.py --all > re/heavyweapon/slots.txt

Each line: Heavy Weapon's slot (offset/index), its function, the slot or slots of Big Money's and BookWorm's vtable of
the same class that hold the matched function, and the name. A slot whose function is a folded stub (one body shared by
many slots) matches many slots; then the slot of the same index in the neighbouring rows decides, so read those by
their neighbours. The other exports are read from the main checkout's re/ folder.
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
MAIN = os.path.normpath(os.path.join(ROOT, "..", "..", "..")) if ".claude" in ROOT else ROOT


def load_classes(path):
    return json.load(open(path, encoding="utf-8"))


def load_match(name):
    result = {}
    for line in open(os.path.join(ROOT, "re", "heavyweapon", "match", name), encoding="utf-8"):
        row = json.loads(line)
        result[int(row["a"], 16)] = int(row["b"], 16)
    return result


def bigmoney_slot_names():
    """{class: {slot: 'Name(args)'}} from FRAMEWORK_12.md's slot blocks."""
    names = {}
    current = None
    for line in open(os.path.join(ROOT, "games", "bigmoney", "docs", "FRAMEWORK_12.md"), encoding="utf-8"):
        heading = re.match(r"^### (\w+)\s*$", line)
        if heading:
            current = names.setdefault("Sexy::" + heading.group(1), {})
            continue
        slot = re.match(r"^\+([0-9A-F]+)/(\d+) (.*?)\s+\[", line)
        if slot and current is not None:
            current[int(slot.group(2))] = slot.group(3)
    return names


def bookworm_names():
    names = {}
    folder = os.path.join(ROOT, "games", "bookworm", "docs", "symbols")
    for leaf in sorted(os.listdir(folder), key=lambda leaf: leaf.startswith("00_")):
        if leaf.startswith(("10_", "00_")):
            for line in open(os.path.join(folder, leaf), encoding="utf-8"):
                row = json.loads(line)
                if "name" in row:
                    names.setdefault(int(row["addr"], 16), row["name"])
    return names


def slots_of(classes, name):
    info = classes.get(name)
    if not info:
        return None
    tables = sorted(info["vtables"], key=lambda t: t["offset"])
    return [int(s, 16) for s in tables[0]["slots"]]


def main():
    heavy = load_classes(os.path.join(ROOT, "re", "heavyweapon", "rtti.classes.json"))
    big = load_classes(os.path.join(MAIN, "re", "bigmoney", "rtti.classes.json"))
    book = load_classes(os.path.join(MAIN, "re", "bookworm", "rtti.classes.json"))
    to_big = load_match("bm.jsonl")
    to_book = load_match("bw.jsonl")
    big_names = bigmoney_slot_names()
    book_names = bookworm_names()
    wanted = sorted(k for k in heavy if k.startswith("Sexy::")) if sys.argv[1:] == ["--all"] else sys.argv[1:]
    for name in wanted:
        mine = slots_of(heavy, name)
        if mine is None:
            print("no class", name)
            continue
        theirs_big = slots_of(big, name) or []
        theirs_book = slots_of(book, name) or []
        print("## %s: %d slots (Big Money %d, BookWorm %d)" % (name, len(mine), len(theirs_big), len(theirs_book)))
        for index, function in enumerate(mine):
            in_big = [i for i, f in enumerate(theirs_big) if to_big.get(function) == f]
            in_book = [i for i, f in enumerate(theirs_book) if to_book.get(function) == f]
            label = ""
            if len(in_big) == 1:
                label = big_names.get(name, {}).get(in_big[0], "")
            if not label and to_book.get(function) in book_names:
                label = book_names[to_book[function]]
            short = lambda found: ",".join(map(str, found[:4])) + ("+" if len(found) > 4 else "") if found else "-"
            print("+%02X/%-3d %x  bm %-8s bw %-8s %s" % (index * 4, index, function, short(in_big), short(in_book), label))


if __name__ == "__main__":
    main()
