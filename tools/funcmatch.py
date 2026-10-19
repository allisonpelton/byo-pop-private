#!/usr/bin/env python3
"""Match the functions of two builds made with the same compiler and carry names from one to the other.

Two PopCap games linked with the same compiler share most of their framework and library code byte for
byte, apart from the addresses they refer to (BookWorm Deluxe 2004 and Big Money Deluxe 2005 are both
VS.NET 2002 builds). This matches the functions of exe A (unnamed) with those of exe B (named in its
Ghidra export) and writes B's names for A's functions:

    python tools/funcmatch.py A.exe A/functions.jsonl B.exe B/functions.jsonl <out.jsonl>
        [--symbols B_symbols.jsonl ...]   extra names (and comments) for B, e.g. vtable slot names
        [--fuzzy]                         also pair changed functions of the same RTTI class (see below)

Function bodies come from Ghidra's export (functions.jsonl: addr, size). Each body is masked: every
4-byte value inside the image's address range and every rel32 after E8/E9 becomes 0, so a function that
only moved, or calls functions that moved, hashes the same in both builds.

Passes:
  exact  bodies whose masked bytes occur exactly once in A and once in B
  call   the n-th call (E8) of a matched pair whose bodies are identical points to a function in each
         build: those two are the same function, even if their bodies changed. Pointers to functions at
         the same place in identical bodies (push offset, mov [reg], offset) pair the same way.
  group  bodies that occur k times in both builds, paired in address order when the link order agrees
         (only for k <= 4, and only when every pairing is consistent with already matched callers)
  fuzzy  (--fuzzy) unmatched functions whose Ghidra namespaces name the same class in both builds
         (e.g. RTTI vtable slots Sexy::Widget::vf13), paired by the best 3-gram similarity of their
         masked bytes when it is clearly better than the second best (a hint, marked as such)

Output, one line per A function matched: {"a", "b", "how", "name", "comment", "score"}. The name is B's
when B's is meaningful (not FUN_/thunk_FUN_/Catch@); "how" says which pass made the match. With
--apply-names <file> a second file is written in ApplySymbols.java format (addr + name + comment).
"""
import argparse
import bisect
import collections
import hashlib
import json
import struct


class Exe:
    def __init__(self, path, functions):
        d = self.d = open(path, "rb").read()
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        optsz = struct.unpack_from("<H", d, pe + 20)[0]
        self.base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        self.size = struct.unpack_from("<I", d, pe + 24 + 56)[0]
        self.secs = []
        off = pe + 24 + optsz
        for _ in range(nsec):
            vsz, va, rsz, rp = struct.unpack_from("<IIII", d, off + 8)
            self.secs.append((self.base + va, rp, rsz))
            off += 40
        self.funcs = {}
        for line in open(functions, encoding="utf-8"):
            f = json.loads(line)
            if f.get("external") or f.get("thunk"):
                continue
            a = int(f["addr"], 16)
            self.funcs[a] = f
        self.starts = sorted(self.funcs)
        self.startset = set(self.starts)
        self.bodies = {}
        self.calls = {}
        self.refs = {}
        for a in self.starts:
            self.load(a)

    def raw(self, va, n):
        for sva, rp, rs in self.secs:
            if sva <= va < sva + rs:
                return self.d[rp + va - sva:rp + va - sva + n]
        return b""

    def load(self, a):
        n = self.funcs[a].get("size") or 0
        # Ghidra's size counts every body range; never read into the next function
        i = bisect.bisect_right(self.starts, a)
        if i < len(self.starts):
            n = min(n, self.starts[i] - a) if n else self.starts[i] - a
        b = bytearray(self.raw(a, n))
        while b and b[-1] in (0xCC, 0x90):
            b.pop()
        calls, refs = [], []
        i = 0
        while i + 4 <= len(b):
            v = struct.unpack_from("<I", b, i)[0]
            if i > 0 and b[i - 1] in (0xE8, 0xE9):
                t = (a + i + 4 + v) & 0xFFFFFFFF
                if t in self.startset:
                    calls.append((i, t))
                b[i:i + 4] = b"\0\0\0\0"
                i += 4
            elif self.base <= v < self.base + self.size:
                if v in self.startset:
                    refs.append((i, v))
                b[i:i + 4] = b"\0\0\0\0"
                i += 4
            else:
                i += 1
        self.bodies[a] = bytes(b)
        self.calls[a] = calls
        self.refs[a] = refs

    def name(self, a):
        return self.funcs[a]["full"] if a in self.funcs else None


def meaningful(name):
    if not name:
        return False
    last = name.split("::")[-1]
    return not (last.startswith("FUN_") or last.startswith("thunk_FUN_") or last.startswith("Catch@")
                or last.startswith("Unwind@") or last.startswith("LAB_"))


