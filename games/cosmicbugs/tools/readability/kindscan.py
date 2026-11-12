#!/usr/bin/env python3
"""Finds the integer literals that stand for a kind or a state (docs/CODE_STANDARDS.md, rule 7), from clang's AST.

    python kindscan.py dump              writes build/ast/<unit>.json for each unit of the game (clang -ast-dump=json)
    python kindscan.py scan [members]    lists the literals compared with, assigned to or switched on a member named in
                                         `members` (default mKind,mState), as (struct, member): value -> places

A place is a literal in the game's own files in one of these forms, with the member's struct taken from the type of the
expression it is read from (a Cell*, an Enemy&, ...):  member == N, member != N, member < N (and the other orders),
member = N, and `case N:` in a `switch (member)`.  `rewrite <file>` reads a rules file and replaces them: see rewrite().
"""
import collections, glob, json, os, re, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..", ".."))
LLVM = os.environ.get("LLVM_MINGW_ROOT", "E:/retools/llvm-mingw-20261006-ucrt-x86_64")
GAME = "cosmicbugs"
AST = os.path.join(ROOT, "build", "ast")


def units():
    db = json.load(open(os.path.join(ROOT, "build", "native", "compile_commands.json")))
    seen = {}
    for e in db:
        f = e["file"].replace("\\", "/")
        if "/games/" + GAME + "/" in f and "/src/" in f:
            seen[f] = e
    return seen


def dump():
    os.makedirs(AST, exist_ok=True)
    for f, e in sorted(units().items()):
        cmd = e["command"].split()
        flags = [a for a in cmd if a.startswith("-I") or a.startswith("-D") or a.startswith("-std")]
        out = os.path.join(AST, os.path.basename(f) + ".json")
        with open(out, "wb") as o:
            subprocess.run([LLVM + "/bin/i686-w64-mingw32-clang++.exe"] + flags +
                           ["-fsyntax-only", "-Xclang", "-ast-dump=json", f], stdout=o, stderr=subprocess.DEVNULL)
        print(os.path.basename(f), os.path.getsize(out))


ORIGIN = {}      # id of a variable -> (struct, member) it was initialised from
ARGS = {}        # function name -> {argument index: "Struct.member"} (RULE_ARGS below)


def ref_name(e):
    """The member's or the variable's name of an expression, or None. A variable initialised from a member is that member."""
    if e["kind"] == "MemberExpr":
        return e["name"]
    if e["kind"] == "DeclRefExpr" and e.get("referencedDecl", {}).get("kind") == "VarDecl":
        o = ORIGIN.get(e["referencedDecl"]["id"])
        return o[1] if o else e["referencedDecl"]["name"]
    return None


def base_struct(expr):
    if expr["kind"] == "DeclRefExpr":
        o = ORIGIN.get(expr.get("referencedDecl", {}).get("id"))
        return o[0] if o else "(var)"
    """The struct a member expression reads from: the type of its base expression, without pointers or qualifiers."""
    t = strip(expr["inner"][0]).get("type", {})
    q = t.get("desugaredQualType") or t.get("qualType") or ""
    q = re.sub(r"\b(const|volatile|struct|class)\b", "", q).replace("*", "").replace("&", "").strip()
    return q.split("::")[-1]


def strip(e):
    while e["kind"] in ("ImplicitCastExpr", "ParenExpr", "ConstantExpr", "CStyleCastExpr", "CXXStaticCastExpr") and "inner" in e:
        e = e["inner"][0]
    return e


def literal(e):
    e = strip(e)
    if e["kind"] == "IntegerLiteral":
        return e
    if e["kind"] == "UnaryOperator" and e.get("opcode") == "-" and strip(e["inner"][0])["kind"] == "IntegerLiteral":
        return None
    return None


LAST_FILE = [None]


def touch(loc):
    """clang prints a location's file only when it differs from the one before it, in the order of the dump."""
    if not isinstance(loc, dict):
        return
    if "file" in loc:
        LAST_FILE[0] = loc["file"]
    for key in ("spellingLoc", "expansionLoc"):
        if key in loc:
            touch(loc[key])


