#!/usr/bin/env python3
"""Print functions of the Ghidra export (re/typershark/gh/decomp) in a compact, readable form, for reading only.

    python games/typershark/tools/decread.py 4092a0 [00404600 ...]     one or more function addresses
    python games/typershark/tools/decread.py -r 404600 405e10          every function from the first to the last address

What it does to Ghidra's text:
  - names from games/typershark/docs/symbols/*.jsonl and the export replace FUN_<addr>;
  - the structured-exception boilerplate (ExceptionList, unwind indexes, stack cookie) is dropped;
  - field accesses `*(int *)((int)this + 0xd4)` become `this.d4` (a suffix {c} char, {s} short, {f} float,
    {d} double marks a narrower or floating type) and virtual calls `(**(code **)(*obj + 0xc0))(...)`
    become `obj.v[c0](...)` (the slot number is offset / 4: 0xc0 is slot 48);
  - a string literal is followed by `@<address>`: its address in the game image, which a translation names as
    GT(0x<address>) (the strings are the player's own, so the output is never written to a file in the repository).
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
GH = os.path.join(ROOT, "re", "typershark", "gh")

_names = None
_strings = None


def names():
    global _names
    if _names is None:
        _names = {}
        for p in sorted(glob.glob(os.path.join(ROOT, "games", "typershark", "docs", "symbols", "*.jsonl"))):
            for line in open(p, encoding="utf-8"):
                line = line.strip()
                if line:
                    j = json.loads(line)
                    _names[j["addr"].lower().zfill(8)] = j["name"].split("::")[-1]
    return _names


def strings():
    """value -> list of (address, set of xref function addresses)"""
    global _strings
    if _strings is None:
        _strings = {}
        for line in open(os.path.join(GH, "strings.jsonl"), encoding="utf-8", errors="replace"):
            j = json.loads(line)
            if j["type"] == "string":
                _strings.setdefault(j["value"], []).append((j["addr"], set(j.get("xrefs", []))))
    return _strings


def find(addr):
    addr = addr.lower().zfill(8)
    hits = glob.glob(os.path.join(GH, "decomp", "*", addr + "_*.c"))
    return hits[0] if hits else None


TYPE = r"(?:[A-Za-z_][\w:]*(?: [\w:]+)*)"
SEH = re.compile(r"^\s*(?:ExceptionList = [^;]*;|local_4 = [^;]*;|local_10 = DAT_00541a04;|puStack_8 = [^;]*;|"
                 r"pvStack_c = ExceptionList;|undefined4 local_10;|void \*pvStack_c;|undefined1 \*puStack_8;|"
                 r"undefined4 local_4;|/\* WARNING: Function: __security_check_cookie[^*]*\*/)\s*$")
SUFFIX = {"char": "c", "uchar": "c", "byte": "c", "bool": "c", "undefined1": "c", "short": "s", "ushort": "s",
          "undefined2": "s", "float": "f", "double": "d", "float10": "d"}


def fieldsub(text):
    # *(T *)((int)X + 0xN)  and  *(T *)(PATH + 0xN)  with PATH an already converted dotted path
    def acc(m):
        t = m.group(1).strip()
        base = m.group(2)
        off = m.group(3)[2:]
        suf = SUFFIX.get(t, "")
        return base + "." + off + ("{" + suf + "}" if suf else "")
    for _ in range(12):
        new = re.sub(r"\*\((" + TYPE + r") \*+\)\(\(int\)(\w[\w.{}]*) \+ (0x[0-9a-f]+)\)",
                     lambda m: acc(m), text)
        new = re.sub(r"\*\((" + TYPE + r") \*+\)\((\w[\w.{}]*) \+ (0x[0-9a-f]+)\)", lambda m: acc(m), new)
        new = re.sub(r"\*\((" + TYPE + r") \*+\)\(\(int\)(\w[\w.{}]*)\)", lambda m: m.group(2), new)
        if new == text:
            break
        text = new
    return text


def vcalls(text):
    text = re.sub(r"\(\*\*\(code \*\*\)\(\*\*\(int \*\*\)(\w[\w.{}]*) \+ (0x[0-9a-f]+)\)\)",
                  lambda m: "%s.v[%d]" % (m.group(1), int(m.group(2), 16) // 4), text)
    text = re.sub(r"\(\*\*\(code \*\*\)\(\*(\w[\w.{}]*) \+ (0x[0-9a-f]+)\)\)",
                  lambda m: "%s.v[%d]" % (m.group(1), int(m.group(2), 16) // 4), text)
    text = re.sub(r"\(\*\*\(code \*\*\)\(\*(\w[\w.{}]*)\)\)", lambda m: "%s.v[0]" % m.group(1), text)
    return text


def annotate_strings(text, addr):
    def sub(m):
        lit = m.group(1)
        try:
            val = lit.encode().decode("unicode_escape")
        except Exception:
            val = lit
        cands = strings().get(val) or strings().get(lit) or []
        if not cands:
            return m.group(0)
        mine = [a for a, xr in cands if addr in xr]
        a = (mine or [c[0] for c in cands])[0]
        return m.group(0) + "@" + a[2:] if a.startswith("00") else m.group(0) + "@" + a
    return re.sub(r'"((?:[^"\\\n]|\\.)*)"', sub, text)


STRINIT = re.compile(r"^[ \t]*(\w+) = 0xf;\n[ \t]*(\w+) = 0;\n[ \t]*(\w+) = \(void \*\)\(\(uint\)\3 & 0xffffff00\);\n", re.M)
STRFREE = re.compile(r"^[ \t]*if \(0xf < \w+\) \{\n[ \t]*_free\(\w+\);\n[ \t]*\}\n", re.M)
STRASSIGN = re.compile(r'std::string::assign_ptr_len\(\(?(?:string \*\))?([^,]+),\(undefined4 \*\)("(?:[^"\\n]|\.)*"(?:@\w+)?),\w+\)')



# strlen idiom: do { c = *p; p = p + 1; } while (c != '\0');
STRLEN = re.compile(r"^[ \t]*do \{\n[ \t]*\w+ = \*(\w+);\n[ \t]*\1 = \1 \+ 1;\n[ \t]*\} while \(\w+ != '\\0'\);\n", re.M)
STRRESET2 = re.compile(r"^[ \t]*\w+ = \(void \*\)\(\(uint\)\w+ & 0xffffff00\);\n", re.M)
STRCSTR = re.compile(r"std::string::assign_ptr_len\(\(?(?:string \*\))?(&?\w+(?:\[\d+\])?),\(undefined4 \*\)([^,]+),[^;]*?\);")
SEHSTATE = re.compile(r"^[ \t]*(?:local_4|uStack_4|uStack_14|uStack_24|uStack_34|local_44|local_64|local_14|uStack_54)(?:\._0_1_)? = [^;]*;\n", re.M)


STRLEN2 = re.compile(r"^[ \t]*(\w+) = (\"(?:[^\"\\\n]|\\.)*\"(?:@\w+)?|&DAT_\w+);\n[ \t]*do \{\n[ \t]*(\w+) = \1;\n[ \t]*\1 = \3 \+ 1;\n[ \t]*\} while \(\*\3 != '\\0'\);\n", re.M)
STRASSIGN2 = re.compile(r"std::string::assign_ptr_len\s*\(\s*\(?(?:string \*\))?(\w+(?:\[\w+\])?|\(\w+ \+ 0x\w+\)),\s*\(_String_base \*\)(\"(?:[^\"\\\n]|\\.)*\"(?:@\w+)?|&DAT_\w+),\s*\(uint\)\([^;]*?\)\);", re.S)


def simplify(t):
    """collapse the string constructor/destructor boilerplate of Dinkumware's std::string"""
    # VS.NET 2002 idiom: p = "lit"; do { q = p; p = q + 1; } while (*q != 0);  then assign_ptr_len(dest, lit, q - base)
    t = STRLEN2.sub("", t)
    t = STRASSIGN2.sub(lambda m: "%s = STR(%s);" % (m.group(1), m.group(2)), t)
    t = STRINIT.sub("", t)
    t = STRFREE.sub("", t)
    t = STRASSIGN.sub(lambda m: "%s = STR(%s)" % (m.group(1), m.group(2)), t)
    t = STRLEN.sub("", t)
    t = STRCSTR.sub(lambda m: "%s = STR(%s);" % (m.group(1), m.group(2)), t)
    t = re.sub(r"std::string::assign_substr\(\(string \*\)([^,]+),([^,]+),0,0xffffffff\)", r"\1 = \2", t)
    t = re.sub(r"std::string::assign_substr\(([^,]+),([^,]+),0,0xffffffff\)", r"\1 = \2", t)
    t = re.sub(r"^[ \t]*std_string_Tidy\(\(int\)[^)]*\);\n", "", t, flags=re.M)
    t = STRRESET2.sub("", t)
    return t


