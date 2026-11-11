#!/usr/bin/env python3
"""python ctorstores.py <function addr hex> [base register]: the stores of a constructor, compactly.

Prints every 'MOV [reg + off], x' as '+off = x' in program order, folding runs of zero stores into
'+a..+b = 0 (n)', and every CALL as 'call <addr> <comment>'. The base register defaults to the one the function
stores most often at offset 0x58 or 0x0 (usually ESI/EBX/EDI after 'MOV reg, ECX'). Values: immediates as they are,
registers as the register name (so a store of a freshly allocated list head shows as 'EAX')."""
import os, re, sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
addr = sys.argv[1].lower().lstrip('0x').zfill(8)
lines = open(os.path.join(ROOT, 're', 'astropop', 'asm', addr + '.asm'), encoding='utf-8', errors='replace').read().splitlines()

store = re.compile(r'MOV (?:byte |word |dword )?ptr \[([A-Z]+) \+ (0x[0-9a-f]+)\],([^ ;]+)')
store0 = re.compile(r'MOV (?:byte |word |dword )?ptr \[([A-Z]+)\],([^ ;]+)')
call = re.compile(r'CALL (0x[0-9a-f]+|[A-Z]+|dword ptr .*?)(?:\s*;\s*(.*))?$')
fp = re.compile(r'(?:FST|FSTP) (?:float|double|qword|dword) ptr \[([A-Z]+) \+ (0x[0-9a-f]+)\]')

base = sys.argv[2].upper() if len(sys.argv) > 2 else None
if base is None:
    cnt = {}
    for l in lines:
        m = store.search(l)
        if m and m.group(2) in ('0x58', '0x5c', '0x60', '0x64', '0x68'):
            cnt[m.group(1)] = cnt.get(m.group(1), 0) + 1
    base = max(cnt, key=cnt.get) if cnt else 'ESI'

out = []
zrun = None


def flush():
    global zrun
    if zrun is not None:
        a, b, n = zrun
        out.append('+%x = 0%s' % (a, '' if n == 1 else '  ..+%x (%d stores)' % (b, n)))
        zrun = None


for l in lines:
    parts = l.split(' ', 1)
    if len(parts) < 2:
        continue
    ins = parts[1]
    m = store.search(ins)
    if m and m.group(1) == base:
        off = int(m.group(2), 16)
        val = m.group(3)
        if val in ('0x0',):
            if zrun is None:
                zrun = [off, off, 1]
            else:
                zrun[1] = off
                zrun[2] += 1
            continue
        flush()
        out.append('+%x = %s' % (off, val))
        continue
    m = fp.search(ins)
    if m and m.group(1) == base:
        flush()
        out.append('+%x = (x87 store)' % int(m.group(2), 16))
        continue
    m = call.search(ins)
    if m:
        flush()
        out.append('call %s %s' % (m.group(1), (m.group(2) or '')))
flush()
print('base register', base)
print('\n'.join(out))
