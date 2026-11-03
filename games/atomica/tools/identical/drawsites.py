#!/usr/bin/env python3
"""Where the original draws random numbers, and where the port does: the number of draw sites of each function.

    python games/atomica/tools/identical/drawsites.py

A draw is a call of GameRand::Get (0x425f10) or ::Next (0x425e80). The original's count comes from the Ghidra export
(re/atomica/gh/all.c, a function starts at a "// <address>  <name>" line); the port's from games/atomica/src, where a
function starts at a column-0 "// 0x<address>" comment. The two lists are printed side by side for the addresses where
the counts differ. A site inside a loop counts once, so a difference is a lead, not yet a fault.
"""
import glob
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))


def original():
    counts = {}
    cur = None
    for l in open(os.path.join(ROOT, 're/atomica/gh/all.c'), encoding='latin1'):
        m = re.match(r'// ([0-9a-f]{8})  ', l)
        if m:
            cur = int(m.group(1), 16)
            continue
        if cur is None:
            continue
        for _ in re.finditer(r'FUN_00425f10\(|FUN_00425e80\(', l):
            counts[cur] = counts.get(cur, 0) + 1
    return counts


def port():
    counts = {}
    for path in glob.glob(os.path.join(ROOT, 'games/atomica/src/**/*.cpp'), recursive=True):
        cur = None
        for l in open(path, encoding='latin1'):
            m = re.match(r'// 0x([0-9a-f]{6})\b', l)
            if m:
                cur = int(m.group(1), 16)
                continue
            if cur is None:
                continue
            for _ in re.finditer(r'\bmRand\.(Get|Next)\(|->mRand\.(Get|Next)\(', l):
                counts[cur] = counts.get(cur, 0) + 1
    return counts


def main():
    o = original()
    p = port()
    keys = sorted(set(o) | set(p))
    same = sum(1 for k in keys if o.get(k, 0) == p.get(k, 0))
    print('%d functions with draws; %d agree' % (len(keys), same))
    for k in keys:
        if o.get(k, 0) != p.get(k, 0):
            print('0x%06x original %d port %d' % (k, o.get(k, 0), p.get(k, 0)))


main()
