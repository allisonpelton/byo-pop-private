#!/usr/bin/env python3
"""port: reading Bejeweled3.exe's Ghidra export (re/bejeweled3/gh), with the names found so far.

    python games/bejeweled3/tools/re3.py dec <addr>...        the decompiled functions, compacted and named
    python games/bejeweled3/tools/re3.py raw <addr>...        the same, uncompacted
    python games/bejeweled3/tools/re3.py calls <addr>...      callers and callees, named
    python games/bejeweled3/tools/re3.py asm <addr> [n]       the disassembly (a function, or about n instructions)
    python games/bejeweled3/tools/re3.py table <addr> <n>     n dwords from an address, named
    python games/bejeweled3/tools/re3.py classes [substr]     each RTTI class placed by its own vtable slots
    python games/bejeweled3/tools/re3.py vt <class>           a class's vtable slots, named
    python games/bejeweled3/tools/re3.py str <regex>          strings matching, with the functions that use them
    python games/bejeweled3/tools/re3.py name <regex>         names matching

Names come from re/bejeweled3/names/*.jsonl (the session's own, `{"addr", "name"}` a line, `//` comments), then the
Mac build's pairs (re/bejeweled3/macmatch), then Zuma's Revenge's Mac build's pairs (re/bejeweled3/macmatch_zr, the
same framework with its symbols) and the export's own. Everything read here is the user's game: output is
for reading only and goes into no file of the repository.
"""
import glob
import json
import os
import re
import statistics
import struct
import subprocess
import sys
import collections

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
GH = os.path.join(ROOT, 're', 'bejeweled3', 'gh2')
EXE = os.path.join(ROOT, 're', 'bejeweled3', 'bin', 'Bejeweled3.exe')
OBJDUMP = 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe'
BS = chr(92)


def load_jsonl(path):
    out = []
    if not os.path.isfile(path):
        return out
    for line in open(path, encoding='utf-8'):
        line = line.strip()
        if line and not line.startswith('//'):
            out.append(json.loads(line))
    return out


funcs = {}
for j in load_jsonl(os.path.join(GH, 'functions.jsonl')):
    funcs[j['addr']] = j
strs = {}
for j in load_jsonl(os.path.join(GH, 'strings.jsonl')):
    if j.get('type', 'string') in ('string', 'unicode', 'wstring') or 'value' in j:
        strs[j['addr'].lower().zfill(8)] = j.get('value', '')
names = {}
# Zuma's Revenge's Mac build (symbols left in) shares the framework: its pairs first, Bejeweled 3's own over them
for j in load_jsonl(os.path.join(ROOT, 're', 'bejeweled3', 'macmatch_zr', 'zr.match.jsonl')):
    names[j['a'].lower().zfill(8)] = j['name']
for j in load_jsonl(os.path.join(ROOT, 're', 'bejeweled3', 'macmatch', 'bejeweled3.match.jsonl')):
    names[j['a'].lower().zfill(8)] = j['name']
for p in sorted(glob.glob(os.path.join(ROOT, 're', 'bejeweled3', 'names', '*.jsonl'))):
    for j in load_jsonl(p):
        names[j['addr'].lower().zfill(8)] = j.get('name', j.get('label'))


def norm(a):
    a = a.lower()
    if a.startswith('0x'):
        a = a[2:]
    return a.zfill(8)


def fname(a):
    if a in names:
        return names[a]
    f = funcs.get(a)
    return f['full'] if f else 'FUN_' + a


def esc(s):
    return '"' + s.replace(BS, BS + BS).replace('"', BS + '"').replace('\n', BS + 'n').replace('\r', BS + 'r') + '"'


decl = re.compile(r'^\s*(?!(?:goto|return|break|continue)\b)(undefined\d?|int|uint|char|byte|bool|float|double|long|'
                  r'short|ushort|ulong|longlong|ulonglong|void|code|float10|wchar_t|[A-Za-z_]\w* \*+|[A-Za-z_][\w:<>]*)'
                  r'\s*\*?\s*\**[A-Za-z_]\w*(\[\d+\])?;\s*$')


def named(t):
    t = re.sub(r'\b(?:thunk_)?FUN_([0-9a-f]{8})\b', lambda m: names.get(m.group(1), m.group(0)).replace('::', '_'), t)
    t = re.sub(r's_\w+?_([0-9a-f]{8})\b', lambda m: esc(strs[m.group(1)]) if m.group(1) in strs else m.group(0), t)
    t = re.sub(r'&?(?:DAT|PTR_DAT|u)_([0-9a-f]{8})\b',
               lambda m: ('(str)' + esc(strs[m.group(1)])) if m.group(1) in strs
               else names.get(m.group(1), m.group(0)).replace('::', '_'), t)
    return t


def compact(t):
    t = '\n'.join(ln for ln in t.split('\n') if not decl.match(ln))
    t = re.sub(r'\n\s*__security_check_cookie\([^;]*\);', '', t)
    eh = set(re.findall(r'\b(\w+)\._0_1_ = ', t)) | set(re.findall(r'\b(\w+) = CONCAT31\(\1\._1_3_', t))
    for v in eh:
        t = re.sub(r'\n\s*' + v + r'(\._0_1_)? = (0x)?[0-9a-f]+;', '', t)
        t = re.sub(r'\n\s*' + v + r' = CONCAT31\(' + v + r'\._1_3_,[^;]*\);', '', t)
    for v in set(re.findall(r'\n\s*(local_\w+) = (?:0x)?[0-9a-f]+;', t)):
        if len(re.findall(r'\b' + v + r'\b', t)) == len(re.findall(r'\n\s*' + v + r' = (?:0x)?[0-9a-f]+;', t)):
            t = re.sub(r'\n\s*' + v + r' = (?:0x)?[0-9a-f]+;', '', t)
    out = []
    for ln in t.split('\n'):
        if not ln.strip():
            continue
        if re.search(r'(pvStack_\w+|puStack_\w+|ExceptionList|local_4 = |uStack_4 = |in_FS_OFFSET)', ln):
            continue
        out.append(ln)
    return '\n'.join(out)


