#!/usr/bin/env python3
"""Compare Ancient China's exe with Ancient Japan's, function by function, and map Japan's addresses to China's.

The port of Ancient China starts as a copy of Ancient Japan's (games/mahjongjapan), whose comments and GT() texts
name Japan's addresses. This pairs the functions of the two exes (tools/funcmatch.py's passes, then the unmatched
functions between two matched ones paired in link order when both gaps hold as many), pairs the data the paired
functions refer to, and lists every pair whose bodies differ with a diff of the two decompilations.

    python games/mahjongchina/tools/compare_exes.py [--out re/mahjongchina/compare]

Inputs are the two Ghidra exports (re/mahjongjapan/gh, re/mahjongchina/gh) and the two exes in re/<game>/bin.
Writes, all git-ignored under --out:
    addrmap.tsv     japan  china  kind (func/data)  how       one line per address
    changed.tsv     japan  china  size_j size_c  ratio  cited  the paired functions whose masked bytes differ
    diff/<japan>_<china>.diff   a unified diff of the two decompilations, names and addresses in Japan's terms
    unmatched.tsv   the functions of each exe in the code range left without a partner
"""
import argparse
import collections
import difflib
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import funcmatch  # noqa: E402

CODE = (0x401000, 0x447000)  # the game, the C runtime and the engine in both builds


def load_exe(game, exe):
    return funcmatch.Exe(os.path.join(ROOT, "re", game, "bin", exe), os.path.join(ROOT, "re", game, "gh", "functions.jsonl"))


def in_order(anchors):
    """The longest run of matched pairs whose partners are in the same order in both builds (a few matches of
    small, alike functions land out of order and would hide the gaps around them)"""
    import bisect
    tails, tail_at, prev = [], [], [None] * len(anchors)
    for k, (_, jy) in enumerate(anchors):
        p = bisect.bisect_left(tails, jy)
        if p == len(tails):
            tails.append(jy)
            tail_at.append(k)
        else:
            tails[p], tail_at[p] = jy, k
        prev[k] = tail_at[p - 1] if p else None
    out, k = [], tail_at[-1] if tail_at else None
    while k is not None:
        out.append(anchors[k])
        k = prev[k]
    return out[::-1]


