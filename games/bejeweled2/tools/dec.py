#!/usr/bin/env python3
"""Print the export's decompilation of functions by address.

    python games/bejeweled2/tools/dec.py 5dd6cb 5dd7a0 ...   # one or more addresses (hex)
    python games/bejeweled2/tools/dec.py -r 5dbb36 5dd000     # every function in the range, in order
    python games/bejeweled2/tools/dec.py -l 5dbb36 5dd000     # only a list: address, size, callers
    python games/bejeweled2/tools/dec.py -a 5dbf24 [5dbb94 ...]  # the disassembly (re/bejeweled2/gt/asm.txt, tools/ghidra/DumpAsm.java)
Reads re/bejeweled2/gn (the export with the Mac-derived names; gh, the plain one, when there is no gn). Substitutes the
resource globals' names (re/bejeweled2/res_map.txt) and the names this session gave functions and globals
(re/bejeweled2/names.txt: '<hex address> <name>' per line) for their placeholders.
"""
import json
import os
import re
import sys

RE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 're', 'bejeweled2')
GH = os.path.join(RE, 'gn') if os.path.isdir(os.path.join(RE, 'gn', 'decomp')) else os.path.join(RE, 'gh')
GT = os.path.join(RE, 'gt')   # games/bejeweled2/tools/ghidra/ExportRange.java: the game's range with __thiscall set (this arguments shown)
_funcs = None


def funcs():
    global _funcs
    if _funcs is None:
        _funcs = {}
        for line in open(os.path.join(GH, 'functions.jsonl'), encoding='utf-8'):
            j = json.loads(line)
            _funcs[int(j['addr'], 16)] = j
    return _funcs


_names = None


def rename(text):
    global _names
    if _names is None:
        _names = {}
        for fn in ('res_map.txt', 'names.txt'):
            path = os.path.join(RE, fn)
            if os.path.exists(path):
                for line in open(path, encoding='utf-8'):
                    parts = line.split()
                    if len(parts) >= 2:
                        _names[int(parts[0], 16)] = parts[1]
    return re.sub(r'\b(thunk_FUN_|FUN_|DAT_|PTR_DAT_|_DAT_)([0-9a-f]{8})\b',
                  lambda m: _names.get(int(m.group(2), 16), m.group(0)), text)


_noise = re.compile(r'^\s*(\*\((?:undefined\d?|int|byte) \*\)\(unaff_EBP \+ -4\) = [0-9a-fx]+;|FUN_005e7e34\(\);|ExceptionList = .*;|'
                    r'(?:Sexy::ResourceManager::HadError|FUN_00429550)\(1,0\);)\s*$')


def clean(text):
    # drops the exception-frame bookkeeping the decompiler leaves in functions with destructors
    return '\n'.join(l for l in text.split('\n') if not _noise.match(l))


_exe = None


def exe_string(va):
    """The C string at virtual address va in the exe ('' if it is not in a section)."""
    global _exe
    import struct
    if _exe is None:
        _exe = open(os.path.join(RE, 'bin', 'Bejeweled2Game.exe'), 'rb').read()
    d = _exe
    pe = struct.unpack_from('<I', d, 0x3C)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
    s = pe + 24 + opt
    for i in range(nsec):
        vs, rva, rs, ro = struct.unpack_from('<IIII', d, s + i * 40 + 8)
        if rva <= va - base < rva + min(vs, rs):
            o = ro + va - base - rva
            e = d.index(b'\x00', o)
            return d[o:e].decode('latin-1')
    return None


def asm_strings(addr):
    """The literals pushed before each call of str_from_cstr (FUN_00422f60) in the function, in address order."""
    lits = []
    last = None
    for line in asm(addr).split(chr(10)):
        m = re.search(r'PUSH 0x([0-9a-f]{6,8})', line)
        if m:
            last = int(m.group(1), 16)
        if '; str_from_cstr' in line or '; FUN_00422f60' in line:
            t = exe_string(last) if last else None
            lits.append(t)
            last = None
    return lits


def fill_strings(addr, text):
    """str_from_cstr() calls whose string the decompiler lost get the literal the asm pushes (in call order)."""
    lits = asm_strings(addr)
    total = len(re.findall(r'str_from_cstr\(', text))
    if total != len(lits):
        return text   # the order cannot be trusted
    it = iter(lits)

    def sub(m):
        lit = next(it)
        if m.group(1) == ')' and lit is not None:
            return 'str_from_cstr(/*asm*/ "%s")' % lit.replace('"', '\\"')
        return m.group(0)
    return re.sub(r'str_from_cstr\((\)?)', sub, text)


def tidy(text):
    # (int)(x) for the x87 truncation helper, a Color for the 4-argument constructor
    text = re.sub(r'TRect<int>::ftol\(SUB108\(([^;]*?),0\)\)', lambda m: '(int)(' + m.group(1).replace('(float10)', '') + ')', text)
    text = text.replace('CRect::Color_rgba(', 'Color(').replace('CRect::Color_rgb(', 'Color3(')
    return text


def body(addr):
    f = funcs().get(addr)
    if f is None:
        return '/* no function at %06x */\n' % addr
    ns = '_global' if f['ns'] == 'Global' else f['ns'].replace('::', '__')
    path = os.path.join(GT, 'decomp', ns, '%s_%s.c' % (f['addr'], f['name']))
    if not os.path.exists(path):
        path = os.path.join(GH, 'decomp', ns, '%s_%s.c' % (f['addr'], f['name']))
    if not os.path.exists(path):
        return '/* no decompilation at %s */\n' % path
    text = clean(rename(open(path, encoding='utf-8', errors='replace').read()))
    return tidy(fill_strings(addr, text))


def asm(addr):
    key = '== %08x ' % addr
    out = []
    on = False
    for line in open(os.path.join(GT, 'asm.txt'), encoding='utf-8', errors='replace'):
        if line.startswith('== '):
            if on:
                break
            on = line.startswith(key)
        if on:
            out.append(rename(line.rstrip(chr(10))))
    return chr(10).join(out) if out else '/* no disassembly at %06x */' % addr


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    if a[0] == '-a':
        for x in a[1:]:
            print(asm(int(x, 16)))
        return
    if a[0] in ('-r', '-l'):
        lo, hi = int(a[1], 16), int(a[2], 16)
        for addr in sorted(funcs()):
            if lo <= addr < hi:
                f = funcs()[addr]
                # -r <lo> <hi> [min]: unnamed functions smaller than min bytes (default 0) are left out
                if a[0] == '-r' and len(a) > 3 and f['name'].startswith('FUN_') and f['size'] < int(a[3]):
                    continue
                if a[0] == '-l':
                    print('%06x %5d %-28s callers=%d %s' % (addr, f['size'], f['full'][:28], len(f['callers']), ','.join(f['callers'][:3])))
                else:
                    print('// ---- %06x %s (%d bytes)' % (addr, f['full'], f['size']))
                    print(body(addr))
    else:
        for x in a:
            print(body(int(x, 16)))


main()
