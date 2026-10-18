#!/usr/bin/env python3
"""Compute MSVC (7.0-2005, 32-bit) data member offsets of SexyAppFramework classes from their headers.

Game code reads framework fields by raw offset (this+0x4c8), so translating a game needs each field's
offset in the framework generation it was built against. This lays out the classes of a framework
tree with the MSVC x86 rules (members in declaration order, aligned to min(size, 8); bases in
declaration order; one vfptr per polymorphic base chain) and the VC7 Dinkumware container sizes.

    python tools/saf_struct_layout.py <framework dir> Widget Dialog SexyAppBase [--define ZYLOM]
        [--check SexyAppBase.mCustomCursorsEnabled=0x4c8 ...]

Unknown member types are reported; give their size with --size Type=bytes[:align]. Members a build
lacks are dropped with --remove Class.member,... (Big Money: games/bigmoney/docs/FRAMEWORK_12.md).
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from saf_vtable_layout import strip_comments, find_classes, top_level  # noqa: E402

# (size, align) of types that are not classes in the tree. VC7 Dinkumware: string/wstring 28 (allocator
# 4 + 16-byte buffer + size + capacity), list/map/set/multimap 12 (allocator 4, head, size), vector 16
# (allocator 4, first, last, end), deque 20, pair<string,int> 32.
BASIC = {
    "bool": (1, 1), "char": (1, 1), "uchar": (1, 1), "BYTE": (1, 1), "SexyChar": (1, 1), "signed char": (1, 1),
    "unsigned char": (1, 1), "short": (2, 2), "ushort": (2, 2), "WORD": (2, 2), "wchar_t": (2, 2),
    "int": (4, 4), "uint": (4, 4), "long": (4, 4), "ulong": (4, 4), "unsigned": (4, 4), "unsigned int": (4, 4),
    "unsigned long": (4, 4), "DWORD": (4, 4), "LONG": (4, 4), "UINT": (4, 4), "float": (4, 4), "HWND": (4, 4),
    "HCURSOR": (4, 4), "HANDLE": (4, 4), "HINSTANCE": (4, 4), "HMODULE": (4, 4), "HFONT": (4, 4), "HDC": (4, 4),
    "HBITMAP": (4, 4), "HRESULT": (4, 4), "KeyCode": (4, 4), "LPDIRECTDRAWSURFACE": (4, 4), "time_t": (4, 4),
    "double": (8, 8), "__int64": (8, 8), "int64": (8, 8), "uint64": (8, 8), "LARGE_INTEGER": (8, 8),
    "std::string": (28, 4), "std::wstring": (28, 4), "SexyString": (28, 4), "string": (28, 4),
    "CRITICAL_SECTION": (24, 4), "MSG": (28, 4), "RECT": (16, 4), "POINT": (8, 4), "GUID": (16, 4),
    "DDSURFACEDESC2": (124, 4), "DDPIXELFORMAT": (32, 4), "WNDPROC": (4, 4),
}
CONTAINERS = {"list": (12, 4), "map": (12, 4), "set": (12, 4), "multimap": (12, 4), "multiset": (12, 4),
              "vector": (16, 4), "deque": (20, 4)}


def norm_type(t):
    t = re.sub(r"\b(const|volatile|mutable|struct|class|enum|typename)\b", " ", t)
    return " ".join(t.split())


class Layouter:
    def __init__(self, tree, defines, sizes, remove=()):
        self.remove = set(remove)  # "Class.member": declared in the headers, absent from the binary
        self.cls = {}
        self.typedefs = {}
        self.consts = {}
        for root, _, files in os.walk(tree):
            for f in files:
                if not f.endswith(".h"):
                    continue
                raw = open(os.path.join(root, f), encoding="latin-1").read()
                src = self.preprocess(strip_comments(raw), defines)
                for m in re.finditer(r"typedef\s+([^;{}]+?)\s+(\w+)\s*;", src):
                    self.typedefs[m.group(2)] = norm_type(m.group(1))
                for m in re.finditer(r"\benum\b[^{;]*\{([^}]*)\}", src):
                    val = -1
                    for item in m.group(1).split(","):
                        item = item.strip()
                        if not item:
                            continue
                        if "=" in item:
                            k, v = [x.strip() for x in item.split("=", 1)]
                            try:
                                val = int(eval(v, {}, dict(self.consts)))
                            except Exception:
                                val += 1
                        else:
                            k = item
                            val += 1
                        self.consts[k] = val
                for m in re.finditer(r"#define\s+(\w+)\s+(\d+)\b", src):
                    self.consts[m.group(1)] = int(m.group(2))
                for name, bases, body in find_classes(src):
                    if name not in self.cls or len(body) > len(self.cls[name][1]):
                        self.cls[name] = (bases, body)
        self.sizes = dict(BASIC)
        self.sizes.update(sizes)
        self.cache = {}
        self.unknown = set()

    @staticmethod
    def preprocess(src, defines):
        out = []
        stack = []
        for line in src.split("\n"):
            s = line.strip()
            m = re.match(r"#\s*(ifdef|ifndef|if|else|elif|endif)\b\s*(.*)", s)
            if m:
                d, arg = m.group(1), m.group(2).strip()
                if d in ("ifdef", "ifndef"):
                    on = (arg in defines) == (d == "ifdef")
                    if arg.startswith("__") and arg.endswith("_H__"):
                        on = True  # include guards
                    stack.append(on)
                elif d == "if":
                    stack.append("0" not in arg.split())
                elif d == "else" and stack:
                    stack[-1] = not stack[-1]
                elif d == "endif" and stack:
                    stack.pop()
                continue
            if all(stack):
                out.append(line)
        return "\n".join(out)

    def has_vptr(self, name):
        bases, body = self.cls.get(name, ([], ""))
        if re.search(r"\bvirtual\b", top_level(body)):
            return True
        return any(self.has_vptr(b) for b in bases if b in self.cls)

    def type_size(self, t):
        t = norm_type(t)
        if t.endswith("*") or t.endswith("&"):
            return 4, 4
        if t in self.sizes:
            return self.sizes[t]
        m = re.match(r"(?:std::)?(\w+)\s*<", t)
        if m and m.group(1) in CONTAINERS:
            return CONTAINERS[m.group(1)]
        if m and m.group(1) == "pair":
            inner = t[t.index("<") + 1:t.rindex(">")]
            parts = split_template_args(inner)
            return self.struct_of([self.type_size(p) for p in parts])
        if m and m.group(1) in ("basic_string",):
            return 28, 4
        if t in self.typedefs and self.typedefs[t] != t:
            return self.type_size(self.typedefs[t])
        short = t.split("::")[-1]
        if short in self.typedefs and self.typedefs[short] != t:
            return self.type_size(self.typedefs[short])
        if short in self.cls:
            lay = self.layout(short)
            return lay["size"], lay["align"]
        if short in self.consts or re.match(r"^[A-Z]\w*$", short) and short.endswith("Type"):
            return 4, 4
        self.unknown.add(t)
        return 4, 4

    @staticmethod
    def struct_of(members):
        off = 0
        al = 1
        for s, a in members:
            off = (off + a - 1) // a * a
            off += s
            al = max(al, a)
        return (off + al - 1) // al * al, al

    def members(self, body):
        res = []
        for decl in top_level(body).split(";"):
            d = " ".join(decl.split())
            d = re.sub(r"^(?:(?:public|protected|private)\s*:\s*)+", "", d)
            if not d or "(" in d or d.startswith(("typedef", "friend", "using", "enum", "static", "virtual",
                                                  "template", "class ", "struct ", "#")):
                if d.startswith(("struct ", "class ")) and "(" not in d and "{" not in d and len(d.split()) == 3:
                    pass  # "struct Foo bar" member: falls through below
                else:
                    continue
            if "operator" in d:
                continue
            d = re.sub(r"\s*=\s*.*$", "", d)  # initialisers
            # several declarators: "int a, b;"  (no templates with commas expected here)
            m = re.match(r"^(.*?[\w>*&\s])\s*([*&]*\s*\w+\s*(?:\[[^\]]*\])*)((?:\s*,\s*[*&]*\s*\w+\s*(?:\[[^\]]*\])*)*)$", d)
            if not m:
                continue
            base_t = m.group(1).strip()
            decls = [m.group(2)] + [x for x in re.split(r"\s*,\s*", m.group(3)) if x.strip()]
            if "<" in base_t and base_t.count("<") != base_t.count(">"):
                # a template argument list with commas was split: re-glue
                whole = d
                tm = re.match(r"^(.*>)\s*([*&]*\s*\w+(?:\[[^\]]*\])*)$", whole)
                if not tm:
                    continue
                base_t, decls = tm.group(1), [tm.group(2)]
            for dn in decls:
                ptr = re.match(r"^\s*([*&]*)\s*(\w+)\s*((?:\[[^\]]*\])*)$", dn)
                if not ptr:
                    continue
                t = base_t + ptr.group(1)
                count = 1
                for dim in re.findall(r"\[([^\]]*)\]", ptr.group(3)):
                    try:
                        count *= int(eval(dim, {}, dict(self.consts)))
                    except Exception:
                        self.unknown.add("array size " + dim)
                res.append((t, ptr.group(2), count))
        return res

    def layout(self, name):
        if name in self.cache:
            return self.cache[name]
        bases, body = self.cls[name]
        fields = []
        off = 0
        al = 1
        poly_base = False
        for b in bases:
            if b not in self.cls:
                self.unknown.add("base " + b)
                continue
            bl = self.layout(b)
            off = (off + bl["align"] - 1) // bl["align"] * bl["align"]
            for f in bl["fields"]:
                fields.append(dict(f, off=f["off"] + off))
            if self.has_vptr(b):
                poly_base = True
            off += bl["size"]
            al = max(al, bl["align"])
        if not poly_base and self.has_vptr(name):
            fields.insert(0, {"off": 0, "size": 4, "type": "vfptr", "name": "__vfptr", "cls": name})
            for f in fields[1:]:
                f["off"] += 4
            off += 4
            al = max(al, 4)
        for t, n, count in self.members(body):
            if "%s.%s" % (name, n) in self.remove:
                continue
            s, a = self.type_size(t)
            a = min(a, 8)
            off = (off + a - 1) // a * a
            fields.append({"off": off, "size": s * count, "type": norm_type(t) + ("[%d]" % count if count > 1 else ""),
                           "name": n, "cls": name})
            off += s * count
            al = max(al, a)
        size = max((off + al - 1) // al * al, 1)
        lay = {"fields": fields, "size": size, "align": al}
        self.cache[name] = lay
        return lay


def split_template_args(s):
    out, depth, cur = [], 0, ""
    for c in s:
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        if c == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += c
    if cur.strip():
        out.append(cur.strip())
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("tree")
    ap.add_argument("classes", nargs="+")
    ap.add_argument("--define", action="append", default=[])
    ap.add_argument("--size", action="append", default=[], help="Type=bytes[:align]")
    ap.add_argument("--check", action="append", default=[], help="Class.member=0xOFF")
    ap.add_argument("--own", action="store_true", help="print only the class's own members")
    ap.add_argument("--remove", default="", help="comma-separated Class.member absent from the binary's build")
    args = ap.parse_args()
    sizes = {}
    for s in args.size:
        t, v = s.split("=")
        b, _, a = v.partition(":")
        sizes[t] = (int(b, 0), int(a, 0) if a else min(int(b, 0), 4))
    lay = Layouter(args.tree, set(args.define), sizes, [x for x in args.remove.split(",") if x])
    for c in args.classes:
        l = lay.layout(c)
        print("== %s  sizeof 0x%x (%d)" % (c, l["size"], l["size"]))
        for f in l["fields"]:
            if args.own and f["cls"] != c:
                continue
            print("  +0x%03x %4d  %-40s %s  [%s]" % (f["off"], f["size"], f["type"][:40], f["name"], f["cls"]))
    bad = 0
    for chk in args.check:
        k, v = chk.split("=")
        c, m = k.split(".")
        f = [x for x in lay.layout(c)["fields"] if x["name"] == m]
        got = f[0]["off"] if f else None
        ok = got == int(v, 0)
        bad += not ok
        print("check %s: expected %s got %s %s" % (k, v, hex(got) if got is not None else None, "ok" if ok else "MISMATCH"))
    if lay.unknown:
        print("unknown types (assumed 4 bytes):", ", ".join(sorted(lay.unknown)), file=sys.stderr)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