def pair_functions(c, j):
    """China -> Japan: funcmatch's exact, call and group passes, then link order inside the gaps."""
    sys.argv = ["funcmatch"]
    out = os.path.join(ROOT, "re", "mahjongchina", "funcmatch.jsonl")
    import subprocess
    subprocess.run([sys.executable, os.path.join(ROOT, "tools", "funcmatch.py"),
                    os.path.join(ROOT, "re", "mahjongchina", "bin", "MahjongEscapeAC.exe"),
                    os.path.join(ROOT, "re", "mahjongchina", "gh", "functions.jsonl"),
                    os.path.join(ROOT, "re", "mahjongjapan", "bin", "MahjongEscapeAJ.exe"),
                    os.path.join(ROOT, "re", "mahjongjapan", "gh", "functions.jsonl"), out],
                   check=True, stdout=subprocess.DEVNULL)
    pairs, how = {}, {}
    for line in open(out):
        m = json.loads(line)
        a, b = int(m["a"], 16), int(m["b"], 16)
        pairs[a], how[a] = b, m["how"]
    # link order: between two matched functions whose partners are in the same order, pair the unmatched ones when
    # both gaps hold the same number of functions
    cs = [a for a in c.starts if CODE[0] <= a < CODE[1]]
    js = [a for a in j.starts if CODE[0] <= a < CODE[1]]
    jindex = {a: i for i, a in enumerate(js)}
    taken = set(pairs.values())
    added = 1
    while added:
        added = 0
        anchors = [(i, jindex[pairs[a]]) for i, a in enumerate(cs) if a in pairs and pairs[a] in jindex]
        for (ci0, ji0), (ci1, ji1) in zip(anchors, anchors[1:]):
            if ji1 <= ji0:
                continue
            cg = [a for a in cs[ci0 + 1:ci1] if a not in pairs]
            jg = [a for a in js[ji0 + 1:ji1] if a not in taken]
            if cg and len(cg) == len(jg) and len(cs[ci0 + 1:ci1]) == len(cg) and len(js[ji0 + 1:ji1]) == len(jg):
                for a, b in zip(cg, jg):
                    pairs[a], how[a] = b, "order"
                    taken.add(b)
                    added += 1
    # the gaps left: aligned in order by the similarity of the masked bodies (3-byte shingles), pairs above 0.3
    anchors = in_order([(i, jindex[pairs[a]]) for i, a in enumerate(cs) if a in pairs and pairs[a] in jindex])
    anchors = [(-1, -1)] + anchors + [(len(cs), len(js))]
    for (ci0, ji0), (ci1, ji1) in zip(anchors, anchors[1:]):
        if ji1 <= ji0:
            continue
        cg = [a for a in cs[ci0 + 1:ci1] if a not in pairs]
        jg = [a for a in js[ji0 + 1:ji1] if a not in taken]
        if not cg or not jg or len(cg) * len(jg) > 400000:
            continue
        sc = {a: funcmatch.shingles(c.bodies[a]) for a in cg}
        sj = {b: funcmatch.shingles(j.bodies[b]) for b in jg}

        def sim(a, b):
            x, y = sc[a], sj[b]
            return len(x & y) / max(1, len(x | y))
        n, m = len(cg), len(jg)
        best = [[0.0] * (m + 1) for _ in range(n + 1)]
        step = [[0] * (m + 1) for _ in range(n + 1)]
        for x in range(n - 1, -1, -1):
            for y in range(m - 1, -1, -1):
                s = sim(cg[x], jg[y])
                options = [(best[x + 1][y], 1), (best[x][y + 1], 2)]
                if s > 0.3:
                    options.append((best[x + 1][y + 1] + s, 0))
                best[x][y], step[x][y] = max(options)
        x = y = 0
        while x < n and y < m:
            if step[x][y] == 0:
                pairs[cg[x]], how[cg[x]] = jg[y], "similar"
                taken.add(jg[y])
                x += 1
                y += 1
            elif step[x][y] == 1:
                x += 1
            else:
                y += 1
    return pairs, how


def pair_data(c, j, pairs):
    """The absolute addresses at the same place in paired bodies of the same length, by vote."""
    votes = collections.defaultdict(collections.Counter)
    for a, b in pairs.items():
        if a not in c.bodies or b not in j.bodies or len(c.bodies[a]) != len(j.bodies[b]):
            continue
        rc, rj = c.raw(a, len(c.bodies[a])), j.raw(b, len(j.bodies[b]))
        bc, bj = c.bodies[a], j.bodies[b]
        i = 0
        while i + 4 <= len(bc):
            if bc[i:i + 4] == b"\0\0\0\0" and rc[i:i + 4] != b"\0\0\0\0" and (i == 0 or rc[i - 1] not in (0xE8, 0xE9)):
                vc, vj = struct.unpack_from("<I", rc, i)[0], struct.unpack_from("<I", rj, i)[0]
                if c.base <= vc < c.base + c.size and j.base <= vj < j.base + j.size:
                    votes[vj][vc] += 1
                    i += 4
                    continue
            i += 1
    out = {}
    for vj, cnt in votes.items():
        vc, n = cnt.most_common(1)[0]
        if n * 2 > sum(cnt.values()):
            out[vj] = vc
    return out


ADDR = re.compile(r"@\??c?[0-9a-f]+")
TOKEN = re.compile(r"\b(?:FUN|DAT|PTR_FUN|PTR_DAT|PTR|LAB|s_\w*?|u_\w*?)_([0-9a-fA-F]{8})\b|\b0x([45][0-9a-fA-F]{5})\b")


