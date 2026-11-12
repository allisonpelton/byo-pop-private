#!/usr/bin/env python3
"""Prints functions of the Ghidra export (re/hammerheads/ghidra/all.c) compactly: local declarations, exception-frame
bookkeeping and blank lines dropped, names from games/hammerheads/docs/symbols.txt put in place of FUN_/DAT_ names.

    python games/hammerheads/tools/decread.py 4092a0 ...            # these functions
    python games/hammerheads/tools/decread.py 401000-402000         # every function that starts in a range
    python games/hammerheads/tools/decread.py --list 401000-402000  # one line each: address, size, name, strings
    python games/hammerheads/tools/decread.py --raw 4092a0          # without the compaction

games/hammerheads/docs/symbols.txt has one name a line, "<address> <name>" (a function, or a global when the address is in
.data); a line may continue with " -- a note". A function named there prints under that name everywhere.
"""
import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pe

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
ALL = os.path.join(ROOT, "re", "hammerheads", "ghidra", "all.c")
FUNCTIONS = os.path.join(ROOT, "re", "hammerheads", "ghidra", "functions.jsonl")
SYMBOLS = os.path.join(ROOT, "games", "hammerheads", "docs", "symbols.txt")
HEADER = re.compile(r"^// ([0-9a-f]{8})  (.*)$", re.M)
DECLARATION = re.compile(
    r"^\s*(undefined\d?|int|uint|char|byte|bool|float|double|float10|long|short|ushort|longlong|ulonglong|void|"
    r"size_t|DWORD|HANDLE|LPVOID|code|[A-Za-z_][\w:<>,]*)(?:\s+\**|\s*\*+)\s*[A-Za-z_]\w*(\[[0-9a-fx]+\])?;\s*$")
NOISE = re.compile(
    r"^\s*/\* WARNING|^\s*\*/\s*$|std::basic_fstream<char>::vbase_dtor|_Tidy\(.*'\x01'\)")
FRAME_NOISE = re.compile(
    r"ExceptionList|^\s*(local_4|uStack_4)(\._\d_\d)? = |^\s*puStack_8 = (&LAB_|\(u8 \*\)|0x)|"
    r"^\s*(pvStack_c|local_c|local_10|pvStack_10) = (ExceptionList|&)")


FRAMEWORK_DOC = os.path.join(ROOT, "games", "bigmoney", "docs", "FRAMEWORK_12.md")
SLOT_CLASSES = ("SexyApp", "Widget", "Dialog")
_SLOTS = None


def slot_tables():
    """{class: {byte offset: method name}} from the vtable slot tables of Big Money's FRAMEWORK_12.md (the same 1.2 layout)."""
    global _SLOTS
    if _SLOTS is None:
        _SLOTS = {}
        current = None
        in_slots = False
        for line in open(FRAMEWORK_DOC, encoding="utf-8"):
            if line.startswith("## Vtable slots"):
                in_slots = True
            elif line.startswith("## ") and in_slots:
                in_slots = False
            elif in_slots and line.startswith("### "):
                current = line[4:].strip()
                _SLOTS[current] = {}
            elif in_slots and current:
                match = re.match(r"^\+([0-9A-F]+)/\d+ (.*?)\(", line)
                if match:
                    _SLOTS[current][int(match.group(1), 16)] = match.group(2).strip()
    return _SLOTS


def slot_note(match):
    """Annotates a virtual call (**(fn**)(*obj + 0xNN)) with the slot's name in the app, widget and dialog tables."""
    text = match.group(2)
    offset = int(text, 16) if text.startswith("0x") else int(text)
    tables = slot_tables()
    names = ["%s:%s" % (c, tables[c][offset]) for c in SLOT_CLASSES if c in tables and offset in tables[c]]
    # the three tables agree on the listener slots 0..0x18 and differ elsewhere
    return "%s/*+0x%x %s*/" % (match.group(0), offset, " ".join(names))


def load_symbols():
    names = {}
    for path in (os.path.join(os.path.dirname(SYMBOLS), "resglobals.txt"), SYMBOLS):
        if os.path.exists(path):
            for line in open(path, encoding="utf-8"):
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


_IMAGE = None


def image():
    global _IMAGE
    if _IMAGE is None:
        _IMAGE = pe.Image(os.path.join(ROOT, "re", "hammerheads", "bin", "HammerHeadsGame.exe"))
    return _IMAGE


def string_at(address):
    """The C string at an address of .rdata or .data, or None when it is not printable text."""
    try:
        data = image().bytes(address, 80)
    except Exception:
        return None
    end = data.find(bytes([0]))
    if end < 0:
        return None
    text = data[:end]
    if all(32 <= c < 127 or c in (9, 10, 13) for c in text):
        return text.decode("ascii")
    return None


def float_note(match):
    """Marks a hex literal that reads as a plausible float, e.g. 0x44968000 /*1200f*/. The export prints a number with the sign bit
    set as a negative integer (-0x40800000 is the float -1.0, 0xbf800000), so a leading minus is read as the integer's sign."""
    value = int(match.group(2), 16)
    if match.group(1):
        value = (-value) & 0xFFFFFFFF
    exponent = (value >> 23) & 0xFF
    if not 100 <= exponent <= 150:
        return match.group(0)
    number = struct.unpack("<f", struct.pack("<I", value))[0]
    if match.group(1):
        return "0x%08x/*%gf*/" % (value, number)
    return "%s/*%gf*/" % (match.group(2), number)


def string_note(match):
    address = int(match.group(1), 16)
    if not 0x551000 <= address < 0x590000:
        return match.group(0)
    value = string_at(address)
    if value is None:
        return match.group(0)
    return '"%s"/*%s*/' % (value, match.group(0))


def compact(text, names):
    lines = []
    has_frame = "ExceptionList" in text
    for line in text.split("\n"):
        keeps = re.match(r"^\s*(return|break|continue|goto)\b", line)
        if not keeps and (NOISE.search(line) or DECLARATION.match(line) or (has_frame and FRAME_NOISE.search(line))):
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

    text = re.sub(r"\b(FUN_|DAT_|PTR_FUN_)([0-9a-f]{8})\b", replace, text)
    text = re.sub(r"\(\*\*\(fn\*\*\)\(\*(\w+(?:\[\w+\])?) \+ (0x[0-9a-f]+|\d+)\)\)", slot_note, text)
    text = re.sub(r"&(?:PTR_)?DAT_([0-9a-f]{8})\b", string_note, text)
    text = re.sub(r"(?<![\w)\]])(-?)(0x[0-9a-f]{8})\b", float_note, text)
    text = re.sub(r"\b0x(5[0-9a-f]{5})\b", string_note, text)  # a name passed as a bare address
    return text


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
