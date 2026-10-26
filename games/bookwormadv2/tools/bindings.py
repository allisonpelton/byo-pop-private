#!/usr/bin/env python3
"""List the C++ methods BookwormAdventuresVol2.exe binds to Lua: for each method, the Lua name, the wrapper's C++ class
and signature (from the RTTI name of its LuaLib::MethodWrapper<> type) and the method it calls.

Each bound method is registered by a chain of calls in its class's constructor (or a registration function), one
call per method to a small function ("template") that makes a MethodWrapper and calls AddMethod (0x6d2400). The
caller pushes all the (member pointer, adjustment) pairs first and then calls the templates in turn, with the
method's name in edi, or inlined into the template when it is used once. This script follows that pattern with a
small simulation of the stack and the registers over each registering function, in a straight line.

    python games/bookwormadv2/tools/bindings.py               # every bound method, by class
    python games/bookwormadv2/tools/bindings.py ArrowHint     # the classes whose name contains a word
The output is for reading only: it holds no game text, but keep it out of the repository with the other exports.
"""
import json
import struct
import os
import re
import subprocess
import sys

EXE = 're/bookwormadv2/bin/BookwormAdventuresVol2.exe'
OBJDUMP = 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe'
GH = 're/bookwormadv2/gh'
ADDMETHOD = 0x6d2400
MAKESTRING = 0x401e00  # std::string::assign(const char*, size)

funcs = {}
for line in open(os.path.join(GH, 'functions.jsonl'), encoding='utf-8'):
    j = json.loads(line)
    funcs[int(j['addr'], 16)] = j
strs = {}
for line in open(os.path.join(GH, 'strings.jsonl'), encoding='utf-8'):
    j = json.loads(line)
    if j.get('type') == 'string':
        strs[int(j['addr'], 16)] = j['value']
wrappers = {}  # vtable address -> MethodWrapper class name
for line in open(os.path.join(GH, 'vtables.jsonl'), encoding='utf-8'):
    j = json.loads(line)
    if j['class'].startswith('LuaLib::') and 'Wrapper<' in j['class'] and 'meta' not in j['symbol']:
        wrappers[int(j['addr'], 16)] = j['class']

DATA = open(EXE, 'rb').read()


def _sections():
    pe = struct.unpack_from('<I', DATA, 0x3C)[0]
    nsec = struct.unpack_from('<H', DATA, pe + 6)[0]
    opt = struct.unpack_from('<H', DATA, pe + 20)[0]
    base = struct.unpack_from('<I', DATA, pe + 24 + 28)[0]
    out = []
    for i in range(nsec):
        sh = pe + 24 + opt + 40 * i
        vsize, va, rsize, raw = struct.unpack_from('<IIII', DATA, sh + 8)
        out.append((base + va, min(vsize, rsize), raw))
    return out


SECS = _sections()
CACHE = os.path.join(os.environ.get('TMPDIR', os.environ.get('TEMP', '.')), 'bwa_text.dis')
starts = sorted(funcs)
_ins = None


def load():
    """The whole code section, disassembled once into a cache file (about a minute the first time)."""
    global _ins
    if _ins is None:
        if not os.path.exists(CACHE):
            with open(CACHE, 'w') as f:
                subprocess.run([OBJDUMP, '-d', '--no-show-raw-insn', '-M', 'intel', '--start-address=0x401000',
                                '--stop-address=0x740000', EXE], stdout=f, text=True)
        _ins = {}
        for ln in open(CACHE):
            m = re.match(r'\s*([0-9a-f]+):\s+(\S+)\s*(.*)', ln)
            if m:
                ops = re.sub(r'\s*<[^>]*>', '', m.group(3)).strip()
                _ins[int(m.group(1), 16)] = (m.group(2), ops)
    return _ins


def disasm(start, stop=None):
    """[(address, mnemonic, operands)] of a function (up to the next function) or of a range."""
    ins = load()
    if stop is None:
        if start not in funcs:
            return []
        i = starts.index(start)
        stop = starts[i + 1] if i + 1 < len(starts) else start + funcs[start]['size']
    out = []
    a = start
    while a < stop:
        if a in ins:
            out.append((a, ins[a][0], ins[a][1]))
        a += 1
    return out


def cstring(va):
    """A C string of the exe's data, read from the file."""
    if va in strs:
        return strs[va]
    for start, size, raw in SECS:
        if start <= va < start + size:
            off = raw + va - start
            end = DATA.index(b'\0', off)
            if 0 < end - off < 80:
                return DATA[off:end].decode('latin-1')
    return None


