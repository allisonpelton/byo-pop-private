"""Rewrites the decompiler's pointer arithmetic as field accesses, and names the fields of a class from a map file.

Ghidra prints a field of an object four ways, depending on the variable's type:
    *(u8 *)((int)this + 0x71d)         this is a void* or a class pointer: byte offset
    *(u32*)(this + 0x38)               this is a class pointer (size 1): byte offset
    param_1[0x1ca]                     param_1 is an int* or u32*: offset 0x1ca * 4
    *(u8 *)(param_1 + 0x1ce)           param_1 is an int*: offset 0x1ce * 4
They all become `var->_<offset in hex>` with the type's suffix when it is not four bytes wide (`param_1->_71d:u8`).
A map file for a class ("<offset> <name> [-- note]", offsets in hex, from games/hammerheads/docs/fields/<Class>.txt) turns
`this->_71d:u8` into `this->mName`.
"""
import os
import re

WIDTH_SUFFIX = {"u8": "u8", "byte": "u8", "char": "u8", "bool": "u8", "u16": "u16", "ushort": "u16", "short": "s16",
                "float": "float", "double": "double", "undefined8": "u64", "longlong": "s64", "ulonglong": "u64"}
FOUR_BYTES = {"u32", "int", "uint", "undefined4", "long", "ulong", "DWORD", "undefined"}
STEP_FOUR = {"u32*", "int*", "uint*", "undefined4*", "float*", "long*", "ulong*"}
STEP_ONE = {"void*", "char*", "u8*", "byte*", "bool*", "undefined*", "undefined1*"}

CAST = r"\*\(([A-Za-z_][\w: ]*?)\s*(\*+)\)"
VARIABLE = r"(\w+(?:->\w+(?::\w+)?)*)"
BYTE_FORM = re.compile(CAST + r"\(\((?:int|uint)\)" + VARIABLE + r"\s*\+\s*(0x[0-9a-f]+|\d+)\)")
POINTER_FORM = re.compile(CAST + r"\(" + VARIABLE + r"\s*\+\s*(0x[0-9a-f]+|\d+)\)")
ADDRESS_FORM = re.compile(r"(?<![*\w])\(([A-Za-z_][\w: ]*?)\s*\*+\)\(" + VARIABLE + r"\s*\+\s*(0x[0-9a-f]+|\d+)\)")
INDEX_FORM = re.compile(r"\b(\w+)\[(0x[0-9a-f]+|\d+)\]")
DECLARED = re.compile(r"^\s*([A-Za-z_][\w:<>]*)\s*(\**)\s*(\w+)(?:\[\w+\])?;\s*$", re.M)
SIGNATURE_PARAM = re.compile(r"([A-Za-z_][\w:<>]*)\s*(\**)\s*(\w+)\s*(?:,|\))")


def number(text):
    return int(text, 16) if text.startswith("0x") else int(text)


def signature(text):
    """The function's signature line: the first line that is not blank, a comment or a brace and holds its parameters."""
    for line in text.splitlines():
        if line.strip() and not line.startswith(("//", "/*", "{", " ", "*/")) and "(" in line:
            return line
    return ""


def variable_types(text):
    """{variable: element type with its stars} from the signature and the declarations, e.g. {"param_1": "int*"}."""
    types = {}
    head = signature(text)
    for match in SIGNATURE_PARAM.finditer(head[head.index("(") + 1:] if "(" in head else ""):
        types[match.group(3)] = match.group(1) + match.group(2).replace(" ", "")
    for match in DECLARED.finditer(text):
        types.setdefault(match.group(3), match.group(1) + match.group(2))
    return types


def step_of(vartype):
    """How many bytes the decompiler adds for each +1 on a variable of this type (None when the type is unknown)."""
    if vartype in STEP_FOUR or vartype.endswith("**"):
        return 4
    if vartype in STEP_ONE or vartype in FOUR_BYTES:
        return 1  # a byte pointer, or an integer holding an address
    if vartype.endswith("*"):
        return 1  # a class pointer: the decompiler counts its size as 1
    return None


