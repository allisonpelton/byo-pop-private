#!/usr/bin/env python3
"""Compact Ghidra's decompilation of the game region: python clean_decomp.py in.c out.c
Drops SEH bookkeeping, destructors of temporary strings and declaration lines, shortens the std::string casts."""
import re, sys
t = open(sys.argv[1], encoding='utf-8').read()
out = []
for ln in t.split('\n'):
    if re.search(r'unaff_EBP \+ -4\) = \w+;$', ln): continue
    if ln.strip() in ('FUN_004c2f74();',): continue
    if 'ExceptionList' in ln: continue
    if re.search(r"std::string::_Tidy\(.*'\x01'\);", ln): continue
    if re.match(r'^  [A-Za-z_][A-Za-z0-9_<>,\* ]* \**[A-Za-z_][A-Za-z0-9_]*(\[[0-9]+\])?;$', ln): continue
    ln = ln.replace('basic_string<char,struct_std::char_traits<char>,class_std::allocator<char>_> *', 'str*')
    ln = ln.replace('basic_string<char,std::char_traits<char>,std::allocator<char>_> *', 'str*')
    ln = re.sub(r'\(string \*\)', '(S*)', ln)
    ln = ln.replace('*(undefined4 *)', '*(u32*)').replace('*(undefined1 *)', '*(u8*)')
    out.append(ln)
s = re.sub(r'\n{3,}', '\n\n', '\n'.join(out))
open(sys.argv[2], 'w', encoding='utf-8').write(s)
print(len(t), len(s))
