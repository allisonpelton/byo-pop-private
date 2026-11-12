#!/usr/bin/env python3
"""Prints functions of the Ghidra export (re/hammerheads/ghidra/all.c) compactly: local declarations, exception-frame
bookkeeping and blank lines dropped, names from games/hammerheads/docs/symbols.txt put in place of FUN_/DAT_ names.

    python games/hammerheads/tools/decread.py 4092a0 ...            # these functions
    python games/hammerheads/tools/decread.py 401000-402000         # every function that starts in a range
    python games/hammerheads/tools/decread.py --list 401000-402000  # one line each: address, size, name, strings
    python games/hammerheads/tools/decread.py --raw 4092a0          # without the compaction
    python games/hammerheads/tools/decread.py --class Avatar 42d730 # fields of Avatar by name (docs/fields/Avatar.txt)
    python games/hammerheads/tools/decread.py --no-offsets 42d730   # leave the pointer arithmetic as the decompiler printed it

games/hammerheads/docs/symbols.txt has one name a line, "<address> <name>" (a function, or a global when the address is in
.data); a line may continue with " -- a note". A function named there prints under that name everywhere.
A field access prints as `var->_<offset in hex>[:type]` (tools/offsets.py); games/hammerheads/docs/fields/<Class>.txt names a
class's fields ("<offset> <name> -- note", offsets in hex) and --class applies it to `this` (the first parameter).
"""
import json
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import offsets
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


# the allocation and constructor call that the compiler wraps in a null check:
#   p = operator_new(N); if (p == 0) { q = 0; } else { q = Ctor(p); }  ->  q = new(N) Ctor(p);
NEW_OBJECT = re.compile(
    r"(\w+) = operator_new\((0x[0-9a-f]+|\d+)\);\n\s*(?:\w+ = \w+;\n\s*)?if \(\1 == (?:\([\w *]+\))?0x0\) \{\n\s*(\w+) = (?:\([\w *]+\))?0x?0?;\n"
    r"\s*\}\n\s*else \{\n\s*\3 = (?:\([\w *]+\))?([\w:~]+)\(\1((?:,[^;\n]*)?)\);\n\s*\}")
# the inlined std::string destructor and reset the compiler leaves after each temporary, and the string assignments
STRING_TIDY = re.compile(r"\n\s*if \(0xf < \(?(?:uint\)?)?\)?\w+\) \{\n\s*_free\([^;\n]*\);\n(?:\s*\w+ = \w+;\n)?\s*\}")
STRING_RESET = re.compile(r"\n\s*\w+ = 0xf;\n\s*\w+ = (?:\(void \*\))?0x?0?;\n\s*\w+ = \(void \*\)\(\(uint\)\w+ & 0xffffff00\);")
STRING_ASSIGN = re.compile(r'std::string::assign_ptr_len\s*\(\s*(?:\(string \*\))?(&?[\w>.\-]+),\s*\(u32\*\)\s*("(?:[^"\\]|\\.)*")(?:/\*.*?\*/)?\s*,\s*\w+\s*\);')
STRING_FROM = re.compile(r'std::string::string_cstr\s*\(\s*(?:\(string \*\))?(&?[\w>.\-]+),\s*("(?:[^"\\]|\\.)*")\s*\);')
VIRTUAL_CALL = re.compile(r"\(\*\*\(fn\*\*\)\(\*(.+?) \+ (0x[0-9a-f]+|\d+)\)\)")
OBJECT_CAST = re.compile(r"^\(\w+ ?\*+\)")


OWNER = {"name": None, "table": None}


def virtual_call(match):
    """Rewrites a virtual call (**(fn**)(*obj + 0xNN)) as obj->vfNN<Name>: the slot's name in the app, widget and dialog
    tables of the 1.2 framework (one name when the tables agree, each table's otherwise)."""
    text = match.group(2)
    offset = int(text, 16) if text.startswith("0x") else int(text)
    tables = slot_tables()
    classes = SLOT_CLASSES
    if OWNER["table"] and OBJECT_CAST.sub("", match.group(1)) == OWNER["name"]:
        classes = (OWNER["table"],)
    names = [(c, tables[c][offset]) for c in classes if c in tables and offset in tables[c]]
    if not names:
        label = ""
    elif len(classes) == 1:
        return "%s->%s" % (OBJECT_CAST.sub("", match.group(1)), names[0][1])
    elif len({n for c, n in names}) == 1:
        label = "<%s>" % names[0][1]
    else:
        label = "<%s>" % "|".join("%s:%s" % cn for cn in names)
    return "%s->vf%x%s" % (OBJECT_CAST.sub("", match.group(1)), offset, label)


def load_symbols():
    names = {}
    for path in (os.path.join(os.path.dirname(SYMBOLS), "autonames.txt"), os.path.join(os.path.dirname(SYMBOLS), "resglobals.txt"), SYMBOLS):
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


# trivial functions are written where they are called: a getter `return param_1->_f4;` as `x->_f4`, a global getter
# `return DAT_005cea54;` as the global
GETTER_BODY = re.compile(r"^\{\n  return ((?:param_1|this)->[\w>:\-]+|DAT_[0-9a-f]{8}|[A-Za-z_]\w*);\n\}$")
_GETTERS = None
FUNCTION_TEXT = {}
ARGUMENT = r"([^,()]*(?:\([^()]*\)[^,()]*)*)"


