#!/usr/bin/env python3
"""Print functions of the Ghidra export (re/pixelus/gh/decomp) in a compact, readable form, for reading only.

    python games/pixelus/tools/decread.py 4092a0 [00404600 ...]     one or more function addresses
    python games/pixelus/tools/decread.py -r 404600 405e10          every function from the first to the last address

What it does to Ghidra's text:
  - names from games/pixelus/docs/symbols/*.jsonl and the export replace FUN_<addr>;
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
GH = os.path.join(ROOT, "re", "pixelus", "gh")

_names = None
_strings = None


def names():
    global _names
    if _names is None:
        _names = {}
        for p in sorted(glob.glob(os.path.join(ROOT, "games", "pixelus", "docs", "symbols", "*.jsonl"))):
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



def simplify(t):
    """collapse the string constructor/destructor boilerplate of Dinkumware's std::string"""
    t = STRINIT.sub("", t)
    t = STRFREE.sub("", t)
    t = STRASSIGN.sub(lambda m: "%s = STR(%s)" % (m.group(1), m.group(2)), t)
    return t


def render(path):
    addr = os.path.basename(path)[:8]
    t = open(path, encoding="utf-8", errors="replace").read()
    lines = [ln for ln in t.split("\n") if not SEH.match(ln)]
    t = "\n".join(lines)
    nm = names()
    t = re.sub(r"\bFUN_([0-9a-f]{8})\b", lambda m: nm.get(m.group(1), m.group(0)), t)
    t = re.sub(r"\n{3,}", "\n\n", t)
    t = fieldsub(t)
    t = vcalls(t)
    t = annotate_strings(t, addr)
    return simplify(t)


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
