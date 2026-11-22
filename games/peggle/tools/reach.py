#!/usr/bin/env python3
"""Which of the exe's game functions can run: the closure of calls and address references from the virtual functions of the
game's classes, the app's constructor and Init (re/peggle/deluxe/gh), minus what the port leaves out.

    python -I games/peggle/tools/reach.py [--cut ADDR ...] [--cut-file games/peggle/docs/cuts.txt] [--list reached|not|all] [--lo A --hi B]
    python -I games/peggle/tools/reach.py --who ADDR     # one chain of calls from a root to this function

Writes re/peggle/deluxe/reach.txt (address, size, name, reached or not) and prints the counts by class. A cut is a function the
port leaves out (registration, the upsell screen, debug and editor code): neither it nor what only it calls counts as reached.
docs/cuts.txt has one address a line, "<address> -- why"; a class name on a line of its own cuts every function of that class
named by the Mac pairs.
"""
import argparse
import json
import os
import re
import sys
from collections import Counter, deque

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
GH = os.path.join(ROOT, "re", "peggle", "deluxe", "gh")
MATCH = os.path.join(ROOT, "re", "peggle", "deluxe", "macmatch", "peggle.match.jsonl")
OUT = os.path.join(ROOT, "re", "peggle", "deluxe", "reach.txt")
LO, HI = 0x401000, 0x4C1800
REFERENCE = re.compile(r"\b(?:PTR_)?FUN_([0-9a-f]{8})\b")
HEADER = re.compile(r"^// ([0-9a-f]{8})  ", re.M)


def load():
    records = {}
    for line in open(os.path.join(GH, "functions.jsonl"), encoding="utf-8"):
        record = json.loads(line)
        records[int(record["addr"], 16)] = record
    names = {}
    if os.path.exists(MATCH):
        for line in open(MATCH, encoding="utf-8"):
            pair = json.loads(line)
            names[int(pair["a"], 16)] = pair["name"]
    return records, names


def references(records):
    """Addresses each function mentions by name in its decompilation (a call or a pointer to a function)."""
    text = open(os.path.join(GH, "all.c"), encoding="utf-8", errors="replace").read()
    marks = list(HEADER.finditer(text))
    result = {}
    for index, mark in enumerate(marks):
        end = marks[index + 1].start() if index + 1 < len(marks) else len(text)
        address = int(mark.group(1), 16)
        result[address] = {int(m, 16) for m in REFERENCE.findall(text[mark.end():end])}
    return result


def vtable_roots():
    roots = set()
    for line in open(os.path.join(GH, "vtables.jsonl"), encoding="utf-8"):
        for slot in json.loads(line)["slots"]:
            address = int(slot["addr"], 16)
            if LO <= address < HI:
                roots.add(address)
    return roots


def class_of(name):
    parts = name.split("::")
    return "::".join(parts[:-1]).replace("Sexy::", "") if len(parts) > 1 else "(unnamed)"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--cut", nargs="*", default=[])
    parser.add_argument("--cut-file", default=os.path.join(ROOT, "games", "peggle", "docs", "cuts.txt"))
    parser.add_argument("--list", choices=["reached", "not", "all"])
    parser.add_argument("--lo", default=hex(LO))
    parser.add_argument("--hi", default=hex(HI))
    parser.add_argument("--who", default=None)
    args = parser.parse_args()

    records, names = load()
    refs = references(records)
    cut_addresses = {int(a, 16) for a in args.cut}
    cut_classes = set()
    if os.path.exists(args.cut_file):
        for line in open(args.cut_file, encoding="utf-8"):
            line = line.split("--")[0].strip()
            if not line or line.startswith("#"):
                continue
            if re.fullmatch(r"[0-9a-f]{6,8}", line):
                cut_addresses.add(int(line, 16))
            else:
                cut_classes.add(line)
    for address, name in names.items():
        if class_of(name) in cut_classes:
            cut_addresses.add(address)

    roots = vtable_roots() | {0x426F10, 0x42CA30}
    reached = set()
    parent = {}
    queue = deque()
    for root in sorted(roots):
        if root not in cut_addresses:
            reached.add(root)
            queue.append(root)
    while queue:
        address = queue.popleft()
        record = records.get(address)
        targets = set(refs.get(address, ()))
        if record:
            targets |= {int(c, 16) for c in record["callees"] if re.fullmatch(r"[0-9a-f]{8}", c)}
        for target in sorted(targets):
            if LO <= target < HI and target not in reached and target not in cut_addresses:
                reached.add(target)
                parent[target] = address
                queue.append(target)

    if args.who:
        chain = [int(args.who, 16)]
        while chain[-1] in parent:
            chain.append(parent[chain[-1]])
        print(" <- ".join("%08x %s" % (a, names.get(a, "")) for a in chain))
        return

    low, high = int(args.lo, 16), int(args.hi, 16)
    game = [a for a in sorted(records) if LO <= a < HI and not records[a]["thunk"]]
    totals, hits, sizes, hit_sizes = Counter(), Counter(), Counter(), Counter()
    with open(OUT, "w", encoding="utf-8") as handle:
        for address in game:
            record = records[address]
            name = names.get(address, "")
            owner = class_of(name) if name else "(unnamed)"
            totals[owner] += 1
            sizes[owner] += record["size"]
            if address in reached:
                hits[owner] += 1
                hit_sizes[owner] += record["size"]
            handle.write("%08x %5d %s %s\n" % (address, record["size"], "R" if address in reached else "-", name))
            if args.list and low <= address < high and (args.list == "all" or (args.list == "reached") == (address in reached)):
                print("%08x %5d %s %s" % (address, record["size"], "R" if address in reached else "-", name))
    if not args.list:
        print("%d game functions (%d bytes), %d reached (%d bytes)" % (
            len(game), sum(sizes.values()), len([a for a in game if a in reached]), sum(hit_sizes.values())))
        for owner, count in sorted(totals.items(), key=lambda item: -sizes[item[0]])[:60]:
            print("%-22s %4d functions %6d bytes   reached %4d (%6d bytes)" % (owner, count, sizes[owner], hits[owner], hit_sizes[owner]))


if __name__ == "__main__":
    main()
