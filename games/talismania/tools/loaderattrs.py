#!/usr/bin/env python3
"""The attributes a data loader's function reads, in order, for writing its translation.

    python -I games/talismania/tools/loaderattrs.py <addr> [<addr>...]

Reads the compacted function (tools/fn.py) and, for each attribute name the function puts in a string and each Params
getter it then calls, prints one line: the name, the getter (string, int, float, bool, intpair, floatpair, range, rect,
color, pivot), the default as written, and the assignment that follows (the field it goes to). The getters are the
engine's engine::Params methods in Talismania's exe: 0x64f2a0 string, 0x64f380 int, 0x64f4e0 float, 0x64f640 range,
0x64f7a0 bool, 0x64f920 floatpair, 0x64fa80 intpair, 0x64fbc0 rect, 0x64fd00 color, 0x64fe90 pivot. It is a reading aid:
the output is checked against the function.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GETTERS = {"64f2a0": "string", "64f380": "int", "64f4e0": "float", "64f640": "range", "64f7a0": "bool",
           "64f920": "floatpair", "64fa80": "intpair", "64fbc0": "rect", "64fd00": "color", "64fe90": "pivot"}
NAME = re.compile(r'FUN_0041177[0-9a-f]\("([^"]+)"|FUN_00411730\("([^"]+)"\)|FUN_00411(?:730|770)\(&DAT_([0-9a-f]+)')
STRINGS = {}
CALL = re.compile(r"FUN_0064([0-9a-f]{4})\((.*)\)")


def load_strings():
    import json
    root = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
    path = os.path.join(root, "re", "talismania", "gh", "strings.jsonl")
    if not os.path.isfile(path):
        common = subprocess.check_output(["git", "rev-parse", "--git-common-dir"], cwd=HERE, text=True).strip()
        main_root = os.path.dirname(os.path.abspath(os.path.join(HERE, common)))
        path = os.path.join(main_root, "re", "talismania", "gh", "strings.jsonl")
    for line in open(path, encoding="utf-8"):
        record = json.loads(line)
        STRINGS[record["addr"].lstrip("0")] = record["value"]


def main():
    load_strings()
    for address in sys.argv[1:]:
        text = subprocess.check_output([sys.executable, "-I", os.path.join(HERE, "fn.py"), address], text=True)
        lines = text.splitlines()
        print("== " + address)
        pending = None
        for i, line in enumerate(lines):
            m = NAME.search(line)
            if m:
                pending = m.group(1) or m.group(2) or STRINGS.get(m.group(3).lstrip("0"), "DAT_" + m.group(3))
                if pending == "":
                    pending = None
                continue
            m = CALL.search(line)
            if m and pending is not None and m.group(1) in {k[1:] if len(k) > 4 else k for k in GETTERS} | set(
                    k[2:] for k in GETTERS):
                kind = GETTERS.get("64" + m.group(1)) or GETTERS.get(m.group(1))
                arguments = [a.strip() for a in m.group(2).split(",")]
                default = arguments[-1]
                target = ""
                for after in lines[i:i + 8]:
                    t = re.search(r"(\*\(\w+ \*\)\(param_\d->m90 \+ (0x[0-9a-f]+)\)|param_\d->m90->m[0-9a-f]+\w*|"
                                  r"iVar\d->m[0-9a-f]+\w*|\w+->m[0-9a-f]+\w*) = ", after)
                    if t:
                        target = after.strip()
                        break
                print("  %-26s %-9s default %-12s %s" % (pending, kind, default, target))
                pending = None


if __name__ == "__main__":
    main()
