#!/usr/bin/env python3
"""Compute MSVC vtable slot layouts from SexyAppFramework headers and name a binary's vtable slots.

MSVC (7.0 through 2005) lays out a class's primary vtable as: the primary base's slots, overrides in
place, then the class's new virtuals in declaration order, with overloads of one name grouped at the
first declaration and in reverse declaration order. Secondary bases (listeners) get their own
vtables at their subobject offsets. This reproduces that from the headers of a framework tree and,
given tools/rtti_msvc70.py output, writes ApplySymbols lines that turn Class::vfNN into real names.

    python tools/saf_vtable_layout.py <framework dir> --dump Widget Dialog       # print layouts
    python tools/saf_vtable_layout.py <framework dir> --classes rtti.classes.json --out fw.jsonl
        [--check]   # compare slot counts with the binary's vtables
        [--nonvirtual SexyAppBase::ProcessDeferredMessages,...]   # build differences (see GAME_NOTES)

Only primary (offset 0) vtables are named; a class whose computed slot count differs from the
binary's is reported and skipped.
"""
import argparse
import json
import os
import re
import sys


def strip_comments(s):
    s = re.sub(r"/\*.*?\*/", " ", s, flags=re.S)
    s = re.sub(r"//[^\n]*", " ", s)
    return s


def find_classes(src):
    """Yield (name, bases, body) for each class/struct definition in a header."""
    for m in re.finditer(r"\b(class|struct)\s+(?:\w+\s+)?(\w+)\s*(:[^{;]*)?\{", src):
        name = m.group(2)
        bases = []
        if m.group(3):
            for b in m.group(3)[1:].split(","):
                b = b.strip().split()
                if b:
                    bases.append(b[-1].split("::")[-1])
        # body: match braces
        i = m.end()
        depth = 1
        while i < len(src) and depth:
            if src[i] == "{":
                depth += 1
            elif src[i] == "}":
                depth -= 1
            i += 1
        yield name, bases, src[m.end():i - 1]


def top_level(body):
    """The body with nested braces (inline bodies, nested classes) removed."""
    out = []
    depth = 0
    for c in body:
        if c == "{":
            depth += 1
            if depth == 1:
                out.append(";")
            continue
        if c == "}":
            depth -= 1
            continue
        if depth == 0:
            out.append(c)
    return "".join(out)


def virtuals(body):
    """[(name, signature)] of virtual member functions in declaration order."""
    res = []
    for decl in top_level(body).split(";"):
        decl = " ".join(decl.split())
        # drop access specifiers glued in front
        decl = re.sub(r"^(?:(?:public|protected|private)\s*:\s*)+", "", decl)
        if not re.match(r"virtual\b", decl):
            continue
        m = re.search(r"(~?\w+)\s*\((.*)\)\s*(const)?\s*(=\s*0)?\s*$", decl)
        if not m:
            m = re.search(r"(~?\w+)\s*\((.*)\)", decl)
            if not m:
                continue
        name = m.group(1)
        params = re.sub(r"\s*=\s*[^,]+", "", m.group(2))  # default args
        res.append((name, "%s(%s)%s" % (name, params.strip(), " const" if (m.lastindex or 0) >= 3 and m.group(3) else "")))
    return res


class Layout:
    def __init__(self, tree, nonvirtual=()):
        self.nonvirtual = set(nonvirtual)  # "Class::Method": virtual in the headers, not in the binary
        self.cls = {}
        for root, _, files in os.walk(tree):
            for f in files:
                if f.endswith(".h"):
                    src = strip_comments(open(os.path.join(root, f), encoding="latin-1").read())
                    for name, bases, body in find_classes(src):
                        if name not in self.cls or len(body) > len(self.cls[name][1]):
                            self.cls[name] = (bases, body)
        self.cache = {}

    def slots(self, name):
        """[(name, signature, defining class)] for the primary vtable."""
        if name in self.cache:
            return self.cache[name]
        bases, body = self.cls.get(name, ([], ""))
        primary = None
        for b in bases:
            if b in self.cls and self.has_vtable(b):
                primary = b
                break
        table = list(self.slots(primary)) if primary else []
        # every virtual of every base, to recognise overrides of secondary-base methods
        secondary = set()
        for b in bases:
            if b != primary and b in self.cls:
                secondary |= self.all_names(b)
        new = []
        for vname, sig in virtuals(body):
            if "%s::%s" % (name, vname) in self.nonvirtual:
                continue
            key = "dtor" if vname.startswith("~") else sig_key(sig)
            idx = next((i for i, s in enumerate(table) if s[3] == key), None)
            if idx is not None:
                table[idx] = (vname if not vname.startswith("~") else "~" + name, sig, name, key)
                continue
            if key in secondary:
                continue
            new.append((vname, sig, name, key))
        # overloads: group at the first declaration, reverse declaration order
        ordered = []
        seen = set()
        for i, s in enumerate(new):
            if s[0] in seen:
                continue
            group = [x for x in new if x[0] == s[0]]
            seen.add(s[0])
            ordered.extend(reversed(group))
        table += ordered
        self.cache[name] = table
        return table

    def all_names(self, name):
        out = set()
        bases, body = self.cls.get(name, ([], ""))
        for vname, sig in virtuals(body):
            out.add("dtor" if vname.startswith("~") else sig_key(sig))
        for b in bases:
            if b in self.cls:
                out |= self.all_names(b)
        return out

    def has_vtable(self, name):
        bases, body = self.cls.get(name, ([], ""))
        return bool(virtuals(body)) or any(self.has_vtable(b) for b in bases if b in self.cls)


