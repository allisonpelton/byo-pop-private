#!/usr/bin/env python3
"""Disassemble code of re/bookwormadv/bin/BookwormAdventures.exe (llvm-objdump, Intel syntax), with call targets
named from games/bookwormadv/docs/symbols/*.jsonl and string addresses inlined (for reading only).

    python games/bookwormadv/tools/asm.py 717540            # a whole function (size from Ghidra's functions.jsonl)
    python games/bookwormadv/tools/asm.py 7175cd 40         # 40 instructions from an address
    python games/bookwormadv/tools/asm.py -t 7175c6 49      # the 49 entries of a jump table at 7175c6 ...
"""
import glob
import json
import os
import re
import struct
import subprocess
import sys

EXE = 're/bookwormadv/bin/BookwormAdventures.exe'
OBJDUMP = 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe'
GH = 're/bookwormadv/gh'

names = {}
for p in sorted(glob.glob('games/bookwormadv/docs/symbols/*.jsonl')):
    for line in open(p, encoding='utf-8'):
        line = line.strip()
        if line and not line.startswith('//'):
            j = json.loads(line)
            if 'name' in j:
                names[int(j['addr'], 16)] = j['name']
funcs = {}
for line in open(os.path.join(GH, 'functions.jsonl'), encoding='utf-8'):
    j = json.loads(line)
    funcs[int(j['addr'], 16)] = j
strs = {}
for line in open(os.path.join(GH, 'strings.jsonl'), encoding='utf-8'):
    j = json.loads(line)
    if j.get('type') == 'string':
        strs[int(j['addr'], 16)] = j['value']


def sections():
    d = open(EXE, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3C)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
    out = []
    for i in range(nsec):
        s = pe + 24 + opt + 40 * i
        vsize, va, rsize, raw = struct.unpack_from('<IIII', d, s + 8)
        out.append((base + va, max(vsize, rsize), raw))
    return d, out


def read32(d, secs, va):
    for start, size, raw in secs:
        if start <= va < start + size:
            return struct.unpack_from('<I', d, raw + va - start)[0]
    return None


def label(v):
    if v in names:
        return names[v]
    if v in funcs:
        return funcs[v]['name']
    if v in strs:
        return '"%s"' % strs[v][:60].replace('\n', '\\n')
    return None


def disasm(start, stop):
    out = subprocess.run([OBJDUMP, '-d', '--no-show-raw-insn', '-M', 'intel', '--start-address=%#x' % start,
                          '--stop-address=%#x' % stop, EXE], capture_output=True, text=True).stdout
    for ln in out.splitlines():
        m = re.match(r'\s*([0-9a-f]+):\s+(.*)', ln)
        if not m:
            continue
        text = m.group(2)
        notes = []
        for h in re.findall(r'0x([0-9a-f]{6,8})', text):
            lab = label(int(h, 16))
            if lab:
                notes.append(lab)
        print('%s  %s%s' % (m.group(1), text, ('   ; ' + ', '.join(notes)) if notes else ''))


args = sys.argv[1:]
if args and args[0] == '-t':
    d, secs = sections()
    va = int(args[1], 16)
    for i in range(int(args[2])):
        t = read32(d, secs, va + 4 * i)
        print('%3d %#x %s' % (i, t, label(t) or ''))
else:
    a = int(args[0], 16)
    if len(args) > 1:
        disasm(a, a + 8 * int(args[1]))  # roughly that many instructions
    elif a in funcs:
        disasm(a, a + funcs[a]['size'])
    else:  # code Ghidra made no function of: up to the next function
        disasm(a, min(f for f in funcs if f > a))
