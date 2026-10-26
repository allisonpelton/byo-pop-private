#!/usr/bin/env python3
"""Tiny straight-line simulator for the llvm-objdump listing of a setup function (games/alchemy/tools/dis.sh output on stdin):
tracks immediates stored to [ebp-X] and registers, and prints the arguments of every call to the addresses given on the command line."""
import re, sys

want = set(int(a, 16) for a in sys.argv[1:])
stack = {}
regs = {}
pushes = []
for ln in sys.stdin:
    m = re.match(r'\s*([0-9a-f]+):\s+(.*)', ln)
    if not m:
        continue
    ins = m.group(2).split(' #')[0].strip()
    def val(s):
        s = s.strip()
        if re.fullmatch(r'-?0x[0-9a-f]+', s) or re.fullmatch(r'-?\d+', s):
            return int(s, 0)
        return ('reg', s) if s in regs and isinstance(regs[s], int) is False and False else regs.get(s, s)
    mm = re.match(r'mov dword ptr \[ebp - (0x[0-9a-f]+)\], (.*)', ins)
    if mm:
        stack[int(mm.group(1), 16)] = val(mm.group(2)); continue
    mm = re.match(r'and dword ptr \[ebp - (0x[0-9a-f]+)\], 0x0', ins)
    if mm:
        stack[int(mm.group(1), 16)] = 0; continue
    mm = re.match(r'mov (e[a-z]{2}), (-?0x[0-9a-f]+|\d+)$', ins)
    if mm:
        regs[mm.group(1)] = int(mm.group(2), 0); continue
    mm = re.match(r'push (-?0x[0-9a-f]+)$', ins)
    if mm:
        pushes.append(int(mm.group(1), 0)); continue
    if ins == 'pop edi' or ins == 'pop eax' or ins == 'pop ebx':
        if pushes:
            regs[ins.split()[1]] = pushes.pop()
        continue
    mm = re.match(r'lea (e[a-z]{2}), \[ebp - (0x[0-9a-f]+)\]', ins)
    if mm:
        regs[mm.group(1)] = ('addr', int(mm.group(2), 16)); continue
    mm = re.match(r'mov (e[a-z]{2}), dword ptr \[e[a-z]{2} \+ (0x[0-9a-f]+)\]', ins)
    if mm:
        regs[mm.group(1)] = ('img', int(mm.group(2), 16)); continue
    mm = re.match(r'push (e[a-z]{2})$', ins)
    if mm:
        pushes.append(regs.get(mm.group(1), mm.group(1))); continue
    mm = re.match(r'push dword ptr \[e[a-z]{2} \+ (0x[0-9a-f]+)\]', ins)
    if mm:
        pushes.append(('field', int(mm.group(1), 16))); continue
    mm = re.match(r'call (0x[0-9a-f]+)', ins)
    if mm:
        a = int(mm.group(1), 16)
        if a in want:
            args = []
            for p in reversed(pushes):
                if isinstance(p, tuple) and p[0] == 'addr':
                    args.append('rect' + str([stack.get(p[1] - 4 * k, '?') for k in range(4)]))
                elif isinstance(p, tuple) and p[0] == 'img':
                    args.append('img[%d]' % (p[1] // 4 - 0x11 + 17 if False else p[1] // 4))
                else:
                    args.append(str(p))
            print('call %x(%s)' % (a, ', '.join(args)))
        pushes = []
