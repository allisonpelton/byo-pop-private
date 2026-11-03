#!/usr/bin/env python3
"""Place each class of BejeweledTwist.exe by its vtable slots (re/bejeweledtwist/gh/vtables.jsonl): a slot address that
appears in only one class's vtable is that class's own method, and Visual C++ keeps an object file's functions together,
so the middle of a class's own methods says where its source file sits. Prints middle, class, count and range.

    python games/bejeweledtwist/tools/classmap.py [substring]
"""
import collections
import json
import statistics
import sys

own = collections.defaultdict(set)
for line in open('re/bejeweledtwist/gh/vtables.jsonl', encoding='utf-8'):
    j = json.loads(line)
    for s in j['slots']:
        own[j['class']].add(int(s['addr'], 16))
count = collections.Counter(a for v in own.values() for a in v)
rows = []
for c, v in own.items():
    u = sorted(a for a in v if count[a] == 1)
    if u and (len(sys.argv) < 2 or sys.argv[1] in c):
        rows.append((statistics.median(u), c, len(u), u[0], u[-1]))
for m, c, n, a, b in sorted(rows):
    print('%06x %-40s own=%3d %06x-%06x' % (int(m), c, n, a, b))
