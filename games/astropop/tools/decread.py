#!/usr/bin/env python3
"""Print functions of the Ghidra export (re/astropop/gh/decomp) in a compact, readable form, for reading only.

    python games/astropop/tools/decread.py 4092a0 [00404600 ...]     one or more function addresses
    python games/astropop/tools/decread.py -r 404600 405e10          every function from the first to the last address

What it does to Ghidra's text:
  - names from games/astropop/docs/symbols/*.jsonl and the export replace FUN_<addr>;
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
GH = os.path.join(ROOT, "re", "astropop", "gh")

_names = None
_strings = None


def names():
    global _names
    if _names is None:
        _names = {}
        for p in sorted(glob.glob(os.path.join(ROOT, "games", "astropop", "docs", "symbols", "*.jsonl"))):
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
SEH = re.compile(r"^\s*(?:ExceptionList = [^;]*;|local_4 = [^;]*;|local_10 = DAT_005a24b0;|puStack_8 = [^;]*;|"
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


DWPTR = re.compile(r"\b(int|uint|undefined4|float|long|ulong|code)\s*\*\s*(\w+)\s*[;,)=]")


def dwordptrs(text):
    """Ghidra scales pointer arithmetic on int * variables: `NAME[0x252]` and `*(char *)(NAME + 0x8d)` are byte
    offsets 0x948 and 0x234. Both are rewritten to the byte-offset form `NAME.948` (suffix {f}, {c} for the type)."""
    names_ = {m.group(2): m.group(1) for m in DWPTR.finditer(text) if m.group(1) != "code"}
    for nm_, ty in names_.items():
        def idx(m, ty=ty):
            suf = "{f}" if ty == "float" else ""
            return "%s.%x%s" % (m.group(1), int(m.group(2), 0) * 4, suf)
        text = re.sub(r"\b(" + re.escape(nm_) + r")\[(0x[0-9a-f]+|\d+)\]", idx, text)
        def acc(m):
            t_ = m.group(1).strip()
            suf = SUFFIX.get(t_, "")
            return "%s.%x%s" % (m.group(2), int(m.group(3), 0) * 4, "{" + suf + "}" if suf else "")
        text = re.sub(r"\*\((" + TYPE + r") \*+\)\((" + re.escape(nm_) + r") \+ (0x[0-9a-f]+|\d+)\)", acc, text)
    return text


def dwordaddrs(text):
    """The address of a subobject, `NAME + 0xN` with an int * NAME, is scaled by 4 as well: shown as `(char*)NAME+0xBYTES`."""
    for nm_ in {m.group(2) for m in DWPTR.finditer(text) if m.group(1) != "code" and m.group(2).startswith(("param_", "this"))}:
        text = re.sub(chr(40) + "?<![" + chr(92) + "w.*)]" + chr(41) + re.escape(nm_) + " [+] (0x[0-9a-f]+|[0-9]+)" + chr(92) + "b",
                      lambda m, nm_=nm_: "(char*)%s+0x%x" % (nm_, int(m.group(1), 0) * 4), text)
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


def simplify(t):
    """collapse the string constructor/destructor boilerplate of Dinkumware's std::string"""
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
        h = os.path.join(ROOT, "games", "astropop", "src", "Res.h")
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



# --- AstroPop additions -------------------------------------------------------------------------------------
# strlen idiom with any variable names, with the pointer assignment before it
STRLEN2 = re.compile(r"^[ \t]*(?:\w+ = (?:&?DAT_\w+|\"(?:[^\"\\\n]|\\.)*\"(?:@\w+)?);\n)?[ \t]*do \{\n[ \t]*(\w+) = (\w+);\n"
                     r"[ \t]*\2 = \1 \+ 1;\n[ \t]*\} while \(\*\1 != '\\0'\);\n", re.M)
ASSIGN2 = re.compile(r"std::string::assign_ptr_len\s*\(\s*(?:\(string \*\))?([^,;]+?),\s*(?:\(_String_base \*\))?\s*(&?DAT_\w+|\"(?:[^\"\\\n]|\\.)*\"(?:@\w+)?),\s*"
                     r"\(uint\)\([^;]*?\)\);", re.S)
UNUSED = re.compile(r"(\w+)\[(0x[0-9a-f]+|\d+)\]\.unused")


def unusedsub(t):
    def f(m):
        idx = int(m.group(2), 0)
        return "%s.%x" % (m.group(1), idx * 4)
    t = UNUSED.sub(f, t)
    t = re.sub(r"\*\(char \*\)\(\(int\)&(\w+)\[(0x[0-9a-f]+|\d+)\]\.unused \+ (\d+)\)",
               lambda m: "%s.%x{c}" % (m.group(1), int(m.group(2), 0) * 4 + int(m.group(3))), t)
    t = re.sub(r"(\w+)->unused", r"\1.0", t)
    return t


