#!/usr/bin/env python3
"""Print a function's decompilation with the object of each CurvedVal call filled in.

The decompiler drops `this` of the CurvedVal methods (they are __thiscall with ecx set by `lea ecx, [esi + 0x178]` or
through another register). This follows the registers in the disassembly (tools/asm.py) and writes each call as
`CurvedVal_GetOutVal<esi+0x178>(...)`, in the order the calls appear (the decompiler keeps that order almost always;
check a doubtful one in the disassembly).

    python games/bejeweledtwist/tools/cvread.py 69a13c
"""
import re
import subprocess
import sys

HERE = 'games/bejeweledtwist/tools/'
LO, HI = 0x6e9700, 0x6ed300  # CurvedVal
REG = r'e(?:ax|bx|cx|dx|si|di|bp|sp)'


def main():
    addr = sys.argv[1].lower().replace('0x', '')
    asm = subprocess.run(['python', HERE + 'asm.py', '0x' + addr], capture_output=True, text=True).stdout
    regs = {}
    targets = []
    for line in asm.splitlines():
        m = re.match(r'\s*([0-9a-f]+)\s+(\w+)\s+(.*?)(?:\s+;.*)?$', line)
        if not m:
            continue
        op, args = m.group(2), m.group(3)
        mm = re.match(r'(%s), \[(%s) \+ (0x[0-9a-f]+)\]' % (REG, REG), args)
        if op == 'lea' and mm:
            base = regs.get(mm.group(2), mm.group(2))
            regs[mm.group(1)] = '%s+%s' % (base, mm.group(3))
            continue
        mm = re.match(r'(%s), (%s)$' % (REG, REG), args)
        if op == 'mov' and mm:
            regs[mm.group(1)] = regs.get(mm.group(2), mm.group(2))
            continue
        mm = re.match(r'(%s), dword ptr \[(%s) \+ (0x[0-9a-f]+)\]' % (REG, REG), args)
        if op == 'mov' and mm:
            base = regs.get(mm.group(2), mm.group(2))
            regs[mm.group(1)] = '[%s+%s]' % (base, mm.group(3))
            continue
        if op == 'call':
            mc = re.match(r'0x([0-9a-f]+)', args)
            if mc and LO <= int(mc.group(1), 16) < HI:
                targets.append(regs.get('ecx', 'ecx'))
            for r in ('eax', 'ecx', 'edx'):
                regs.pop(r, None)
            continue
        mm = re.match(r'(%s)\b' % REG, args)
        if mm and op not in ('push', 'cmp', 'test'):
            regs.pop(mm.group(1), None)
    dec = subprocess.run(['python', HERE + 'decread.py', addr], capture_output=True, text=True).stdout
    it = iter(targets)
    out = re.sub(r'\b(CurvedVal_\w+)\(', lambda m: '%s<%s>(' % (m.group(1), next(it, '?')), dec)
    left = list(it)
    sys.stdout.write(out)
    if left:
        print('// cvread: %d more calls in the disassembly than in the decompilation' % len(left))


if __name__ == '__main__':
    main()