def walk(node, members, out, switch_member=None):
    kind = node.get("kind")
    touch(node.get("loc"))
    rng = node.get("range", {})
    touch(rng.get("begin"))
    here = LAST_FILE[0]
    touch(rng.get("end"))
    if kind == "VarDecl" and node.get("inner"):
        init = strip(node["inner"][-1])
        if init["kind"] == "MemberExpr" and init["name"] in members:
            ORIGIN[node["id"]] = (base_struct(init), init["name"])
    if kind == "CallExpr" and node["inner"]:
        callee = strip(node["inner"][0])
        if callee["kind"] == "DeclRefExpr":
            want = ARGS.get(callee["referencedDecl"]["name"])
            if want:
                for idx, target in want.items():
                    if idx + 1 < len(node["inner"]):
                        lit = literal(node["inner"][idx + 1])
                        if lit:
                            st, mem = target.split(".")
                            out.append((st, mem, int(lit["value"]), lit["range"]["begin"], "arg", here))
    if kind == "BinaryOperator" and node.get("opcode") in ("==", "!=", "<", ">", "<=", ">=", "="):
        a, b = node["inner"][0], node["inner"][1]
        for m, l in ((a, b), (b, a)):
            ms = strip(m)
            if ref_name(ms) in members:
                lit = literal(l)
                if lit and (node["opcode"] != "=" or m is a):
                    out.append((base_struct(ms), ref_name(ms), int(lit["value"]), lit["range"]["begin"], node["opcode"], here))
    if kind == "SwitchStmt":
        cond = None
        for c in node["inner"][:-1]:
            if c["kind"] != "DeclStmt":
                cond = strip(c)
                break
        if cond is not None and ref_name(cond) in members:
            switch_member = (base_struct(cond), ref_name(cond))
        else:
            switch_member = None
    if kind == "CaseStmt" and switch_member:
        lit = literal(node["inner"][0])
        if lit:
            out.append((switch_member[0], switch_member[1], int(lit["value"]), lit["range"]["begin"], "case", here))
    for c in node.get("inner", []):
        walk(c, members, out, switch_member)


def scan(members):
    allhits = collections.defaultdict(set)
    for f in sorted(units()):
        path = os.path.join(AST, os.path.basename(f) + ".json")
        if os.path.getsize(path) > 300e6:      # the host and main.cpp: the framework's headers, no kinds
            continue
        tu = json.load(open(path))
        out = []
        LAST_FILE[0] = None
        walk(tu, members, out)
        for st, mem, val, loc, op, fl in out:
            if fl and ("/games/" + GAME + "/") in fl.replace("\\", "/"):
                allhits[(st, mem, val)].add((fl.replace("\\", "/"), loc.get("offset"), loc.get("tokLen"), op))
        print(os.path.basename(f), len(out), file=sys.stderr)
    return allhits


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(2)
    if sys.argv[1] == "dump":
        dump()
    elif sys.argv[1] == "places":
        # places <Struct.member> [value ...]: each place as file:line: text
        st, mem = sys.argv[2].split(".")
        want = set(int(v, 0) for v in sys.argv[3:])
        hits = scan({mem})
        shown = set()
        for (s2, m2, val), places in sorted(hits.items()):
            if s2 != st or (want and val not in want):
                continue
            for fl, off, ln, op in sorted(places):
                data = open(fl, "rb").read()
                line = data.count(b"\n", 0, off) + 1
                text = data.split(b"\n")[line - 1].decode("latin-1").strip()
                print("%s:%d: [%d] %s" % (os.path.basename(fl), line, val, text[:130]))
    elif sys.argv[1] == "rewrite":
        # rewrite <rules file>: Struct.member value NAME, one a line; '#' starts a comment
        rules = {}
        for line in open(sys.argv[2]):
            line = line.split("#")[0].strip()
            if line.startswith("arg "):
                # arg <function> <index of the argument, from 0> <Struct.member>
                _, fn, idx, target = line.split()
                ARGS.setdefault(fn, {})[int(idx)] = target
            elif line:
                key, val, name = line.split()
                rules[(key, int(val, 0))] = name
        members = set(k.split(".")[1] for k, _ in rules)
        hits = scan(members)
        edits = collections.defaultdict(dict)
        for (st, mem, val), places in hits.items():
            name = rules.get(("%s.%s" % (st, mem), val))
            if name:
                for fl, off, ln, op in places:
                    edits[fl][off] = (ln, name)
        total = 0
        for fl, es in edits.items():
            data = bytearray(open(fl, "rb").read())
            for off in sorted(es, reverse=True):
                ln, name = es[off]
                data[off:off + ln] = name.encode()
                total += 1
            open(fl, "wb").write(bytes(data))
        print("%d literals replaced in %d files" % (total, len(edits)))
    elif sys.argv[1] == "scan":
        members = set((sys.argv[2] if len(sys.argv) > 2 else "mKind,mState").split(","))
        hits = scan(members)
        by = collections.defaultdict(dict)
        for (st, mem, val), places in hits.items():
            by[(st, mem)][val] = places
        for (st, mem), vals in sorted(by.items()):
            print("%s::%s" % (st, mem))
            for val in sorted(vals):
                print("   %3d (0x%x): %d" % (val, val, len(vals[val])))
