#!/usr/bin/env python3
"""Find strings of the game image: addresses and the functions that use them (from Ghidra's export, re/rocketmania/gh/strings.jsonl).

    python games/rocketmania/tools/strfind.py <function address>     the strings that function uses, with their addresses
    python games/rocketmania/tools/strfind.py -t <text>              strings that contain the text
    python games/rocketmania/tools/strfind.py -a <string address>    the string at that address (from the exe, any bytes)
"""
import json
import os
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


def rows():
    for line in open(os.path.join(ROOT, "re", "rocketmania", "gh", "strings.jsonl"), encoding="utf-8", errors="replace"):
        j = json.loads(line)
        if j["type"] == "string":
            yield j


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    if a[0] == "-t":
        for j in rows():
            if a[1].lower() in j["value"].lower():
                print(j["addr"], repr(j["value"]), " ".join(j.get("xrefs", [])))
    elif a[0] == "-a":
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import ptrtable
        print(repr(ptrtable.cstr(int(a[1], 16))))
    else:
        f = a[0].lower().replace("0x", "").zfill(8)
        for j in sorted(rows(), key=lambda r: r["addr"]):
            if f in j.get("xrefs", []):
                print(j["addr"], repr(j["value"]))


if __name__ == "__main__":
    main()
