#!/usr/bin/env python3
"""A histogram of the random draws by their range, original against port: each GameRand::Get(range, signed) call site
counts once, grouped by its literal range (and "var" for a range that is an expression), plus the Next calls.

    python games/atomica/tools/identical/drawhist.py

A difference is a lead for the cases where the call sites are not in the same function (drawsites.py compares the
functions); a run's draws per update are in compare.py's rng_j.
"""
import collections
import glob
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
GET = re.compile(r"FUN_00425f10\((?:[^;]*?),([^,;]*?),'(\\x01|\\0)'\)")
PORT = re.compile(r"mRand\.Get\(([^,)]*(?:\([^)]*\))?[^,)]*)(?:,\s*(true|false))?\)")


def key(arg, signed):
    arg = arg.strip()
    if re.fullmatch(r'0x[0-9a-f]+', arg):
        arg = str(int(arg, 16))
    return (arg if re.fullmatch(r'\d+', arg) else 'var', signed)


def main():
    o = collections.Counter()
    for l in open(os.path.join(ROOT, 're/atomica/gh/all.c'), encoding='latin1'):
        for m in GET.finditer(l):
            o[key(m.group(1), m.group(2) != '\\0')] += 1
        for m in re.finditer(r'FUN_00425e80\(', l):
            o[('NEXT', False)] += 1
    p = collections.Counter()
    for f in glob.glob(os.path.join(ROOT, 'games/atomica/src/**/*.cpp'), recursive=True):
        for l in open(f, encoding='latin1'):
            for m in PORT.finditer(l):
                p[key(m.group(1), m.group(2) == 'true')] += 1
            for m in re.finditer(r'mRand\.Next\(', l):
                p[('NEXT', False)] += 1
    print('original %d sites, port %d' % (sum(o.values()), sum(p.values())))
    for k in sorted(set(o) | set(p), key=str):
        if o[k] != p[k]:
            print('range %s signed=%s: original %d, port %d' % (k[0], k[1], o[k], p[k]))


main()