def imm(s):
    m = re.fullmatch(r'(-?0x[0-9a-f]+|-?\d+)', s)
    return int(m.group(1), 0) & 0xffffffff if m else None


_pops = {}


def popcount(fn):
    """How many dwords a function removes from the stack on return (ret N), or 0."""
    if fn not in _pops:
        n = 0
        for _, mn, ops in disasm(fn):
            if mn == 'ret' and ops:
                n = imm(ops) // 4
                break
        _pops[fn] = n
    return _pops[fn]


def wrapper_vtable(fn, depth=0):
    """The MethodWrapper vtable a template installs (in itself or in the constructor it calls)."""
    for _, mn, ops in disasm(fn):
        if mn == 'mov' and ops.startswith('dword ptr ['):
            v = imm(ops.split(',')[-1].strip())
            if v in wrappers:
                return v
    if depth < 2:
        for _, mn, ops in disasm(fn):
            if mn == 'call':
                t = imm(ops)
                if t is not None and t != ADDMETHOD and t in funcs and funcs[t]['size'] < 400:
                    v = wrapper_vtable(t, depth + 1)
                    if v:
                        return v
    return None


def inline_name(fn):
    """The method name a template builds itself (push <string>; call MAKESTRING), or None (it takes it in a
    register: edi, or ecx in the raw method templates, which copy it to edi first)."""
    prev = None
    for _, mn, ops in disasm(fn):
        if mn == 'call' and imm(ops) == MAKESTRING:
            return cstring(prev) if prev is not None else None
        if mn == 'push':
            prev = imm(ops)
    return None


def name_register(fn):
    for _, mn, ops in disasm(fn):
        if mn == 'call':
            break
        if mn == 'mov' and ops in ('edi, ecx', 'esi, ecx'):
            return 'ecx'
    return 'edi'


def inline_pmf(fn):
    """The member pointer a template stores into the wrapper it makes (mov [esi+4], <method>), when the
    instantiation has it built in instead of taking it from its caller."""
    for _, mn, ops in disasm(fn):
        m = re.fullmatch(r'dword ptr \[e[sd]i \+ 0x4\], (0x[0-9a-f]+)', ops or '') if mn == 'mov' else None
        if m:
            return int(m.group(1), 16)
    return None


templates = {a: {'vt': wrapper_vtable(a), 'name': inline_name(a), 'reg': name_register(a), 'pops': popcount(a),
                 'pmf': inline_pmf(a)}
             for a in (int(c, 16) for c in funcs[ADDMETHOD]['callers'])}


def vcall_slot(fn):
    """A member pointer to a virtual function points at a thunk: mov eax,[ecx]; jmp [eax+N]."""
    ins = disasm(fn, fn + 12)
    if len(ins) >= 2 and ins[0][1] == 'mov' and ins[0][2] == 'eax, dword ptr [ecx]' and ins[1][1] == 'jmp':
        m = re.fullmatch(r'dword ptr \[eax( \+ (0x[0-9a-f]+))?\]', ins[1][2])
        if m:
            return int(m.group(2) or '0', 16) // 4
    return None


_lua_class = {}


CREATECLASS = 0x6d1720
starters = {int(c, 16) for c in funcs[CREATECLASS]['callers']}


def lua_class(fn):
    """The Lua class a function that starts a class's chain creates (it calls CreateClass with that name: the
    first class-like string it refers to), or None."""
    if fn not in _lua_class:
        _lua_class[fn] = None
        if fn in starters:
            for _, mn, ops in disasm(fn):
                for h in re.findall(r'0x7[0-9a-f]{5}', ops):
                    c = cstring(int(h, 16))
                    if c and re.fullmatch(r'[A-Za-z][\w.<>]*', c) and c not in ('CClasses.', '__index'):
                        _lua_class[fn] = c
                        return c
        elif fn in funcs and fn not in templates:
            # a function that calls a starter first (and then, say, SetBaseClass)
            for _, mn, ops in disasm(fn):
                t = imm(ops) if mn == 'call' else None
                if t in starters:
                    _lua_class[fn] = lua_class(t)
                    break
    return _lua_class[fn]


