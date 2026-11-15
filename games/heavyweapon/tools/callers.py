#!/usr/bin/env python3
"""Prints who calls a function of the export, up to a depth, with each caller's strings.

    python games/heavyweapon/tools/callers.py 432320 [--depth 4]
"""
import argparse
import json
import os

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("address")
    parser.add_argument("--depth", type=int, default=3)
    args = parser.parse_args()
    functions = {}
    for line in open(os.path.join(ROOT, "re", "heavyweapon", "gh", "functions.jsonl"), encoding="utf-8"):
        record = json.loads(line)
        functions[record["addr"]] = record

    def show(address, depth, seen):
        record = functions.get(address)
        if record is None:
            return
        strings = ",".join(s[:18] for s in dict.fromkeys(record["strings"]))[:60]
        print("  " * depth + address, record["size"], record["name"][:28], strings)
        if depth >= args.depth or address in seen:
            return
        seen.add(address)
        for caller in record["callers"]:
            show(caller, depth + 1, seen)

    show(args.address.zfill(8), 0, set())


if __name__ == "__main__":
    main()