TYPE_WORDS = {"int", "bool", "char", "short", "long", "float", "double", "unsigned", "signed", "const", "uint",
              "ulong", "uchar", "ushort", "DWORD", "WPARAM", "LPARAM", "HWND", "UINT"}


def sig_key(sig):
    """Name plus parameter types (no parameter names) for override matching."""
    m = re.match(r"(~?\w+)\((.*)\)( const)?$", sig)
    if not m:
        return sig
    types = []
    for p in m.group(2).split(","):
        p = p.strip()
        if not p or p == "void":
            continue
        # array parameters are pointers; then drop the trailing parameter name
        arrays = len(re.findall(r"\[[^\]]*\]", p))
        p = re.sub(r"\[[^\]]*\]", "", p)
        tok = p.replace("*", " * ").replace("&", " & ").split()
        if len(tok) > 1 and re.match(r"^[A-Za-z_]\w*$", tok[-1]) and tok[-1] not in TYPE_WORDS:
            tok = tok[:-1]
        types.append(" ".join(tok + ["*"] * arrays))
    return "%s(%s)%s" % (m.group(1), ",".join(types), m.group(3) or "")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("tree")
    ap.add_argument("--dump", nargs="*")
    ap.add_argument("--classes")
    ap.add_argument("--out")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--nonvirtual", default="", help="comma-separated Class::Method declared virtual in the "
                    "headers but not in the binary's build")
    args = ap.parse_args()
    lay = Layout(args.tree, [x for x in args.nonvirtual.split(",") if x])
    if args.dump:
        for c in args.dump:
            for i, s in enumerate(lay.slots(c)):
                print("%s +%02X/%d %s   [%s]" % (c, 4 * i, i, s[1], s[2]))
    if args.classes:
        bin_classes = json.load(open(args.classes))
        # Identical-COMDAT-folded stubs (empty bodies, "return field") sit in the vtables of unrelated
        # classes and are called non-virtually elsewhere: a class name on them would mislead
        refs = {}
        for full, info in bin_classes.items():
            for v in info["vtables"]:
                for f in v["slots"]:
                    refs.setdefault(f, set()).add(full)
        lines = []
        folded = []
        for full, info in sorted(bin_classes.items()):
            if not full.startswith("Sexy::"):
                continue
            short = full.split("::")[-1]
            if short not in lay.cls:
                continue
            vt = next((v for v in info["vtables"] if v["offset"] == 0), None)
            if not vt:
                continue
            table = lay.slots(short)
            ok = len(table) == len(vt["slots"])
            if args.check or not ok:
                print("%-28s computed %3d  binary %3d  %s" % (full, len(table), len(vt["slots"]), "ok" if ok else "MISMATCH"))
            if not ok:
                continue
            for i, (vname, sig, owner, key) in enumerate(table):
                if owner != short:
                    continue  # inherited slot: named by the class that defines it
                if any(full not in [c] + bin_classes[c]["bases"] for c in refs.get(vt["slots"][i], ())):
                    # shared with an unrelated class (folded stub): a neutral name replaces any earlier one
                    folded.append({"addr": vt["slots"][i], "name": "folded_stub_" + vt["slots"][i]})
                    continue
                nm = "~" + short if vname.startswith("~") else vname
                lines.append({"addr": vt["slots"][i], "name": "Sexy::%s::%s" % (short, nm.replace("~", "dtor_")),
                              "comment": "vtable slot %d: %s" % (i, sig)})
        if args.out:
            # a function shared by several slots (COMDAT folding) keeps its first name
            seen = set()
            named = {x["addr"] for x in lines}
            with open(args.out, "w") as w:
                for x in [f for f in folded if f["addr"] not in named] + lines:
                    if x["addr"] in seen:
                        continue
                    seen.add(x["addr"])
                    w.write(json.dumps(x) + "\n")
            print("wrote %d names" % len(seen))


if __name__ == "__main__":
    main()