def normalize(text, tomap, own_names):
    """Addresses and Ghidra's address names in Japan's terms (tomap: this exe's address -> Japan's, None for Japan)"""
    def sub(m):
        v = int(m.group(1) or m.group(2), 16)
        if tomap is not None:
            if v not in tomap:
                return "@?c%x" % v
            v = tomap[v]
        return "@%x" % v
    text = TOKEN.sub(sub, text)
    for name, addr in own_names:
        text = re.sub(r"\b%s\b" % re.escape(name), "@%x" % addr, text)
    body = []
    started = False
    for line in text.splitlines():
        t = line.strip()
        if not t or t.startswith("/*") or t.startswith("//") or t.startswith("*"):
            continue
        # the declarations of the locals: their numbering and stack places are Ghidra's, not the code's
        if not started and t != "{" and DECL.match(t):
            continue
        if t != "{" and body:
            started = True
        body.append(t)
    # the signature's types are Ghidra's guesses: the body starts after the first brace
    if "{" in body:
        body = body[body.index("{") + 1:]
    text = " ".join(body)
    text = VARS.sub("v", text)
    text = STACK.sub("s", text)
    out = []
    for part in re.split(r"(?<=[;{}])", text):
        part = re.sub(r"\s+", " ", part).strip()
        part = re.sub(r"\s*([(),\[\]])\s*", r"\1", part)
        if part:
            out.append(part)
    return out


def shaped(statements):
    out = []
    for t in statements:
        t = re.sub(r"\((?:const )?(?:[a-z]*int\d*|undefined\d*|u?char|u?short|u?long|bool|float|double|code|byte|"
                   r"HWND|HKEY|LPCSTR|LPSTR|void|[A-Z]\w*) ?\**\)", "", t)
        t = re.sub(r"\+ ?(?:0x[0-9a-f]+|\d+)\b", "+o", t)
        t = re.sub(r"\bv =v;?", "", t)
        if t and t != "v = v;":
            out.append(t)
    return out


DECL = re.compile(r"^[A-Za-z_][\w ]*?[\s*]+\w+(\s*\[\w+\])*;$")
VARS = re.compile(r"\b(?:[a-zA-Z]{1,4}Var\d+|extraout_\w+|unaff_\w+|in_\w+|a\d+)\b")
STACK = re.compile(r"\b(?:local_\w+|\w*Stack_\w+|stack0x\w+)\b")


STACKOP = re.compile(r"^(?:(?:ADD|SUB) ESP,|RET )")


def load_insns(game):
    path = os.path.join(ROOT, "re", game, "insns.txt")
    out = collections.defaultdict(list)
    if not os.path.exists(path):
        return out
    for line in open(path):
        p = line.rstrip("\n").split(" ", 3)
        out[int(p[0], 16)].append((int(p[1], 16), p[2], p[3] if len(p) > 3 else ""))
    return out


def insn_lines(insns, start, exe, tomap):
    """An instruction per line, in Japan's terms: a jump inside the function becomes @L, an address of this exe
    Japan's (tomap None: already Japan's), an unmapped one @?"""
    end = max((a for a, _, _ in insns), default=start)

    def sub(m):
        v = int(m.group(1), 16)
        if start <= v <= end:
            return "@L"
        if tomap is not None:
            return "@%x" % tomap[v] if v in tomap else "@?"
        return "@%x" % v
    return ["%s %s" % (mn, re.sub(r"@([0-9a-f]+)", sub, ops)) for _, mn, ops in insns if mn != "NOP"]


