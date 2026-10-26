#!/usr/bin/env python3
"""Dump the game region (0x401000-0x413fff) of Ghidra's decompilation into re/alchemy/game.c, then
python games/alchemy/tools/clean_decomp.py re/alchemy/game.c re/alchemy/game_clean.c
(run from the repository root after tools/ghidra_import.sh and the symbol apply, see WORKING_NOTES.md)."""
import re

t = open('re/alchemy/gh/all.c', encoding='utf-8', errors='replace').read()
parts = re.split(r'(?m)^(?=// [0-9a-f]{8}  )', t)
out = []
for p in parts:
    m = re.match(r'// ([0-9a-f]{8})  ', p)
    if m and 0x401000 <= int(m.group(1), 16) < 0x414000:
        out.append(p)
open('re/alchemy/game.c', 'w', encoding='utf-8').write(''.join(out))
print(len(out), 'functions')
