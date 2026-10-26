#!/usr/bin/env python3
"""Rewrite `this`-relative accesses of Ghidra's decompilation as byte offsets, and apply field names.

    python norm.py <fields file> <addr>...
        fields file: lines "0x60 mState" (byte offset, name); '#' starts a comment. Names are used as @mState.
        <addr>:      function addresses of re/alchemy/game.c (the uncleaned region dump)

`this` is any of this, this_00, extraout_ECX, and param_1 in a function declared __fastcall (or with 'this' unnamed).
Pointer arithmetic of the alias scales by its declared element type (int * = 4), so this_00[0x18] and
*(double *)(this_00 + 0x344) become @0x60 and @0xd10. An access written as (int)this + 0xaed is already in bytes.
"""
import re, sys

FIELDS = {}
NL = chr(10)


def load_fields(path):
    for ln in open(path, encoding='utf-8'):
        ln = ln.split('#')[0].strip()
        if not ln:
            continue
        off, name = ln.split()[:2]
        FIELDS[int(off, 16)] = name


def nm(off):
    n = FIELDS.get(off)
    return '@' + n if n else '@0x%x' % off


ELEM = {'int': 4, 'uint': 4, 'undefined4': 4, 'long': 4, 'ulong': 4, 'float': 4, 'DWORD': 4, 'double': 8,
        'undefined8': 8, 'longlong': 8, 'char': 1, 'byte': 1, 'undefined1': 1, 'bool': 1, 'uchar': 1,
        'short': 2, 'ushort': 2, 'undefined2': 2, 'void': 1}


def conv(body, alias_elem):
    def num(s):
        return int(s, 16) if s.lower().startswith('0x') else int(s)

    out = body
    for a, el in alias_elem.items():
        ea = re.escape(a)
        # a[expr + k]
        out = re.sub(r'%s\[([^\]\[]+?) \+ (0x[0-9a-fA-F]+|\d+)\]' % ea, lambda m: nm(num(m.group(2)) * el) + '[' + m.group(1) + ']', out)
        # a[idx]
        out = re.sub(r'\b%s\[(0x[0-9a-fA-F]+|\d+)\]' % ea, lambda m: nm(num(m.group(1)) * el), out)
        # (int)a + k  and  (int)a + -k
        out = re.sub(r'\(\(int\)%s \+ (0x[0-9a-fA-F]+|\d+)\)' % ea, lambda m: '&' + nm(num(m.group(1))) if False else '(' + nm(num(m.group(1))) + ')', out)
        # (a + k) pointer arithmetic
        out = re.sub(r'\(%s \+ (0x[0-9a-fA-F]+|\d+)\)' % ea, lambda m: '(' + nm(num(m.group(1)) * el) + ')', out)
        # bare address: a + k (an array or a sub-object)
        out = re.sub(r'(?<![\w@&(])%s \+ (0x[0-9a-fA-F]+|\d+)\b' % ea, lambda m: '&' + nm(num(m.group(1)) * el), out)
        out = re.sub(r'\(int\)%s \+ (0x[0-9a-fA-F]+|\d+)\b' % ea, lambda m: '&' + nm(num(m.group(1))), out)
    return out



GS = re.compile(r'std::string::string_cstr\(\(S\*\)\(unaff_EBP \+ -(0x[0-9a-f]+)\),"(\w+)"\);\s*(?:pvVar2 = \(void \*\)@mApp;\s*)?pbVar3 = FUN_0046d7d0\([^;]*?\);\s*std::string::assign_substr\s*\(\(S\*\)\(unaff_EBP \+ -(0x[0-9a-f]+)\),\s*\(str\*\)\s*pbVar3,0,0xffffffff\);')


def squash(t):
    return GS.sub(lambda m: 'strv(-%s) = GetString("%s");' % (m.group(3), m.group(2)), t)


def clean(t):
    out = []
    for ln in t.split(NL):
        if re.search(r'unaff_EBP \+ -4\) = \w+;$', ln): continue
        if ln.strip() == 'FUN_004c2f74();' or 'ExceptionList' in ln: continue
        if re.search(r"std::string::_Tidy\(.*'\x01'\);", ln): continue
        if re.match(r'^  [A-Za-z_][A-Za-z0-9_<>,\* ]* \**[A-Za-z_][A-Za-z0-9_]*(\[[0-9]+\])?;$', ln): continue
        ln = ln.replace('basic_string<char,struct_std::char_traits<char>,class_std::allocator<char>_> *', 'str*')
        ln = ln.replace('basic_string<char,std::char_traits<char>,std::allocator<char>_> *', 'str*')
        ln = re.sub(r'\(string \*\)', '(S*)', ln)
        ln = ln.replace('*(undefined4 *)', '*(u32*)').replace('*(undefined1 *)', '*(u8*)')
        out.append(ln)
    return squash(re.sub(NL + '{3,}', NL + NL, NL.join(out)))


APPF = {}


def app_names(t):
    return re.sub(r'\(@mApp \+ (0x[0-9a-fA-F]+|\d+)\)', lambda m: '(@mApp->' + (('@' + APPF[int(m.group(1), 0)]) if int(m.group(1), 0) in APPF else '0x%x' % int(m.group(1), 0)) + ')', t)


def main():
    load_fields(sys.argv[1])
    import os
    if os.path.exists('re/alchemy/fields_App.txt'):
        for ln in open('re/alchemy/fields_App.txt'):
            ln = ln.split('#')[0].split()
            if len(ln) >= 2: APPF[int(ln[0], 16)] = ln[1]
    t = open('re/alchemy/game.c', encoding='utf-8').read()
    parts = re.split(r'(?m)^(?=// [0-9a-f]{8}  )', t)
    want = set(a.lower().zfill(8) for a in sys.argv[2:])
    sys.path.insert(0, 'games/alchemy/tools')
    for p in parts:
        m = re.match(r'// ([0-9a-f]{8})  ', p)
        if not (m and m.group(1) in want):
            continue
        # declared pointer element sizes
        alias_elem = {}
        for d in re.finditer(r'^  ([A-Za-z_][A-Za-z0-9_]*)( \*+)? ?\**(this_00|this|extraout_ECX|param_1)\s*;', p, re.M):
            pass
        for d in re.finditer(r'(?m)^\s*([A-Za-z_][A-Za-z0-9_<>]*) (\*+)?(this_00|extraout_ECX)\b', p):
            alias_elem[d.group(3)] = ELEM.get(d.group(1), 4) if d.group(2) else 1
        hdr = p.split('{')[0]
        mt = re.search(r'\b([A-Za-z_][A-Za-z0-9_<>]*) (\*+)?this\b', hdr)
        if mt and 'this' not in alias_elem:
            alias_elem['this'] = ELEM.get(mt.group(1), 1) if mt.group(2) else 1
        if '__fastcall' in hdr or 'void * __thiscall' in hdr:
            mp = re.search(r'\b([A-Za-z_][A-Za-z0-9_<>]*) (\*+)?param_1\b', hdr)
            if mp and 'param_1' not in alias_elem:
                alias_elem['param_1'] = ELEM.get(mp.group(1), 4) if mp.group(2) else 1
        # extraout_ECX and this_00 are declared lower in the body; default to 4-byte elements
        for a in ('this_00', 'extraout_ECX'):
            if a in p and a not in alias_elem:
                alias_elem[a] = 4
        body = conv(p, alias_elem)
        # tidy
        body = re.sub(r"unaff_EBP \+ -4\) = \w+;", 'unaff_EBP + -4) = SEH;', body)
        print(app_names(clean(body)).strip())
        print()


if __name__ == '__main__':
    main()
