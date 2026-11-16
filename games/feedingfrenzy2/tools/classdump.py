#!/usr/bin/env python3
"""A class of FeedingFrenzy2Game.exe as its RTTI describes it, for translating it: its bases, each vtable with every
slot's function (address, size, the export's name) marked "=" where the slot is the primary base's own function, and
the functions that store one of its vtables into an object (its constructors and destructors).

    python -I games/feedingfrenzy2/tools/classdump.py <class> [<class>...]     (as RTTI names it: engine::Scene)
    python -I games/feedingfrenzy2/tools/classdump.py --find <text>            the classes whose name holds the text

Reads re/feedingfrenzy2/rtti/ff2.classes.json (tools/rtti_msvc70.py's output) and the export's functions.jsonl, both
git-ignored, from the worktree when it has them and else from the main checkout.
"""
import json
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
COMMON = subprocess.check_output(["git", "rev-parse", "--git-common-dir"], cwd=HERE, text=True).strip()
MAIN = os.path.dirname(os.path.abspath(os.path.join(HERE, COMMON)))


def find_re():
    for base in (ROOT, MAIN):
        path = os.path.join(base, "re", "feedingfrenzy2")
        if os.path.isfile(os.path.join(path, "rtti", "ff2.classes.json")):
            return path
    sys.exit("no re/feedingfrenzy2/rtti/ff2.classes.json: run tools/rtti_msvc70.py first")


RE = find_re()
EXE = os.path.join(MAIN, "re", "feedingfrenzy2", "bin", "FeedingFrenzy2Game.exe")


def load():
    classes = json.load(open(os.path.join(RE, "rtti", "ff2.classes.json"), encoding="utf-8"))
    functions = {}
    with open(os.path.join(RE, "gh", "functions.jsonl"), encoding="utf-8") as f:
        for line in f:
            record = json.loads(line)
            functions[int(record["addr"], 16)] = record
    return classes, functions


def vtable_stores(vtables):
    """The code addresses that store one of the vtables (a mov of the address as an immediate into memory)."""
    data = open(EXE, "rb").read()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional = struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    hits = {}
    for i in range(count):
        name = data[pe + 24 + optional + i * 40:pe + 24 + optional + i * 40 + 8].rstrip(b"\0")
        if name != b".text":
            continue
        size, rva, raw_size, raw = struct.unpack_from("<IIII", data, pe + 24 + optional + i * 40 + 8)
        text = data[raw:raw + raw_size]
        for va in vtables:
            needle = struct.pack("<I", va)
            at = text.find(needle)
            while at >= 0:
                # C7 /0 (mov r/m32, imm32): the immediate is the last 4 bytes of the instruction
                if any(text[at - k] == 0xC7 for k in (2, 3, 6)):
                    hits.setdefault(va, []).append(base + rva + at)
                at = text.find(needle, at + 1)
    return hits


def owner(functions, starts, address):
    import bisect
    i = bisect.bisect_right(starts, address) - 1
    if i < 0:
        return None
    start = starts[i]
    return start if address < start + functions[start]["size"] else None


def dump(name, classes, functions):
    if name not in classes:
        print("%s: no such class" % name)
        return
    entry = classes[name]
    print("%s : %s" % (name, ", ".join(entry["bases"])))
    primary = classes.get(entry["bases"][0]) if entry["bases"] else None
    base_slots = {}
    if primary:
        for table in primary["vtables"]:
            base_slots[table["offset"]] = table["slots"]
    starts = sorted(functions)
    vtables = [int(t["va"], 16) for t in entry["vtables"]]
    stores = vtable_stores(vtables)
    for table in entry["vtables"]:
        va = int(table["va"], 16)
        inherited = base_slots.get(table["offset"], []) if table["offset"] == 0 else []
        print("  vtable %x at +0x%x, %d slots" % (va, table["offset"], len(table["slots"])))
        cells = []
        for i, slot in enumerate(table["slots"]):
            address = int(slot, 16)
            record = functions.get(address, {})
            mark = "=" if i < len(inherited) and inherited[i] == slot else " "
            label = record.get("full", "")
            if label.startswith("Global::"):
                label = label[8:]
            cells.append("    %03x%s %x %4d %s" % (i * 4, mark, address, record.get("size", 0), label))
        print("\n".join(cells))
        users = sorted({owner(functions, starts, a) for a in stores.get(va, []) if owner(functions, starts, a)})
        if users:
            print("    stored by " + " ".join("%x" % u for u in users))


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    classes, functions = load()
    if args[0] == "--find":
        for name in sorted(classes):
            if args[1].lower() in name.lower():
                print(name)
        return
    for name in args:
        dump(name, classes, functions)


if __name__ == "__main__":
    main()