def simulate(site):
    """Yield (template, name, member pointer, adjustment, Lua class) for each template call in a registering
    function. Templates of raw functions (with no member pointer) take the function on the stack (cdecl) and
    belong to the class the last class-starting call named."""
    regs = {}
    stack = []
    current = None
    for addr, mn, ops in disasm(site):
        parts = [p.strip() for p in ops.split(',')] if ops else []
        if mn == 'push':
            v = imm(parts[0])
            stack.append(v if v is not None else regs.get(parts[0]))
        elif mn == 'pop':
            if stack:
                stack.pop()
            regs.pop(parts[0], None)
        elif mn == 'mov' and len(parts) == 2 and re.fullmatch(r'e[a-d]x|e[sd]i|ebp', parts[0]):
            v = imm(parts[1])
            regs[parts[0]] = v if v is not None else regs.get(parts[1])
        elif mn == 'xor' and len(parts) == 2 and parts[0] == parts[1]:
            regs[parts[0]] = 0
        elif mn in ('lea', 'and', 'or', 'sub', 'add', 'movzx', 'movsx', 'xor') and parts and parts[0] in regs:
            if mn == 'add' and parts[0] == 'esp':
                pass
            regs.pop(parts[0], None)
        if mn == 'add' and parts and parts[0] == 'esp' and imm(parts[1]) is not None:
            del stack[len(stack) - imm(parts[1]) // 4:]
        elif mn == 'sub' and parts and parts[0] == 'esp' and imm(parts[1]) is not None:
            stack.extend([None] * (imm(parts[1]) // 4))
        elif mn == 'call':
            t = imm(parts[0]) if parts else None
            if t in templates:
                tp = templates[t]
                if tp['pops']:
                    args = [stack.pop() if stack else None for _ in range(tp['pops'])]
                else:
                    args = [None, stack[-1] if stack else None]
                name = tp['name'] or (cstring(regs[tp['reg']]) if regs.get(tp['reg']) else None)
                pmf = args[1] if len(args) > 1 else None
                if len(args) == 1:
                    pmf = args[0]  # raw templates pop the function; the caller drops the 0 pushed under it
                adj = args[2] if len(args) > 2 else None
                if pmf is None:
                    pmf = tp['pmf']
                yield t, name, pmf, adj, current
            elif t is not None:
                current = lua_class(t) or current
                for _ in range(popcount(t)):
                    if stack:
                        stack.pop()
            for r in ('eax', 'ecx', 'edx'):
                regs.pop(r, None)


def describe(pmf):
    if pmf is None:
        return '?'
    s = vcall_slot(pmf)
    if s is not None:
        return 'virtual slot %d' % s
    f = funcs.get(pmf)
    return '%06x %s' % (pmf, f['full'] if f else '')


def short(wrapper):
    w = wrapper.replace('std::basic_string<char,std::char_traits<char>,std::allocator<char>_>', 'string')
    w = w.replace('__thiscall_', '').replace('_', ' ')
    return w


sites = sorted({int(c, 16) for t in templates for c in funcs[t]['callers']})
rows = []
for site in sites:
    for t, name, pmf, adj, current in simulate(site):
        vt = templates[t]['vt']
        w = wrappers.get(vt) or ('LuaLib::RawFunction<%s>' % current if current else '?')
        rows.append((w, name or '?', pmf, adj, site, t, current))

# Grouped by the Lua class each chain registers into (CClasses.<name>); the C++ class of a method is the
# wrapper's, shown when it is another one (a base class's method)
DEBUG = '-d' in sys.argv
words = [w for w in sys.argv[1:] if w != '-d']
byclass = {}
for w, name, pmf, adj, site, t, current in rows:
    m = re.match(r'LuaLib::\w*<([^,]+)(,(.*))?>$', w)
    cls, sig = (m.group(1), m.group(3) or 'raw (lua_State*)') if m else ('?', w)
    lua = current or cls
    if words and not any(x in lua or x in cls for x in words):
        continue
    sig = short(sig)
    if cls != lua and not cls.endswith('::' + lua) and not cls.startswith('sexy.'):
        sig = sig.replace(cls + '::*', '*') + '  [' + cls + ']'
    byclass.setdefault(lua, []).append((name, sig, pmf, adj, site, t))
for cls in sorted(byclass):
    print('%s (%d)' % (cls, len(byclass[cls])))
    for name, sig, pmf, adj, site, t in byclass[cls]:
        print('    %-28s %-60s %s%s%s' % (name, sig, describe(pmf), ' adj %d' % adj if adj else '',
                                         ' [%06x in %06x]' % (t, site) if DEBUG else ''))
print('%d methods in %d classes, %d registering functions' % (len(rows), len(byclass), len(sites)))