def suffix_for(base, stars):
    if len(stars) > 1:
        return ""
    base = base.strip()
    if base in WIDTH_SUFFIX:
        return ":" + WIDTH_SUFFIX[base]
    if base in FOUR_BYTES:
        return ""
    return ":" + base


FRAMEWORK_DOC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "bigmoney", "docs", "FRAMEWORK_12.md")
FRAMEWORK_FIELD = re.compile(r"^\s+\+0x([0-9a-f]+)\s+(\d+)\s+(\S.*?)\s+(m[A-Z]\w*|__vfptr)\s+\[(\w+)\]")


def framework_fields(*classes):
    """{offset: name} of the own members of these framework classes, from the field layouts of Big Money's FRAMEWORK_12.md
    (the same 1.2 layout)."""
    fields = {}
    in_layouts = False
    for line in open(FRAMEWORK_DOC, encoding="utf-8"):
        if line.startswith("## "):
            in_layouts = line.startswith("## Field layouts")
        match = FRAMEWORK_FIELD.match(line) if in_layouts else None
        if match and match.group(5) in classes and match.group(4) != "__vfptr":
            offset, size, name = int(match.group(1), 16), int(match.group(2)), match.group(4)
            count = re.search(r"\[(\d+)\]$", match.group(3))
            if count and int(count.group(1)) > 1:
                for index in range(int(count.group(1))):
                    fields[offset + index * (size // int(count.group(1)))] = "%s[%d]" % (name, index)
            else:
                fields[offset] = name
    return fields


def load_map(path):
    """{offset: name} from a class's field file (a line "<offset in hex> <name> -- note"); "framework <Class>..." adds
    the framework's own members of those classes."""
    fields = {}
    if path and os.path.exists(path):
        for line in open(path, encoding="utf-8"):
            line = line.split(" -- ")[0].strip()
            if line.startswith("framework "):
                fields.update(framework_fields(*line.split()[1:]))
            elif line.startswith("table "):
                fields["table"] = line.split()[1]
            elif line and not line.startswith("#"):
                offset, name = line.split(None, 1)
                fields[int(offset, 16)] = name.split()[0]
    return fields


def normalize(text, fields=None, owner=None, types=None):
    """Returns the text with every field access written as var->_<offset>[:type]; fields names those of `owner`.
    types is variable_types() of the raw text, whose declarations the compaction drops."""
    types = types if types is not None else variable_types(text)
    fields = fields or {}

    def named(variable, offset, suffix):
        if owner and variable == owner and offset in fields:
            return "%s->%s%s" % (variable, fields[offset], suffix)
        return "%s->_%x%s" % (variable, offset, suffix)

    def step(variable):
        return 1 if "->" in variable else step_of(types.get(variable, ""))

    def byte_form(match):
        return named(match.group(3), number(match.group(4)), suffix_for(match.group(1), match.group(2)))

    def pointer_form(match):
        variable = match.group(3)
        unit = step(variable)
        if unit is None:
            return match.group(0)
        return named(variable, number(match.group(4)) * unit, suffix_for(match.group(1), match.group(2)))

    def index_form(match):
        variable = match.group(1)
        if step_of(types.get(variable, "")) != 4:
            return match.group(0)
        return named(variable, number(match.group(2)) * 4, "")

    def address_form(match):
        variable = match.group(2)
        unit = step(variable)
        if unit is None:
            return match.group(0)
        return "&" + named(variable, number(match.group(3)) * unit, "")

    for _ in range(6):
        before = text
        text = ADDRESS_FORM.sub(address_form, text)
        text = BYTE_FORM.sub(byte_form, text)
        text = POINTER_FORM.sub(pointer_form, text)
        text = INDEX_FORM.sub(index_form, text)
        if text == before:
            break
    return text


def owner_of(text):
    """The variable that holds the object a method works on: `this` when the signature has it, else the first parameter."""
    header = signature(text)
    if re.search(r"\bthis\b", header):
        return "this"
    match = re.search(r"\b(param_1)\b", header)
    return match.group(1) if match else None