def show(a, raw):
    fs = glob.glob(os.path.join(GH, 'decomp', '*', a + '_*.c'))
    if not fs:
        print('// no decomp for', a)
        return
    t = open(fs[0], encoding='latin1').read()
    if not raw:
        t = compact(t)
    f = funcs.get(a, {})
    print('// %s  %s  size %s' % (a, fname(a), f.get('size')))
    print(named(t))
    print()


def sections():
    d = open(EXE, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3C)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
    out = []
    for i in range(nsec):
        s = pe + 24 + opt + 40 * i
        vsize, va, rsize, rawp = struct.unpack_from('<IIII', d, s + 8)
        out.append((base + va, max(vsize, rsize), rawp))
    return d, out


def read32(d, secs, va):
    for start, size, rawp in secs:
        if start <= va < start + size:
            return struct.unpack_from('<I', d, rawp + va - start)[0]
    return None


def label(v):
    a = '%08x' % v
    if a in names:
        return names[a]
    if a in funcs:
        return funcs[a]['name']
    if a in strs:
        return esc(strs[a][:60])
    return None


def disasm(start, stop):
    out = subprocess.run([OBJDUMP, '-d', '--no-show-raw-insn', '-M', 'intel', '--start-address=%#x' % start,
                          '--stop-address=%#x' % stop, EXE], capture_output=True, text=True).stdout
    for ln in out.splitlines():
        m = re.match(r'\s*([0-9a-f]+):\s+(.*)', ln)
        if not m:
            continue
        text = m.group(2)
        notes = [lab for h in re.findall(r'0x([0-9a-f]{6,8})', text) for lab in [label(int(h, 16))] if lab]
        print('%s  %s%s' % (m.group(1), text, ('   ; ' + ', '.join(notes)) if notes else ''))


def vtables():
    return load_jsonl(os.path.join(GH, 'vtables.jsonl'))


def main():
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return
    cmd, rest = args[0], args[1:]
    if cmd in ('dec', 'raw'):
        for a in rest:
            show(norm(a), cmd == 'raw')
    elif cmd == 'calls':
        for a in map(norm, rest):
            f = funcs[a]
            print('%s %s size %d' % (a, fname(a), f['size']))
            print('  callers:', ' '.join('%s:%s' % (c, fname(c)) for c in f['callers']))
            print('  callees:', ' '.join('%s:%s' % (c, fname(c)) for c in f['callees']))
    elif cmd == 'asm':
        a = int(rest[0], 16)
        if len(rest) > 1:
            disasm(a, a + 8 * int(rest[1]))
        elif '%08x' % a in funcs:
            disasm(a, a + funcs['%08x' % a]['size'])
        else:
            disasm(a, min(int(f, 16) for f in funcs if int(f, 16) > a))
    elif cmd == 'table':
        d, secs = sections()
        va = int(rest[0], 16)
        for i in range(int(rest[1])):
            t = read32(d, secs, va + 4 * i)
            print('%3d %#x %s' % (i, t or 0, (label(t) if t else '') or ''))
    elif cmd == 'classes':
        own = collections.defaultdict(set)
        for j in vtables():
            for s in j['slots']:
                own[j['class']].add(int(s['addr'], 16))
        count = collections.Counter(a for v in own.values() for a in v)
        rows = []
        for c, v in own.items():
            u = sorted(a for a in v if count[a] == 1)
            if u and (not rest or rest[0] in c):
                rows.append((statistics.median(u), c, len(u), u[0], u[-1]))
        for m, c, n, a, b in sorted(rows):
            print('%06x %-46s own=%3d %06x-%06x' % (int(m), c, n, a, b))
    elif cmd == 'vt':
        for j in vtables():
            if j['class'] == rest[0] and j['slots']:
                print('%s %s at %s' % (j['class'], j['symbol'], j['addr']))
                for s in j['slots']:
                    print('  %3d %s %s' % (s['i'], s['addr'], fname(s['addr'])))
    elif cmd == 'str':
        import bisect
        rx = re.compile(rest[0])
        starts = sorted(int(a, 16) for a in funcs)
        for j in load_jsonl(os.path.join(GH, 'strings.jsonl')):
            if not rx.search(j.get('value', '')):
                continue
            users = []
            for x in j.get('xrefs', []):
                if not re.fullmatch(r'[0-9a-fA-F]+', x):
                    users.append(x)
                    continue
                i = bisect.bisect_right(starts, int(x, 16)) - 1
                f = '%08x' % starts[i] if i >= 0 else None
                users.append('%s:%s' % (x, fname(f)) if f else x)
            print('%s %s  <- %s' % (j['addr'], esc(j['value'][:100]), ' '.join(users)))
    elif cmd == 'name':
        rx = re.compile(rest[0])
        for a in sorted(set(names) | set(funcs)):
            n = fname(a)
            if rx.search(n):
                print(a, n)
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
