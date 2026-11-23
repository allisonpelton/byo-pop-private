#!/usr/bin/env python3
"""Pair the engine functions of Feeding Frenzy's exe with Word Harmony's, beyond tools/funcmatch.py's byte matches.

The two exes hold different builds of Sprout's engine (docs/ENGINES.md, "Sprout"): only about a fifth of its function
bodies are the same after masking addresses, so funcmatch pairs few of them. Starting from its pairs, this adds:

  str    a string that exactly one function uses in each exe, when the two functions' sets of strings agree
  vtbl   two vtables of the same length with at least two slots already paired at the same index and none paired
         elsewhere: every slot pairs
  call   the n-th direct call of a paired function, when both have the same number of calls and the callees' sizes
         are alike

repeated until a round adds nothing. Every pair stays one to one. Output: the funcmatch format, one line a Pizza
Frenzy function ({"a", "b", "how", "name"}), names from a symbol file of Word Harmony's port (port_names.py).

    python -I games/feedingfrenzy/tools/enginematch.py <match.jsonl> <names.jsonl> <out.jsonl> <apply.jsonl>

Everything it reads and writes is in the main checkout's git-ignored re/.
"""
import json
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
COMMON = subprocess.check_output(["git", "rev-parse", "--git-common-dir"], cwd=HERE, text=True).strip()
MAIN = os.path.dirname(os.path.abspath(os.path.join(HERE, COMMON)))
RE = os.path.join(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], cwd=HERE, text=True).strip(), "re")


class Exe:
    def __init__(self, exe, functions, vtables):
        self.data = open(exe, "rb").read()
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        n = struct.unpack_from("<H", self.data, pe + 6)[0]
        opt = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.base = struct.unpack_from("<I", self.data, pe + 24 + 28)[0]
        self.secs = []
        s = pe + 24 + opt
        for i in range(n):
            vs, rva, rs, ro = struct.unpack_from("<IIII", self.data, s + i * 40 + 8)
            self.secs.append((self.base + rva, max(vs, rs), ro))
        self.funcs = {}
        for line in open(functions, encoding="utf-8"):
            f = json.loads(line)
            if f.get("external"):
                continue
            self.funcs[int(f["addr"], 16)] = f
        self.vtables = []
        for line in open(vtables):
            cols = line.split()
            self.vtables.append((int(cols[0], 16), [int(c, 16) for c in cols[1:]]))
        self.calls = {}

    def offset(self, va):
        for lo, size, raw in self.secs:
            if lo <= va < lo + size:
                return raw + va - lo
        return None

    def call_list(self, a):
        """The direct calls (E8 rel32) inside the function, in order, to the starts of known functions."""
        if a in self.calls:
            return self.calls[a]
        f = self.funcs[a]
        off = self.offset(a)
        out = []
        size = f.get("size") or 0
        i = 0
        while off is not None and i + 5 <= size:
            if self.data[off + i] == 0xE8:
                t = (a + i + 5 + struct.unpack_from("<i", self.data, off + i + 1)[0]) & 0xFFFFFFFF
                if t in self.funcs:
                    out.append(t)
                    i += 5
                    continue
            i += 1
        self.calls[a] = out
        return out

    def strings(self, a):
        return frozenset(str(s) for s in self.funcs[a].get("strings") or [] if len(str(s)) >= 3)


def main():
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    match_path, names_path, out_path, apply_path = sys.argv[1:]
    ref = os.environ.get("REF", "pizzafrenzy")   # the reference exe: pizzafrenzy, wordharmony or igglepop
    refexe = {"pizzafrenzy": "PizzaFrenzyGame.exe", "wordharmony": "WordHarmonyGame.exe", "igglepop": "IgglePopGame.exe"}[ref]
    pf = Exe(os.path.join(RE, "feedingfrenzy", "bin", "FeedingFrenzy.exe"),
             os.path.join(RE, "feedingfrenzy", "gh", "functions.jsonl"), os.path.join(RE, "feedingfrenzy", "vtables.txt"))
    wh = Exe(os.path.join(RE, ref, "bin", refexe),
             os.path.join(RE, ref, "gh", "functions.jsonl"), os.path.join(RE, ref, "vtables.txt"))
    names = {int(j["addr"], 16): j["name"] for j in map(json.loads, open(names_path, encoding="utf-8"))}
    a2b, b2a, how = {}, {}, {}
    for r in map(json.loads, open(match_path, encoding="utf-8")):
        a, b = int(r["a"], 16), int(r["b"], 16)
        a2b[a], b2a[b], how[a] = b, a, r["how"]

    def pair(a, b, kind):
        if a in a2b or b in b2a or a not in pf.funcs or b not in wh.funcs:
            return False
        a2b[a], b2a[b], how[a] = b, a, kind
        return True

    def alike(a, b):
        sa, sb = pf.funcs[a].get("size") or 1, wh.funcs[b].get("size") or 1
        return 0.5 <= sa / sb <= 2.0 or abs(sa - sb) < 24

    # str: strings used by one function in each exe
    def users(exe):
        u = {}
        for a in exe.funcs:
            for s in exe.strings(a):
                u.setdefault(s, set()).add(a)
        return u
    pf_users, wh_users = users(pf), users(wh)
    rounds = 0
    while True:
        rounds += 1
        added = 0
        for s, us in pf_users.items():
            them = wh_users.get(s)
            if len(us) == 1 and them is not None and len(them) == 1:
                a, b = next(iter(us)), next(iter(them))
                sa, sb = pf.strings(a), wh.strings(b)
                if len(sa & sb) * 2 >= max(len(sa), len(sb)) and pair(a, b, "str"):
                    added += 1
        # vtbl
        for va, ea in pf.vtables:
            best = None
            for vb, eb in wh.vtables:
                if len(ea) != len(eb):
                    continue
                same = sum(1 for x, y in zip(ea, eb) if a2b.get(x) == y)
                cross = sum(1 for x, y in zip(ea, eb) if x in a2b and a2b[x] != y and a2b[x] in eb)
                if same >= 2 and cross == 0 and (best is None or same > best[0]):
                    best = (same, eb)
            if best:
                for x, y in zip(ea, best[1]):
                    if x != y and pair(x, y, "vtbl"):
                        added += 1
        # call
        for a, b in list(a2b.items()):
            ca, cb = pf.call_list(a), wh.call_list(b)
            if len(ca) != len(cb) or not ca:
                continue
            for x, y in zip(ca, cb):
                if x not in a2b and y not in b2a and alike(x, y) and pair(x, y, "call"):
                    added += 1
        if not added:
            break
    count = {}
    with open(out_path, "w", encoding="utf-8") as out, open(apply_path, "w", encoding="utf-8") as ap:
        for a in sorted(a2b):
            b = a2b[a]
            rec = {"a": "%08x" % a, "b": "%08x" % b, "how": how[a]}
            if b in names:
                rec["name"] = names[b]
                if pf.funcs[a]["name"].startswith(("FUN_", "thunk_FUN_")):
                    ap.write(json.dumps({"addr": "%08x" % a, "name": names[b].replace("engine::", ""),
                                         "comment": "%s 0x%x %s (%s)" % (ref, b, names[b], how[a])}) + "\n")
            out.write(json.dumps(rec) + "\n")
            count[how[a]] = count.get(how[a], 0) + 1
    print("rounds %d, pairs %d %s, named %d" % (rounds, len(a2b), count, sum(1 for a in a2b if a2b[a] in names)))


if __name__ == "__main__":
    main()
