#!/usr/bin/env python3
"""Finds the original's functions that the game can reach, to decide which of the exe's code is translated. A function is
reachable when WinMain's roots (the Application's constructor, `0x401000` on) lead to it by a direct call (E8 rel32),
by an address of it stored or pushed (a callback), or by a vtable of a class whose address is stored or pushed (every
slot of that vtable). The scan reads machine code, so it follows what the decompiler shows and what it hides.

    python games/hammerheads/tools/reach.py --from 41ffd0 [--skip 405550-41e030 ...] [--out re/hammerheads/reach.txt]
    python games/hammerheads/tools/reach.py --from 41ffd0 --report 401000-4767e0    # reached and unreached sizes, by range

--skip a-b leaves the functions of that address range out of the walk (the editor, the DRM client): a call into one is
not followed, and the functions that only they reach stay unreached.
"""
import argparse
import bisect
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decread
import pe

TEXT = (0x401000, 0x550000)
RDATA = (0x551000, 0x590000)


def parse_range(text):
    low, high = text.split("-")
    return int(low, 16), int(high, 16)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--from", dest="roots", action="append", default=[])
    parser.add_argument("--skip", action="append", default=[])
    parser.add_argument("--report", default="401000-4767e0")
    parser.add_argument("--out")
    args = parser.parse_args()
    skips = [parse_range(s) for s in args.skip]
    image = pe.Image()
    functions = {}
    for line in open(decread.FUNCTIONS, encoding="utf-8"):
        record = json.loads(line)
        if not record["external"]:
            functions[int(record["addr"], 16)] = record["size"]
    starts = sorted(functions)

    def function_at(address):
        index = bisect.bisect_right(starts, address) - 1
        if index >= 0 and starts[index] <= address < starts[index] + functions[starts[index]]:
            return starts[index]
        return None

    classes = json.load(open(os.path.join(decread.ROOT, "re", "hammerheads", "rtti.classes.json")))
    vtable_slots = {}
    for info in classes.values():
        for table in info["vtables"]:
            vtable_slots[int(table["va"], 16)] = [int(s, 16) for s in table["slots"]]

    def targets_of(start):
        code = image.bytes(start, functions[start])
        found = set()
        for i in range(len(code) - 4):
            if code[i] == 0xE8:
                target = (start + i + 5 + struct.unpack_from("<i", code, i + 1)[0]) & 0xFFFFFFFF
                if TEXT[0] <= target < TEXT[1]:
                    found.add(target)
            value = struct.unpack_from("<I", code, i)[0]
            if TEXT[0] <= value < TEXT[1] and value in functions:
                found.add(value)
            elif value in vtable_slots:
                found.update(vtable_slots[value])
        return found

    reached = set()
    pending = [int(r, 16) for r in args.roots]
    while pending:
        address = pending.pop()
        owner = function_at(address)
        if owner is None or owner in reached:
            continue
        if any(low <= owner < high for low, high in skips):
            continue
        reached.add(owner)
        pending.extend(targets_of(owner) - reached)
    low, high = parse_range(args.report)
    total = sum(size for a, size in functions.items() if low <= a < high)
    done = sum(functions[a] for a in reached if low <= a < high)
    print("reached %d of %d bytes in %x-%x (%d functions)" % (done, total, low, high, len([a for a in reached if low <= a < high])))
    step = 0x4000
    for start in range(low, high, step):
        block_total = sum(size for a, size in functions.items() if start <= a < start + step)
        block_done = sum(functions[a] for a in reached if start <= a < start + step)
        print("  %x-%x  reached %6d of %6d" % (start, start + step, block_done, block_total))
    if args.out:
        with open(args.out, "w") as f:
            for address in sorted(reached):
                f.write("%08x %d\n" % (address, functions[address]))


if __name__ == "__main__":
    main()