_resmap = None


def resmap():
    global _resmap
    if _resmap is None:
        p = os.path.join(ROOT, "re", "astropop", "resmap.json")
        _resmap = json.load(open(p)) if os.path.exists(p) else {}
    return _resmap


_img = None


def rdata_string(va):
    """The short C string at a .rdata address (a DAT_ that is really text), else None."""
    global _img
    import struct
    if _img is None:
        d = open(os.path.join(ROOT, "re", "astropop", "bin", "AstroPopGame.exe"), "rb").read()
        pe = struct.unpack_from("<I", d, 0x3c)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        opt = struct.unpack_from("<H", d, pe + 20)[0]
        base = struct.unpack_from("<I", d, pe + 52)[0]
        secs = []
        for i in range(nsec):
            o = pe + 24 + opt + i * 40
            vs, sva, rs, ro = struct.unpack_from("<IIII", d, o + 8)
            secs.append((sva, vs, ro, rs))
        _img = (d, base, secs)
    d, base, secs = _img
    r = va - base
    for sva, vs, ro, rs in secs:
        if sva <= r < sva + max(vs, rs):
            o = ro + r - sva
            e = d.find(b"\0", o)
            if e < 0 or e - o > 60:
                return None
            b = d[o:e]
            if all(32 <= c < 127 for c in b):
                return b.decode("latin-1")
            return None
    return None


def datsub(m):
    r = resmap().get(m.group(1))
    if r:
        return r
    va = int(m.group(1), 16)
    if 0x55d000 <= va < 0x59c000:
        s = rdata_string(va)
        if s is not None:
            return 'S"%s"@%x' % (s.replace('"', "'"), va)
    return m.group(0)


def extra(t):
    t = re.sub(r"\b_?DAT_([0-9a-f]{8})\b", datsub, t)
    t = re.sub(r"\bLAB_[0-9a-f]{8}\b", "LAB", t) if False else t
    t = STRLEN2.sub("", t)
    t = ASSIGN2.sub(lambda m: "%s = STR(%s);" % (m.group(1).strip(), m.group(2)), t)
    t = unusedsub(t)
    t = re.sub(r"^[ \t]*local_8 = [^;]*;\n", "", t, flags=re.M)
    t = re.sub(r"^[ \t]*local_8\._\d_\d = [^;]*;\n", "", t, flags=re.M)
    t = re.sub(r"\(_String_base \*\)", "", t)
    t = re.sub(r"\(void \*\)0x0", "NULL", t)
    return t


def render(path):
    addr = os.path.basename(path)[:8]
    t = open(path, encoding="utf-8", errors="replace", newline="").read().replace("\r\n", "\n")
    lines = [ln for ln in t.split("\n") if not SEH.match(ln)]
    t = "\n".join(lines)
    nm = names()
    t = re.sub(r"\bFUN_([0-9a-f]{8})\b", lambda m: nm.get(m.group(1), m.group(0)), t)
    t = re.sub(r"\n{3,}", "\n\n", t)
    t = dwordptrs(t)
    t = fieldsub(t)
    t = vcalls(t)
    t = dwordaddrs(t)
    t = annotate_strings(t, addr)
    t = simplify(t)
    t = extra(t)
    t = annotate_res(t)
    # local variable declarations ("uint uVar5;", "undefined4 **ppuStack_e4;") and the stack-cookie noise
    # (never a statement: `return 1;` and `goto LAB_x;` look like a declaration of a variable)
    kw = r"(?!return\b|goto\b|else\b|break\b|case\b|continue\b|delete\b|throw\b)"
    decls = []

    def keep(m):
        decls.append(m.group(0).strip())
        return ""

    t = re.sub(r"^[ \t]+" + kw + r"(?:undefined\d?|uint|int|char|bool|float|double|byte|void|string|Color|Rect|Graphics|ulonglong|longlong|"
               r"short|ushort|\w+ \*?)[ \t]*\**[ \t]*\w+(?:\[\w+\])?;\n", keep, t, flags=re.M)
    t = re.sub(r"^[ \t]+" + kw + r"(?:undefined\d?|uint|int|char|bool|void|string|\w+)(?: \*+| )+\w+(?:\[\w+\])?;\n", keep, t, flags=re.M)
    t = re.sub(r"\n{2,}", "\n", t)
    if decls:
        # the locals' types on one line (their signedness matters)
        groups = {}
        for d in decls:
            ty, _, nm = d.rstrip(";").rpartition(" ")
            if nm.startswith("*"):
                ty, nm = ty + " *", nm.lstrip("*")
            groups.setdefault(ty, []).append(nm)
        line = "  // locals: " + "; ".join("%s %s" % (ty, ",".join(v)) for ty, v in groups.items())
        t = t.replace("{\n", "{\n" + line + "\n", 1)
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
