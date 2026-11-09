"""Pairs the functions of Ancient Japan's exe that re/mahjongchina/compare/addrmap.tsv has no row for with the functions of
Ancient China's exe that no row names, by their place: the two builds keep their functions in the same order, so the
functions strictly between two mapped neighbours of one build are the ones between their images in the other. A gap that
holds as many unmapped functions on each side is paired in order; any other gap by sizes and calls (a pair stands when
the sizes agree within a few bytes, or every callee the Japan function has that the table maps is a callee of the
China one). The new rows are written to stdout as "japan<TAB>china<TAB>func<TAB>gap" for the table to take, with a
summary on stderr.

    python -I games/mahjongchina/tools/pairgaps.py [--write]      --write appends the rows to addrmap.tsv
"""
import json
import sys

ROOT = 're/'
MAP = ROOT + 'mahjongchina/compare/addrmap.tsv'
JAPAN_RANGES = [(0x401000, 0x4175b0), (0x41e240, 0x4319e0)]
CHINA_RANGES = [(0x401000, 0x4171f0), (0x41dea0, 0x4374f0)]
# Pairs read from the two exports by hand, which replace the table's row for the same Japan function: TextEdit::Update
# (0x428010; China's compiler inlined Object::IsActive into it), and the destructor of the tip box (0x408c80; the table
# paired it with another destructor of the same shape)
MANUAL = {0x428010: 0x431ac0, 0x408c80: 0x408b00}


def load(game, ranges):
    out = {}
    for l in open(ROOT + game + '/gh/functions.jsonl'):
        f = json.loads(l)
        a = int(f['addr'], 16)
        if any(lo <= a < hi for lo, hi in ranges):
            out[a] = f
    return out


