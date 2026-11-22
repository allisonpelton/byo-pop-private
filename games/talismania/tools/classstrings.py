#!/usr/bin/env python3
"""The strings a class's functions use, for reading a data loader's attributes at a glance.

    python -I games/talismania/tools/classstrings.py <class> [<class>...]

For each function in the class's vtables (and the constructors and destructors RTTI found) prints its address, size and
the strings it references in the order of their addresses, from re/talismania/gh/strings.jsonl and functions.jsonl.
"""
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
COMMON = subprocess.check_output(["git", "rev-parse", "--git-common-dir"], cwd=HERE, text=True).strip()
MAIN = os.path.dirname(os.path.abspath(os.path.join(HERE, COMMON)))
RE = os.path.join(ROOT, "re", "talismania")
if not os.path.isfile(os.path.join(RE, "rtti", "tal.classes.json")):
    RE = os.path.join(MAIN, "re", "talismania")


def main():
    classes = json.load(open(os.path.join(RE, "rtti", "tal.classes.json"), encoding="utf-8"))
    functions = {}
    for line in open(os.path.join(RE, "gh", "functions.jsonl"), encoding="utf-8"):
        record = json.loads(line)
        functions[int(record["addr"], 16)] = record
    starts = sorted(functions)
    xrefs = {}
    for line in open(os.path.join(RE, "gh", "strings.jsonl"), encoding="utf-8"):
        record = json.loads(line)
        for x in record["xrefs"]:
            if ":" not in x:
                xrefs.setdefault(int(x, 16), []).append(record["value"])
    import bisect
    owner_strings = {}
    for address, texts in xrefs.items():
        i = bisect.bisect_right(starts, address) - 1
        if i >= 0 and address < starts[i] + functions[starts[i]]["size"]:
            owner_strings.setdefault(starts[i], []).extend(texts)
    for name in sys.argv[1:]:
        info = classes.get(name)
        if not info:
            print("no class " + name)
            continue
        seen = []
        for vtable in info["vtables"]:
            for slot in vtable["slots"]:
                address = slot if isinstance(slot, int) else int(str(slot), 16)
                if address not in seen:
                    seen.append(address)
        print("== " + name)
        for address in seen:
            record = functions.get(address)
            texts = owner_strings.get(address, [])
            if texts:
                print("  %x %d: %s" % (address, record["size"] if record else 0,
                                       " | ".join(t.replace("\n", "\\n")[:50] for t in texts)))


if __name__ == "__main__":
    main()