def getters(names):
    global _GETTERS
    if _GETTERS is None:
        _GETTERS = {}
        for address, text in FUNCTION_TEXT.items():
            if len(text) > 400 or not 0x401000 <= address < 0x4767e0:
                continue
            body = compact(text, names, None, True, inline=False)
            lines = body.split("\n")
            signature_end = next((i for i, line in enumerate(lines) if line.startswith("{")), None)
            if signature_end is None:
                continue
            match = GETTER_BODY.match("\n".join(lines[signature_end:]).strip())
            if match:
                _GETTERS[address] = match.group(1)
    return _GETTERS


def inline_getters(text, names):
    table = getters(names)

    def replace(match):
        address = int(match.group(1), 16)
        expression = table.get(address)
        if expression is None:
            return match.group(0)
        if match.group(2) is None:
            return expression
        argument = match.group(2).strip()
        argument = re.sub(r"^\((?:int|u32|void|\w+) ?\*+\)", "", argument)
        return re.sub(r"\b(param_1|this)\b", lambda m: argument, expression, 1)

    return re.sub(r"\bFUN_([0-9a-f]{8})\(" + ARGUMENT + r"\)", replace, text)


def compact(text, names, fields=None, normalize=True, inline=True):
    types = offsets.variable_types(text)
    owner = offsets.owner_of(text)
    OWNER["name"] = owner
    OWNER["table"] = (fields or {}).get("table")
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
    text = re.sub(r"&(?:PTR_)?DAT_([0-9a-f]{8})\b", string_note, text)
    text = re.sub(r"(?<![\w)\]])(-?)(0x[0-9a-f]{8})\b", float_note, text)
    text = re.sub(r"\b0x(5[0-9a-f]{5})\b", string_note, text)  # a name passed as a bare address
    if normalize:
        text = offsets.normalize(text, fields, owner, types)
    text = VIRTUAL_CALL.sub(virtual_call, text)
    text = STRING_TIDY.sub("", text)
    text = STRING_RESET.sub("", text)
    text = STRING_ASSIGN.sub(lambda m: "%s = %s;" % (m.group(1).lstrip("&"), m.group(2)), text)
    text = STRING_FROM.sub(lambda m: "%s = %s;" % (m.group(1).lstrip("&"), m.group(2)), text)
    text = NEW_OBJECT.sub(lambda m: "%s = new(%s) %s(%s);" % (m.group(3), m.group(2), m.group(4), m.group(5)), text)
    if inline:
        text = inline_getters(text, names)
    return text


OBJDUMP = None
if os.path.isdir("E:/retools"):
    for folder in sorted(os.listdir("E:/retools")):
        if folder.startswith("llvm-mingw") and os.path.exists(os.path.join("E:/retools", folder, "bin", "llvm-objdump.exe")):
            OBJDUMP = os.path.join("E:/retools", folder, "bin", "llvm-objdump.exe")
FP_STORE = re.compile(r"^\s+[0-9a-f]+:\s+(fst|fstp|fist|fistp)\s+(dword|qword|word) ptr (\[.*\])")


def fp_note(address, size):
    """What the decompiler gets wrong about floating point: it types an x87 value as float where the code stores a double.
    The note lists the stores of the function's machine code, f for a 32-bit float and d for a 64-bit double, i for an
    integer, in order, so a `(float)` in the text can be checked against what is stored."""
    if not OBJDUMP:
        return ""
    out = subprocess.run([OBJDUMP, "-d", "--no-show-raw-insn", "--x86-asm-syntax=intel", "--start-address=0x%x" % address,
                          "--stop-address=0x%x" % (address + size), os.path.join(ROOT, "re", "hammerheads", "bin", "HammerHeadsGame.exe")],
                         capture_output=True, text=True).stdout
    stores = []
    for line in out.splitlines():
        match = FP_STORE.match(line.replace("\t", " "))
        if match:
            kind = "i" if match.group(1).startswith("fi") else {"dword": "f", "qword": "d", "word": "w"}[match.group(2)]
            stores.append("%s%s" % (kind, match.group(3)))
    return "// x87 stores: " + " ".join(stores) if stores else ""


def main():
    arguments = sys.argv[1:]
    if not arguments:
        sys.exit(__doc__)
    raw = "--raw" in arguments
    listing = "--list" in arguments
    normalize = "--no-offsets" not in arguments
    fields = None
    if "--class" in arguments:
        at = arguments.index("--class")
        fields = offsets.load_map(os.path.join(ROOT, "games", "hammerheads", "docs", "fields", arguments[at + 1] + ".txt"))
        del arguments[at:at + 2]
    arguments = [a for a in arguments if a not in ("--raw", "--list", "--no-offsets")]
    names = load_symbols()
    functions = load_functions()
    FUNCTION_TEXT.update(functions)
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
        out = text if raw else compact(text, names, fields, normalize)
        if label:
            out = out.replace("// %08x  Global::FUN_%08x" % (address, address), "// %08x  %s" % (address, label), 1)
        if not raw and record and ("fVar" in out or "float10" in out or "(float)" in out or "double" in out):
            note = fp_note(address, record["size"])
            if note:
                out = out.replace("\n", "\n" + note + "\n", 1)
        print(out)
        print()


if __name__ == "__main__":
    main()
