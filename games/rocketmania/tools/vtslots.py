#!/usr/bin/env python3
"""Print which virtual slots a class overrides compared with its base (re/rocketmania/rtti.classes.json),
with the sizes from the Ghidra export:  python games/rocketmania/tools/vtslots.py Board [Widget]"""
import json, sys
c = json.load(open('re/rocketmania/rtti.classes.json'))
fs = {f['addr']: f for f in map(json.loads, open('re/rocketmania/gh/functions.jsonl'))}


def find(name):
    return next(k for k in c if k == name or k == 'Sexy::' + name)


def main_vt(name):
    return next(v['slots'] for v in c[find(name)]['vtables'] if v['offset'] == 0)


cls = sys.argv[1]
base = sys.argv[2] if len(sys.argv) > 2 else 'Widget'
a, b = main_vt(cls), main_vt(base)
for i, x in enumerate(a):
    if i >= len(b) or x != b[i]:
        print(i, hex(i * 4), x, fs.get(x, {}).get('size'), '(base %s)' % (b[i] if i < len(b) else '-'))
