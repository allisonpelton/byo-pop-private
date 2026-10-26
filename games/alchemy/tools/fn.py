#!/usr/bin/env python3
"""Print functions of re/alchemy/game_clean.c by address: python fn.py 402fcf 40400d ..."""
import re, sys
t = open('re/alchemy/game_clean.c', encoding='utf-8').read()
parts = re.split(r'(?m)^(?=// [0-9a-f]{8}  )', t)
want = set(a.lower().zfill(8) for a in sys.argv[1:])
for p in parts:
    m = re.match(r'// ([0-9a-f]{8})  ', p)
    if m and m.group(1) in want:
        print(p.strip()); print()
