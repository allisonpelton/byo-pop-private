#!/usr/bin/env python3
"""Evaluates the original's C++ static initialisers (the table at 0x59c000-0x59c1b8 that _cinit walks) with a tiny x86
interpreter, for the tiny ones that build Colors and copy dwords between globals. Prints the resulting globals that were
written (address: value) and the initialisers it could not follow. Usage: initvals.py [exe]"""
import struct, sys, collections
exe = sys.argv[1] if len(sys.argv) > 1 else 're/astropop/bin/AstroPopGame.exe'
data = open(exe, 'rb').read()
pe = struct.unpack_from('<I', data, 0x3c)[0]
nsec = struct.unpack_from('<H', data, pe + 6)[0]; opt = struct.unpack_from('<H', data, pe + 20)[0]
base = struct.unpack_from('<I', data, pe + 52)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + opt + i * 40
    vs, va, rs, ro = struct.unpack_from('<IIII', data, o + 8)
    secs.append((base + va, vs, ro, rs))
def rd8(v):
    for va, vs, ro, rs in secs:
        if va <= v < va + vs: return data[ro + v - va] if v - va < rs else 0
    raise KeyError(hex(v))
def rd32(v): return rd8(v) | rd8(v+1) << 8 | rd8(v+2) << 16 | rd8(v+3) << 24
def s8(b): return b - 256 if b > 127 else b
def s32(x): return x - (1 << 32) if x >= 1 << 31 else x
mem = {}   # written dwords
def load(a): return mem.get(a, rd32(a))
CTOR_COLOR = None
calls = collections.Counter()
unknown = []
ptrs = [rd32(0x59c000 + i) for i in range(0, 0x1b8, 4)]
log = []
for p in [x for x in ptrs if x]:
    pc = p; reg = {}; stack = []; ok = True; steps = 0; written = []
    while steps < 400:
        steps += 1
        b = rd8(pc)
        if b == 0x68: stack.append(rd32(pc + 1)); pc += 5
        elif b == 0x6a: stack.append(s8(rd8(pc + 1)) & 0xffffffff); pc += 2
        elif 0xb8 <= b <= 0xbf: reg[b - 0xb8] = rd32(pc + 1); pc += 5
        elif b == 0xa1: reg[0] = load(rd32(pc + 1)); pc += 5
        elif b == 0xa3: a = rd32(pc + 1); mem[a] = reg[0]; written.append((a, reg[0])); pc += 5
        elif b == 0x8b and rd8(pc + 1) & 0xc7 == 0x05: r = (rd8(pc + 1) >> 3) & 7; reg[r] = load(rd32(pc + 2)); pc += 6
        elif b == 0x89 and rd8(pc + 1) & 0xc7 == 0x05: r = (rd8(pc + 1) >> 3) & 7; a = rd32(pc + 2); mem[a] = reg[r]; written.append((a, reg[r])); pc += 6
        elif b == 0xc3: break
        elif b == 0xe8:
            t = (pc + 5 + s32(rd32(pc + 1))) & 0xffffffff
            if t == 0x402d80 and 1 in reg and len(stack) >= 4:      # Color::Color(r, g, b, a)
                vals = [stack[-1], stack[-2], stack[-3], stack[-4]]; del stack[-4:]
            elif t == 0x4a8be0 and 1 in reg and len(stack) >= 3:    # Color::Color(r, g, b), alpha 255
                vals = [stack[-1], stack[-2], stack[-3], 255]; del stack[-3:]
            else:
                ok = False; break
            calls[t] += 1
            for i, v in enumerate(vals): mem[reg[1] + 4 * i] = v; written.append((reg[1] + 4 * i, v))
            pc += 5; continue
        else: ok = False; break
    if not ok: unknown.append((hex(p), hex(pc), hex(rd8(pc))))
print('color ctor', hex(CTOR_COLOR or 0), dict((hex(k), v) for k, v in calls.items()))
for a in sorted(mem): print('%08x: %d' % (a, s32(mem[a])) if mem[a] < 0x1000 or mem[a] >= 0xfffff000 else '%08x: 0x%x' % (a, mem[a]))
print('unfollowed:', len(unknown)); print(unknown[:140])