def shingles(b):
    return {b[i:i + 3] for i in range(len(b) - 2)}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("a_exe")
    ap.add_argument("a_functions")
    ap.add_argument("b_exe")
    ap.add_argument("b_functions")
    ap.add_argument("out")
    ap.add_argument("--symbols", nargs="*", default=[])
    ap.add_argument("--fuzzy", action="store_true")
    ap.add_argument("--apply-names")
    args = ap.parse_args()

    A = Exe(args.a_exe, args.a_functions)
    B = Exe(args.b_exe, args.b_functions)
    bnames = {a: f["full"] for a, f in B.funcs.items()}
    bcomments = {}
    for path in args.symbols:
        for line in open(path, encoding="utf-8"):
            if not line.strip():
                continue
            try:
                s = json.loads(line)
            except ValueError:
                # ApplySymbols.java's reader takes a lone backslash literally ("properties\default.xml")
                s = json.loads(line.replace("\\", "\\\\"))
            if "addr" in s and "name" in s:
                a = int(s["addr"], 16)
                bnames[a] = s["name"]
                if s.get("comment"):
                    bcomments[a] = s["comment"]

    def hashes(E):
        h = collections.defaultdict(list)
        for a, b in E.bodies.items():
            if len(b) >= 6:
                h[hashlib.md5(b).digest()].append(a)
        return h

    ha, hb = hashes(A), hashes(B)
    match, rev, how = {}, {}, {}

    def add(a, b, why):
        if a in match or b in rev:
            return False
        match[a], rev[b], how[a] = b, a, why
        return True

    for k, la in ha.items():
        lb = hb.get(k)
        if lb and len(la) == 1 and len(lb) == 1:
            add(la[0], lb[0], "exact")

    def identical(a, b):
        return A.bodies[a] == B.bodies[b]

    def propagate():
        queue = list(match.items())
        while queue:
            a, b = queue.pop()
            if not identical(a, b):
                continue
            ca, cb = dict(A.calls[a]), dict(B.calls[b])
            ra, rb = dict(A.refs[a]), dict(B.refs[b])
            for pos in set(ca) & set(cb):
                if add(ca[pos], cb[pos], "call"):
                    queue.append((ca[pos], cb[pos]))
            for pos in set(ra) & set(rb):
                if add(ra[pos], rb[pos], "call"):
                    queue.append((ra[pos], rb[pos]))

    propagate()
    # Groups of identical bodies (small helpers, folded stubs): pair in address order when the counts agree
    changed = True
    while changed:
        changed = False
        for k, la in ha.items():
            lb = hb.get(k)
            if not lb or len(la) != len(lb) or len(la) > 4:
                continue
            fa = [a for a in sorted(la) if a not in match]
            fb = [b for b in sorted(lb) if b not in rev]
            if fa and len(fa) == len(fb):
                for a, b in zip(fa, fb):
                    changed |= add(a, b, "group")
        if changed:
            propagate()

    if args.fuzzy:
        def cls(E, a):
            ns = E.funcs[a]["ns"]
            return ns if ns.startswith("Sexy::") or ns.startswith("std::") else None
        bycls = collections.defaultdict(list)
        for b in B.starts:
            c = cls(B, b)
            if c and b not in rev:
                bycls[c].append(b)
        sh = {}
        for a in A.starts:
            c = cls(A, a)
            if not c or a in match or len(A.bodies[a]) < 16:
                continue
            cands = [b for b in bycls.get(c, []) if b not in rev]
            if not cands:
                continue
            sa = shingles(A.bodies[a])
            scored = []
            for b in cands:
                if b not in sh:
                    sh[b] = shingles(B.bodies[b])
                sb = sh[b]
                j = len(sa & sb) / max(1, len(sa | sb))
                scored.append((j, b))
            scored.sort(reverse=True)
            best = scored[0]
            second = scored[1][0] if len(scored) > 1 else 0.0
            if best[0] >= 0.5 and best[0] - second >= 0.1:
                if add(a, best[1], "fuzzy"):
                    how[a] = "fuzzy:%.2f" % best[0]

    out = []
    for a in sorted(match):
        b = match[a]
        n = bnames.get(b)
        rec = {"a": "%08x" % a, "b": "%08x" % b, "how": how[a], "a_name": A.name(a)}
        if meaningful(n):
            rec["name"] = n
        if b in bcomments:
            rec["comment"] = bcomments[b]
        out.append(rec)
    with open(args.out, "w", encoding="utf-8") as f:
        for r in out:
            f.write(json.dumps(r) + "\n")
    if args.apply_names:
        with open(args.apply_names, "w", encoding="utf-8") as f:
            for r in out:
                if "name" not in r:
                    continue
                s = {"addr": r["a"], "name": r["name"]}
                c = "Big Money %s (%s)" % (r["b"], r["how"])
                if "comment" in r:
                    c += ": " + r["comment"]
                s["comment"] = c
                f.write(json.dumps(s) + "\n")
    cnt = collections.Counter(r["how"].split(":")[0] for r in out)
    named = sum(1 for r in out if "name" in r)
    print("A functions %d, B functions %d, matched %d (%s), with a B name %d" % (
        len(A.starts), len(B.starts), len(out), ", ".join("%s %d" % kv for kv in sorted(cnt.items())), named))


if __name__ == "__main__":
    main()