def main():
    J = load('mahjongjapan', JAPAN_RANGES)
    C = load('mahjongchina', CHINA_RANGES)
    m = {}
    for l in open(MAP):
        p = l.rstrip('\n').split('\t')
        if len(p) > 2 and p[2] == 'func':
            m[int(p[0], 16)] = int(p[1], 16)
    m.update(MANUAL)
    taken = set(m.values())
    mapped = sorted(a for a in m if a in J)
    new = {}
    ja = sorted(J)
    # the gaps: consecutive mapped Japan functions whose images are in the same order
    for i in range(len(mapped) - 1):
        p, n = mapped[i], mapped[i + 1]
        if not m[p] < m[n]:
            continue
        jg = [a for a in ja if p < a < n and a not in m]
        cg = [a for a in sorted(C) if m[p] < a < m[n] and a not in taken]
        if not jg or not cg:
            continue
        if len(jg) == len(cg):
            for a, b in zip(jg, cg):
                new[a] = b
            continue
        # unequal gaps: greedy in order, taking the first China function that fits each Japan one
        k = 0
        for a in jg:
            sa = J[a]['size']
            for t in range(k, len(cg)):
                b = cg[t]
                sb = C[b]['size']
                callee_j = [m[int(x, 16)] for x in J[a]['callees'] if x[0] != 'E' and int(x, 16) in m]
                callee_c = [int(x, 16) for x in C[b]['callees'] if x[0] != 'E']
                if sa == sb or (abs(sa - sb) <= 4 and min(sa, sb) > 16) or (callee_j and all(x in callee_c for x in callee_j)):
                    new[a] = b
                    k = t + 1
                    break
    # a pair stands when the sizes agree within 4 bytes, or when the callees the table maps are the China one's callees and
    # the sizes are within 30%; two pairs for one China function leave the nearer in size
    def stands(a, b):
        sa, sb = J[a]['size'], C[b]['size']
        if sa == sb or (abs(sa - sb) <= 4 and min(sa, sb) > 16):
            return True
        callee_j = [m[int(x, 16)] for x in J[a]['callees'] if x[0] != 'E' and int(x, 16) in m]
        callee_c = [int(x, 16) for x in C[b]['callees'] if x[0] != 'E']
        if bool(callee_j) and all(x in callee_c for x in callee_j) and max(sa, sb) <= 1.3 * min(sa, sb):
            return True
        # or the functions that call it, at least two, are the ones that call the other (the compiler inlined more or less)
        caller_j = [m[int(x, 16)] for x in J[a]['callers'] if x[0] != 'E' and int(x, 16) in m]
        caller_c = [int(x, 16) for x in C[b]['callers'] if x[0] != 'E']
        return len(caller_j) >= 2 and all(x in caller_c for x in caller_j)
    new = {a: b for a, b in new.items() if stands(a, b)}
    best = {}
    for a, b in new.items():
        if b not in best or abs(J[a]['size'] - C[b]['size']) < abs(J[best[b]]['size'] - C[b]['size']):
            best[b] = a
    new = {a: b for b, a in best.items()}
    # then by the call graph, until nothing more pairs: a Japan function that a mapped function calls is a function that the
    # image of that function calls, and the same by the functions it calls. Candidates not yet named, of a size within 30%
    # (4 bytes for a small one); the one candidate that is left, or the only one of the same size
    def num(x):
        return int(x, 16) if x[0] != 'E' else None
    jcallers = {a: [num(x) for x in J[a]['callers'] if num(x) is not None] for a in J}
    jcallees = {a: [num(x) for x in J[a]['callees'] if num(x) is not None] for a in J}
    ccallers = {b: set(num(x) for x in C[b]['callers'] if num(x) is not None) for b in C}
    ccallees = {b: set(num(x) for x in C[b]['callees'] if num(x) is not None) for b in C}
    image = dict(m)
    image.update(new)
    named = set(image.values())
    changed = True
    rounds = 0
    while changed:
        changed = False
        rounds += 1
        for a in ja:
            if a in image:
                continue
            cand = None
            for q in jcallers[a]:
                if q in image and image[q] in C:
                    cs = set(x for x in ccallees[image[q]] if x in C and x not in named)
                    cand = cs if cand is None else cand & cs
            for r in jcallees[a]:
                if r in image and image[r] in C:
                    cs = set(x for x in ccallers[image[r]] if x in C and x not in named)
                    cand = cs if cand is None else cand & cs
            if not cand:
                continue
            sa = J[a]['size']
            close = [b for b in cand if C[b]['size'] == sa or (abs(C[b]['size'] - sa) <= 4 and sa > 16)]
            loose = [b for b in cand if max(C[b]['size'], sa) <= 1.3 * min(C[b]['size'], sa)]
            nc = len([q for q in jcallers[a] if q in image])
            pick = close[0] if len(close) == 1 else (loose[0] if len(cand) == 1 and len(loose) == 1 else None)
            if pick is None and len(cand) == 1 and nc >= 2:
                pick = next(iter(cand))
            if pick is not None:
                new[a] = pick
                image[a] = pick
                named.add(pick)
                changed = True
    print('call graph pass: %d rounds' % rounds, file=sys.stderr)
    # report, with the sizes that disagree by more than 4 bytes marked
    bad = 0
    for a in sorted(new):
        sa, sb = J[a]['size'], C[new[a]]['size']
        if abs(sa - sb) > 4:
            bad += 1
        print('%x\t%x\tfunc\tgap' % (a, new[a]))
    unmapped = [a for a in ja if a not in m and a not in new]
    print('%d pairs (%d with sizes more than 4 bytes apart); %d Japan functions still unmapped; %d China functions unnamed'
          % (len(new), bad, len(unmapped), len([b for b in C if b not in taken and b not in new.values()])), file=sys.stderr)
    if '--write' in sys.argv:
        rows = [l for l in open(MAP, newline='') if int(l.split('\t')[0], 16) not in MANUAL or l.split('\t')[2] != 'func']
        with open(MAP, 'w', newline='') as f:
            f.writelines(rows)
            for a in sorted(MANUAL):
                f.write('%x\t%x\tfunc\tmanual\n' % (a, MANUAL[a]))
            for a in sorted(new):
                f.write('%x\t%x\tfunc\tgap\n' % (a, new[a]))


main()