_res = None


def resnames():
    """index -> name of the IMAGE_ and FONT_ enumerators of src/Res.h (the app's mImages / mFonts tables)"""
    global _res
    if _res is None:
        _res = {"IMAGE": {}, "FONT": {}}
        h = os.path.join(ROOT, "games", "typershark", "src", "Res.h")
        if os.path.exists(h):
            for m in re.finditer(r"^\s*((?:IMAGE|FONT)_\w+)\s*=\s*(\d+)", open(h, encoding="utf-8").read(), re.M):
                _res[m.group(1).split("_")[0]][int(m.group(2))] = m.group(1)
    return _res


def annotate_res(t):
    """app.638.<hex off> is mImages[off / 4], app.63c.<hex off> mFonts[off / 4]"""
    r = resnames()
    t = re.sub(r"\.638\.([0-9a-f]+)", lambda m: ".IMG[%s]" % r["IMAGE"].get(int(m.group(1), 16) // 4, "?"), t)
    t = re.sub(r"\.63c\.([0-9a-f]+)", lambda m: ".FONT[%s]" % r["FONT"].get(int(m.group(1), 16) // 4, "?"), t)
    t = re.sub(r"\.63c \+ (\d+)", lambda m: ".FONT[%s]" % r["FONT"].get(int(m.group(1)) // 4, "?"), t)
    return t


def strip_seh(t):
    """VS.NET 2002 frames: `pXStack_N = &LAB_...; local_M = ExceptionList; ExceptionList = &local_M; local_K = state;`.
    The handler variable's number N gives the state variable (N - 4)."""
    m = re.search(r"^\s*(\w*)Stack_([0-9a-f]+) = &LAB_\w+;", t, re.M)
    if not m:
        return t
    n = int(m.group(2), 16)
    out = []
    for ln in t.split("\n"):
        if re.match(r"^\s*\w*Stack_%x = &LAB_\w+;\s*$" % n, ln):
            continue
        if re.match(r"^\s*\w+ = ExceptionList;\s*$", ln) or re.match(r"^\s*ExceptionList = [^;]*;\s*$", ln):
            continue
        if re.match(r"^\s*(?:local|uStack|pStack|puStack|pvStack)_%x(?:\._0_1_)? = [^;]*;\s*$" % (n - 4), ln):
            continue
        out.append(ln)
    return "\n".join(out)


ELEMSIZE = {"undefined4": 4, "int": 4, "uint": 4, "float": 4, "long": 4, "ulong": 4, "undefined2": 2, "short": 2,
            "ushort": 2, "undefined8": 8, "double": 8, "longlong": 8, "ulonglong": 8}


def scale_ptr_arith(t):
    """Ghidra writes `*(char *)(param_1 + 0x17)` for an `int *param_1` and means byte offset 0x5c (C pointer
    arithmetic scales by the element size). Rewrite those to `*(char *)((int)param_1 + 0x5c)` first, or the
    field conversion below would read them as offset 0x17. Class-typed pointers (`this`) count bytes."""
    size = {}
    for m in re.finditer(r"\b(undefined\d|int|uint|float|long|ulong|short|ushort|double|longlong|ulonglong) (\*+)\s*(\w+)\b", t):
        n = 4 if len(m.group(2)) > 1 else ELEMSIZE.get(m.group(1), 1)
        size[m.group(3)] = n
    def sub(m):
        n = size.get(m.group(2), 1)
        if n == 1:
            return m.group(0)
        return "*(%s *)((int)%s + 0x%x)" % (m.group(1), m.group(2), int(m.group(3), 16) * n)
    return re.sub(r"\*\((" + TYPE + r") \*+\)\((\w+) \+ (0x[0-9a-f]+)\)", sub, t)


def render(path):
    addr = os.path.basename(path)[:8]
    t = open(path, encoding="utf-8", errors="replace").read()
    t = strip_seh(t)
    lines =[ln for ln in t.split("\n") if not SEH.match(ln)]
    t = "\n".join(lines)
    nm = names()
    t = re.sub(r"\bFUN_([0-9a-f]{8})\b", lambda m: nm.get(m.group(1), m.group(0)), t)
    t = re.sub(r"\n{3,}", "\n\n", t)
    t = scale_ptr_arith(t)
    t = fieldsub(t)
    t = vcalls(t)
    t = annotate_strings(t, addr)
    t = simplify(t)
    t = annotate_res(t)
    # local variable declarations ("uint uVar5;", "undefined4 **ppuStack_e4;") and the stack-cookie noise
    t = re.sub(r"^[ \t]+(?:undefined\d?|uint|int|char|bool|float|double|byte|void|string|Color|Rect|Graphics|ulonglong|longlong|"
               r"short|ushort|\w+ \*?)[ \t]*\**[ \t]*\w+(?:\[\w+\])?;\n", "", t, flags=re.M)
    t = re.sub(r"^[ \t]+(?:undefined\d?|uint|int|char|bool|void|string|\w+)(?: \*+| )+\w+(?:\[\w+\])?;\n", "", t, flags=re.M)
    t = re.sub(r"\n{2,}", "\n", t)
    return t


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return
    if args[0] == "-r":
        lo, hi = int(args[1], 16), int(args[2], 16)
        addrs = sorted({os.path.basename(p)[:8] for p in glob.glob(os.path.join(GH, "decomp", "*", "*.c"))
                        if lo <= int(os.path.basename(p)[:8], 16) <= hi})
    else:
        addrs = args
    for a in addrs:
        p = find(a)
        print(render(p) if p else "// %s: not found" % a)


if __name__ == "__main__":
    main()
