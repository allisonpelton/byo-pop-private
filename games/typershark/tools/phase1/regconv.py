#!/usr/bin/env python3
r"""regconv.py ini2reg <registry.ini> <out.reg> | reg2ini <reg export> <out.ini>
Converts the port's emulated registry (registry.ini of its -userdir) to a .reg file that `reg import` takes (for the original) and the
original's `reg export` of HKCU\Software\PopCap\TyperShark to the port's registry.ini. P1b: each program starts a session from the
profile the other one wrote."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from endcmp import read_reg, read_ini

BS = chr(92)


def ini2reg(src, dst):
    vals = read_ini(src)
    keys = {}
    for (k, name), (kind, v) in vals.items():
        keys.setdefault(k, []).append((name, kind, v))
    out = ['Windows Registry Editor Version 5.00', '']
    for k in sorted(keys):
        out.append('[HKEY_CURRENT_USER' + BS + k.replace('software' + BS, 'Software' + BS, 1) + ']')
        for name, kind, v in keys[k]:
            if kind == 'dword':
                out.append('"%s"=dword:%08x' % (name, v & 0xffffffff))
            else:
                out.append('"%s"="%s"' % (name, v.replace(BS, BS + BS).replace('"', BS + '"')))
        out.append('')
    open(dst, 'w', newline='', encoding='utf-16').write('\r\n'.join(out) + '\r\n')


def reg2ini(src, dst):
    vals = read_reg(src)
    keys = {}
    for (k, name), (kind, v) in vals.items():
        keys.setdefault(k, []).append((name, kind, v))
    out = ['; Settings (emulated Windows registry, HKEY_CURRENT_USER)', '']
    for k in sorted(keys):
        head = k.replace('software' + BS + 'popcap' + BS + 'typershark', 'SOFTWARE' + BS + 'PopCap' + BS + 'TyperShark', 1)
        out.append('[' + head + ']')
        for name, kind, v in keys[k]:
            if kind == 'dword':
                out.append('%s=dword:%d' % (name, v if v < 0x80000000 else v - 0x100000000))
            else:
                out.append('%s=sz:%s' % (name, v))
        out.append('')
    open(dst, 'w', newline='').write('\n'.join(out) + '\n')


if __name__ == '__main__':
    {'ini2reg': ini2reg, 'reg2ini': reg2ini}[sys.argv[1]](sys.argv[2], sys.argv[3])