def decomp(game, addr):
    d = os.path.join(ROOT, "re", game, "gh", "decomp")
    for sub in os.listdir(d):
        p = os.path.join(d, sub)
        if not os.path.isdir(p):
            continue
        for f in os.listdir(p):
            if f.startswith("%08x_" % addr):
                return open(os.path.join(p, f), encoding="utf-8", errors="replace").read()
    return ""


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default=os.path.join(ROOT, "re", "mahjongchina", "compare"))
    args = ap.parse_args()
    j = load_exe("mahjongjapan", "MahjongEscapeAJ.exe")
    c = load_exe("mahjongchina", "MahjongEscapeAC.exe")
    pairs, how = pair_functions(c, j)
    data = pair_data(c, j, pairs)
    voted = set(data)
    c2j = {a: b for a, b in pairs.items()}
    for vj, vc in data.items():
        c2j.setdefault(vc, vj)

    cited = set()
    for sub in ("src", "engine"):
        base = os.path.join(ROOT, "games", "mahjongjapan", sub)
        for f in os.listdir(base):
            for m in re.finditer(r"\b0x(4[0-9a-fA-F]{5})\b", open(os.path.join(base, f), encoding="utf-8").read()):
                cited.add(int(m.group(1), 16))

    os.makedirs(os.path.join(args.out, "diff"), exist_ok=True)
    jnames = [(fn["name"], a) for a, fn in j.funcs.items() if funcmatch.meaningful(fn["name"])]
    jnames.sort(key=lambda p: -len(p[0]))
    changed = [(a, b) for a, b in sorted(pairs.items(), key=lambda p: p[1])
               if CODE[0] <= b < CODE[1] and c.bodies.get(a) != j.bodies.get(b)]
    texts = {(a, b): (decomp("mahjongchina", a), decomp("mahjongjapan", b)) for a, b in changed}
    # the data of changed functions: statements that are the same but for their addresses pair those addresses
    votes = collections.defaultdict(collections.Counter)
    for a, b in changed:
        lc = normalize(texts[(a, b)][0], {}, [])
        lj = normalize(texts[(a, b)][1], None, jnames)
        mc = [ADDR.sub("@", t) for t in lc]
        mj = [ADDR.sub("@", t) for t in lj]
        for blk in difflib.SequenceMatcher(a=mj, b=mc, autojunk=False).get_matching_blocks():
            for k in range(blk.size):
                xs, ys = ADDR.findall(lj[blk.a + k]), ADDR.findall(lc[blk.b + k])
                for x, y in zip(xs, ys):
                    if y.startswith("@?c") and not x.startswith("@?"):
                        votes[int(y[3:], 16)][int(x[1:], 16)] += 1
    learned = 0
    for vc, cnt in votes.items():
        vj, n = cnt.most_common(1)[0]
        if vc not in c2j and n * 2 > sum(cnt.values()):
            c2j[vc] = vj
            data.setdefault(vj, vc)
            learned += 1
    rows, shapes = [], []
    for a, b in changed:
        lc = normalize(texts[(a, b)][0], c2j, [])
        lj = normalize(texts[(a, b)][1], None, jnames)
        sm = difflib.SequenceMatcher(a=lj, b=lc, autojunk=False)
        ratio = sm.ratio()
        if lc == lj:
            continue
        # the shape: the same statements but for field offsets, casts and the temporaries Ghidra keeps or folds
        shape = sorted(set(shaped(lj)) ^ set(shaped(lc)))
        loops = sum(1 for t in lj if t.startswith(("while", "do{", "for(")))
        name = "%x_%x.diff" % (b, a)
        with open(os.path.join(args.out, "diff", name), "w", newline="\n") as f:
            f.write("\n".join(difflib.unified_diff(lj, lc, "japan %x" % b, "china %x" % a, n=2, lineterm="")) + "\n")
        rows.append((b, a, len(j.bodies[b]), len(c.bodies[a]), ratio, b in cited, len(shape), loops))
        sj, sc = shaped(lj), shaped(lc)
        only_j = [t for t in sj if t not in set(sc)]
        only_c = [t for t in sc if t not in set(sj)]
        shapes.append((b, a, only_j, only_c))
    # insns.txt: the instructions that differ, from MjInsnDump.java's dumps (tools/insndump.sh), with the fields'
    # offsets hidden and the addresses in Japan's terms; immediates.txt: the constants only one side has
    ij, ic = load_insns("mahjongjapan"), load_insns("mahjongchina")
    if ij and ic:
        irows = []
        with open(os.path.join(args.out, "insns.txt"), "w", newline="\n") as f, \
                open(os.path.join(args.out, "immediates.txt"), "w", newline="\n") as g:
            for a, b in changed:
                xj = insn_lines(ij.get(b, []), b, j, None)
                xc = insn_lines(ic.get(a, []), a, c, c2j)
                if xj == xc:
                    continue
                sm = difflib.SequenceMatcher(a=xj, b=xc, autojunk=False)
                nd = 0
                out = []
                for op, i1, i2, j1, j2 in sm.get_opcodes():
                    if op == "equal":
                        continue
                    nd += max(i2 - i1, j2 - j1)
                    out.append("  @@ japan +%d china +%d" % (i1, j1))
                    out += ["  - " + t for t in xj[i1:i2]] + ["  + " + t for t in xc[j1:j2]]
                # the stack's adjustments after a call and RET n follow the calls' arguments, not the game
                imm_j = collections.Counter(t for x in xj if not STACKOP.match(x) for t in re.findall(r"#[0-9a-f]+", x))
                imm_c = collections.Counter(t for x in xc if not STACKOP.match(x) for t in re.findall(r"#[0-9a-f]+", x))
                gone, new = imm_j - imm_c, imm_c - imm_j
                irows.append((b, a, nd, len(xj), len(xc)))
                f.write("== japan %x china %x: %d of %d/%d instructions differ\n" % (b, a, nd, len(xj), len(xc)))
                f.write("\n".join(out) + "\n")
                if gone or new:
                    g.write("%x %x  japan only %s  china only %s\n" % (
                        b, a, " ".join("%s%s" % (k, "x%d" % v if v > 1 else "") for k, v in sorted(gone.items())),
                        " ".join("%s%s" % (k, "x%d" % v if v > 1 else "") for k, v in sorted(new.items()))))
        with open(os.path.join(args.out, "insn_changed.tsv"), "w", newline="\n") as f:
            f.write("japan\tchina\tdiffering\tlen_j\tlen_c\tcited\n")
            for r in sorted(irows, key=lambda r: -r[2]):
                f.write("%x\t%x\t%d\t%d\t%d\t%s\n" % (r[0], r[1], r[2], r[3], r[4], "cited" if r[0] in cited else ""))
        print("instructions: %d changed functions differ in their instructions" % len(irows))

    # shapes.txt: for each changed function, the statements only one side has (field offsets and casts hidden)
    with open(os.path.join(args.out, "shapes.txt"), "w", newline="\n") as f:
        for b, a, only_j, only_c in shapes:
            if not only_j and not only_c:
                continue
            f.write("== japan %x china %x\n" % (b, a))
            for t in only_j:
                f.write("- %s\n" % t[:200])
            for t in only_c:
                f.write("+ %s\n" % t[:200])
    with open(os.path.join(args.out, "addrmap.tsv"), "w", newline="\n") as f:
        for a, b in sorted(pairs.items(), key=lambda p: p[1]):
            f.write("%x\t%x\tfunc\t%s\n" % (b, a, how[a]))
        for vj, vc in sorted(data.items()):
            if vj not in j.funcs:
                f.write("%x\t%x\tdata\t%s\n" % (vj, vc, "vote" if vj in voted else "statement"))

    with open(os.path.join(args.out, "changed.tsv"), "w", newline="\n") as f:
        f.write("japan\tchina\tsize_j\tsize_c\tratio\tshape\tloops\tcited\n")
        for r in sorted(rows, key=lambda r: (r[6] == 0, -r[6] * (1 + r[7]))):
            f.write("%x\t%x\t%d\t%d\t%.3f\t%d\t%d\t%s\n" % (r[0], r[1], r[2], r[3], r[4], r[6], r[7],
                                                          "cited" if r[5] else ""))
    taken = set(pairs.values())
    with open(os.path.join(args.out, "unmatched.tsv"), "w", newline="\n") as f:
        for a in j.starts:
            if CODE[0] <= a < CODE[1] and a not in taken:
                f.write("japan\t%x\t%d\t%s\t%s\n" % (a, len(j.bodies[a]), j.funcs[a]["name"], "cited" if a in cited else ""))
        for a in c.starts:
            if CODE[0] <= a < CODE[1] and a not in pairs:
                f.write("china\t%x\t%d\t%s\n" % (a, len(c.bodies[a]), c.funcs[a]["name"]))
    inrange = [a for a in j.starts if CODE[0] <= a < CODE[1]]
    print("japan functions in the code range %d, paired %d (%s); data pairs %d (%d from statements); changed (decompilation differs) %d, "
          "of them cited by the port %d; cited functions unpaired %d" % (
              len(inrange), sum(1 for a in inrange if a in taken),
              ", ".join("%s %d" % kv for kv in sorted(collections.Counter(how.values()).items())),
              len(data), learned, len(rows), sum(1 for r in rows if r[5]),
              sum(1 for a in cited if a in j.funcs and a not in taken)))


main()
