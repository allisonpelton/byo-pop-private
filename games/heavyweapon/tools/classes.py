#!/usr/bin/env python3
"""Lists the vtables of Heavy Weapon's own classes, which have no RTTI, and the functions that store each one.

    python games/heavyweapon/tools/classes.py [--out re/heavyweapon/classes.txt]

A vtable is a run of pointers into .text in .rdata that RTTI does not name. A constructor or destructor stores the
vtable's address into its object (mov [reg], imm32 or mov [reg+off], imm32), so the functions holding that immediate
name the class. The output is one block a vtable: its address, slot count, the functions that store it and its slots.
"""
import argparse
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, HERE)
import pe  # noqa: E402

IMAGE_FILE = pe.Image()
TEXT = (0x401000, 0x401000 + 0x125D10)
RDATA = (0x527000, 0x527000 + 0x3724E)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default=os.path.join(ROOT, "re", "heavyweapon", "classes.txt"))
    args = parser.parse_args()
    image = IMAGE_FILE.data

    def read_rdata(va):
        return IMAGE_FILE.u32(va)

    known = set()
    for info in json.load(open(os.path.join(ROOT, "re", "heavyweapon", "rtti.classes.json"))).values():
        for table in info["vtables"]:
            known.add(int(table["va"], 16))
    functions = sorted(
        int(json.loads(line)["addr"], 16)
        for line in open(os.path.join(ROOT, "re", "heavyweapon", "gh", "functions.jsonl"), encoding="utf-8")
        if not json.loads(line)["external"])

    def function_of(va):
        low, high = 0, len(functions)
        while low < high:
            middle = (low + high) // 2
            if functions[middle] <= va:
                low = middle + 1
            else:
                high = middle
        return functions[low - 1] if low else None

    runs = []
    va = RDATA[0]
    while va < RDATA[1]:
        if TEXT[0] <= read_rdata(va) < TEXT[1]:
            start = va
            while va < RDATA[1] and TEXT[0] <= read_rdata(va) < TEXT[1]:
                va += 4
            runs.append((start, (va - start) // 4))
        else:
            va += 4
    stores = {}
    text = IMAGE_FILE.bytes(TEXT[0], TEXT[1] - TEXT[0])
    for offset in range(len(text) - 4):
        immediate_at = offset + 1
        if immediate_at + 4 > len(text):
            continue
        value = struct.unpack_from("<I", text, immediate_at)[0]
        if RDATA[0] <= value < RDATA[1]:
            stores.setdefault(value, []).append(TEXT[0] + offset)
    with open(args.out, "w") as out:
        for start, count in runs:
            if start in known or count < 3:
                continue
            holders = sorted({function_of(site) for site in stores.get(start, []) if function_of(site)})
            out.write("vtable 0x%x slots %d stored by %s\n" % (start, count, " ".join("0x%x" % h for h in holders)))
            out.write("  " + " ".join("%x" % read_rdata(start + 4 * i) for i in range(count)) + "\n")
    print("wrote", args.out)


if __name__ == "__main__":
    main()
