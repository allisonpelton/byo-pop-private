#!/usr/bin/env python3
"""Prints functions of the Ghidra export (re/venice/gh/all.c) compactly: local declarations, exception-frame
bookkeeping and blank lines dropped, names from games/venice/docs/symbols.txt put in place of FUN_/DAT_ names.

    python games/venice/tools/decread.py 4092a0 ...            # these functions
    python games/venice/tools/decread.py 401000-402000         # every function that starts in a range
    python games/venice/tools/decread.py --list 401000-402000  # one line each: address, size, name, strings
    python games/venice/tools/decread.py --raw 4092a0          # without the compaction

games/venice/docs/symbols.txt has one name a line, "<address> <name>" (a function, or a global when the address is in
.data); a line may continue with " -- a note". A function named there prints under that name everywhere.
"""
import json
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
ALL = os.path.join(ROOT, "re", "venice", "gh", "all.c")
FUNCTIONS = os.path.join(ROOT, "re", "venice", "gh", "functions.jsonl")
SYMBOLS = os.path.join(ROOT, "games", "venice", "docs", "symbols.txt")
HEADER = re.compile(r"^// ([0-9a-f]{8})  (.*)$", re.M)
DECLARATION = re.compile(
    r"^\s*(undefined\d?|int|uint|char|byte|bool|float|double|float10|long|short|ushort|longlong|ulonglong|void|"
    r"size_t|DWORD|HANDLE|LPVOID|code|[A-Za-z_][\w:<>,]*)\s*\**\s*\**[A-Za-z_]\w*(\[[0-9a-fx]+\])?;\s*$")
NOISE = re.compile(
    r"^\s*/\* WARNING|^\s*\*/\s*$|std::basic_fstream<char>::vbase_dtor|_Tidy\(.*'\x01'\)")
FRAME_NOISE = re.compile(
    r"ExceptionList|^\s*puStack_c = |^\s*local_8 = (0x)?[0-9a-f]+;|^\s*local_8 = \(uint\)local_8\._1_3_ << 8;|"
    r"^\s*local_8 = CONCAT31|^\s*local_8\._\d_\d = |^\s*local_10 = ExceptionList")


def load_symbols():
    names = {}
    if os.path.exists(SYMBOLS):
        for line in open(SYMBOLS, encoding="utf-8"):
            line = line.split(" -- ")[0].strip()
            if line and not line.startswith("#"):
                address, name = line.split(None, 1)
                names[address.lower().zfill(8)] = name.strip()
    return names


def load_functions():
    text = open(ALL, encoding="utf-8", errors="replace").read()
    marks = list(HEADER.finditer(text))
    result = {}
    for index, mark in enumerate(marks):
        end = marks[index + 1].start() if index + 1 < len(marks) else len(text)
        result[int(mark.group(1), 16)] = text[mark.start():end].rstrip() + "\n"
    return result


def compact(text, names):
    lines = []
    for line in text.split("\n"):
        if NOISE.search(line) or DECLARATION.match(line):
            continue
        if not line.strip():
            continue
        lines.append(line.rstrip())
    text = "\n".join(lines)
    text = text.replace("undefined4 *", "u32*").replace("undefined4", "u32").replace("undefined1", "u8")
    text = text.replace("undefined2", "u16").replace("(code **)", "(fn**)")

    def replace(match):
        address = match.group(2).lower()
        name = names.get(address)
        return name if name else match.group(0)

    return re.sub(r"\b(FUN_|DAT_|PTR_FUN_)([0-9a-f]{8})\b", replace, text)


def main():
    arguments = sys.argv[1:]
    if not arguments:
        sys.exit(__doc__)
    raw = "--raw" in arguments
    listing = "--list" in arguments
    arguments = [a for a in arguments if a not in ("--raw", "--list")]
    names = load_symbols()
    functions = load_functions()
    records = {}
    for line in open(FUNCTIONS, encoding="utf-8"):
        record = json.loads(line)
        records[int(record["addr"], 16)] = record
    selected = []
    for argument in arguments:
        if "-" in argument:
            low, high = (int(x, 16) for x in argument.split("-"))
            selected += [a for a in sorted(functions) if low <= a < high]
        else:
            selected.append(int(argument, 16))
    for address in selected:
        record = records.get(address)
        if listing:
            if record:
                strings = ",".join(s[:20].replace("\n", "\n") for s in dict.fromkeys(record["strings"]))[:80]
                print("%08x %5d %-24s %s" % (address, record["size"], names.get("%08x" % address, ""), strings))
            continue
        text = functions.get(address)
        if text is None:
            print("// %08x not in the export\n" % address)
            continue
        label = names.get("%08x" % address)
        out = text if raw else compact(text, names)
        if label:
            out = out.replace("// %08x  Global::FUN_%08x" % (address, address), "// %08x  %s" % (address, label), 1)
        print(out)
        print()


if __name__ == "__main__":
    main()
