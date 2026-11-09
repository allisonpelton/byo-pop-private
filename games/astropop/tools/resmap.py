#!/usr/bin/env python3
"""Write re/astropop/resmap.json: the virtual address of each resource global (IMAGE_x, FONT_x, SOUND_x) -> its name, read
from the Extract<Group>Resources functions of the Ghidra export (callees of ExtractResourcesByName 0x422f20). decread.py
then prints the name where the decompilation has DAT_<address>.

    python games/astropop/tools/resmap.py"""
import glob, json, os, re
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
GH = os.path.join(ROOT, 're', 'astropop', 'gh')
fs = {f['addr']: f for f in map(json.loads, open(os.path.join(GH, 'functions.jsonl')))}
out = {}
for callee in fs['00422f20']['callees']:
    p = glob.glob(os.path.join(GH, 'decomp', '*', callee + '_*.c'))
    if not p:
        continue
    t = open(p[0], encoding='utf-8', errors='replace').read()
    for m in re.finditer(r'"((?:IMAGE|FONT|SOUND)_[A-Z0-9_]+)"[^\n]*\n(?:[^\n]*\n){0,3}?\s*_?(DAT_[0-9a-f]{8}) = ', t):
        out[m.group(2)[4:]] = m.group(1)
json.dump(out, open(os.path.join(ROOT, 're', 'astropop', 'resmap.json'), 'w'), indent=0)
print(len(out), 'globals')
