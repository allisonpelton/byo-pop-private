"""Collects the port's prototypes of TipTop.exe's functions for Ghidra (ghidra/TipTopSignatures.java), so its
decompilation of the game code shows the right arguments at every call (it lost some, e.g. two of PlaySound's
three) and the port's names.

    python games/tiptop/tools/ghsigs.py > re/tiptop/ghscripts/sigs.txt

A prototype counts when its declaration ends with a comment naming the address: `char InitSound();  // 0x432e50`.
Output lines: address|name|return type|parameter types separated by commas (... for varargs).
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.normpath(os.path.join(HERE, '..'))
HEADERS = [('lowos_', 'engine/Lowos.h'), ('res_', 'engine/Res.h'), ('', 'src/Gui.h'), ('', 'src/Game.h')]
DECL = re.compile(r'([\w:<>\s\*]+?)\b(\w+)\s*\((.*)\)\s*;\s*//\s*0x([0-9a-f]{6})\b', re.S)


def kind(t):
    """Ghidra's type for a C++ one: ptr, byte, short, double, void or int (32 bits)"""
    t = re.sub(r'\b(const|volatile|struct)\b', '', t).strip()
    if '*' in t or t.endswith('&'):
        return 'ptr'
    t = t.split()[0] if t.split() else t
    if t in ('char', 'uint8_t', 'int8_t', 'bool'):
        return 'byte'
    if t in ('uint16_t', 'int16_t', 'short'):
        return 'short'
    if t in ('double', 'int64_t', 'uint64_t'):
        return 'double' if t == 'double' else 'long'
    if t == 'void':
        return 'void'
    return 'int'


def params(text):
    text = re.sub(r'=\s*[^,]+', '', text).strip()
    if text in ('', 'void'):
        return []
    out = []
    for p in text.split(','):
        p = p.strip()
        if p == '...':
            out.append('...')
            continue
        m = re.match(r'(.*?)(\w+)\s*$', p)
        out.append(kind(m.group(1) if m and m.group(1).strip() else p))
    return out


def main():
    for prefix, rel in HEADERS:
        path = os.path.join(GAME, rel)
        if not os.path.exists(path):
            continue
        stmt = ''
        above = ''  # an address that a comment line just above the declaration starts with ("// 0x439b30: ...")
        for line in open(path, encoding='utf-8'):
            code, _, comment = line.partition('//')
            if not stmt and not code.strip():
                a = re.match(r'\s*(0x[0-9a-f]{6})\b', comment) if comment else None
                above = a.group(1) if a else (above if comment else '')
                continue
            stmt += code
            if ';' not in code:
                continue
            if not re.search(r'0x[0-9a-f]{6}', comment) and above:
                comment = ' ' + above
            above = ''
            m = DECL.search(stmt.rstrip() + ' //' + comment) if comment else None
            if m and '(' in stmt and 'typedef' not in stmt:
                ret = kind(m.group(1).split('\n')[-1])
                sys.stdout.write('%s|%s%s|%s|%s\n' % (m.group(4), prefix, m.group(2), ret, ','.join(params(m.group(3)))))
            stmt = ''


if __name__ == '__main__':
    main()
