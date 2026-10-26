#!/usr/bin/env python3
"""Print functions of the Ghidra export (re/bookwormadv/gh/decomp) in a compact, readable form: names from
games/bookwormadv/docs/symbols/*.jsonl, strings inlined (for reading only, never written to a file in the
repository), local declarations dropped.

    python games/bookwormadv/tools/decread.py 717540 ...      # by address
    python games/bookwormadv/tools/decread.py -c 717540       # the callers and callees of a function, named
"""
import glob
import json
import os
import re
import sys

ROOT = 're/bookwormadv/gh'
strs = {}
for line in open(os.path.join(ROOT, 'strings.jsonl'), encoding='utf-8'):
    j = json.loads(line)
    if j.get('type') == 'string':
        strs[j['addr']] = j['value']
names = {}
for p in sorted(glob.glob('games/bookwormadv/docs/symbols/*.jsonl')):
    for line in open(p, encoding='utf-8'):
        line = line.strip()
        if line and not line.startswith('//'):
            j = json.loads(line)
            if 'name' in j:
                names[j['addr'].lower().zfill(8)] = j['name'].replace('::', '_')
funcs = {}
for line in open(os.path.join(ROOT, 'functions.jsonl'), encoding='utf-8'):
    j = json.loads(line)
    funcs[j['addr']] = j

decl = re.compile(r'^\s*(undefined\d?|int|uint|char|byte|bool|float|double|long|short|ushort|ulong|longlong|'
                  r'ulonglong|void|code|float10|[A-Za-z_]\w* \*+|[A-Za-z_]\w*)\s*\*?\s*\**[A-Za-z_]\w*(\[\d+\])?;\s*$')
BS = chr(92)


def esc(s):
    return '"' + s.replace(BS, BS + BS).replace('"', BS + '"').replace('\n', BS + 'n').replace('\r', BS + 'r') + '"'


def fname(addr):
    f = funcs.get(addr)
    n = names.get(addr) or (f['full'] if f else 'FUN_' + addr)
    return n.replace('::', '_')


def comp(t):
    t = re.sub(r'\b(?:thunk_)?FUN_([0-9a-f]{8})\b', lambda m: names.get(m.group(1), m.group(0)), t)
    t = re.sub(r's_\w+?_([0-9a-f]{8})\b', lambda m: esc(strs[m.group(1)]) if m.group(1) in strs else m.group(0), t)
    t = re.sub(r'&?(?:DAT|PTR_DAT|u)_([0-9a-f]{8})\b',
               lambda m: ('(str)' + esc(strs[m.group(1)])) if m.group(1) in strs else m.group(0), t)
    return t


STRLEN = re.compile(r'(\w+) = ([\w.\[\]*()+ ]+?);\n\s*do \{\n\s*(\w+) = \*\1;\n\s*\1 = \1 \+ 1;\n\s*\} while \(\3 != '
                    r"'\\0'\);\n(\s*)FUN_00401e00\(\2,\(int\)\1 - \(int\)\(\2 \+ 1\)\);")


def compact(t):
    """Folds the code VC8 inlines for std::string (the strlen loop before assign, the small-buffer setup, the
    release when the buffer is on the heap), the exception-state stores, the stack cookie and the checked
    iterators' _invalid_parameter_noinfo calls (-r keeps them)."""
    t = '\n'.join(ln for ln in t.split('\n') if not decl.match(ln))
    t = STRLEN.sub(lambda m: m.group(4) + 'FUN_00401e00(' + m.group(2) + ');', t)
    t = re.sub(r'\n\s*(\w+) = 0;\n\s*(\w+) = \(void \*\)\(\(uint\)\2 & 0xffffff00\);', '', t)
    t = re.sub(r'\n\s*(\w+) = \(void \*\)\(\(uint\)\1 & 0xffffff00\);', '', t)
    t = re.sub(r'\n\s*if \((0xf|0x7) < \w+\) \{\n\s*(_free|operator_delete)\([^;]*\);\n\s*\}', '', t)
    t = re.sub(r'\n\s*if \([^{}\n]*\) \{\n\s*FUN_007341dd\(\);\n\s*\}', '', t)
    t = re.sub(r'\n\s*FUN_007341dd\([^;]*\);', '', t)
    eh = set(re.findall(r'\b(\w+)\._0_1_ = ', t)) | set(re.findall(r'\b(\w+) = CONCAT31\(\1\._1_3_', t))
    for v in eh:
        t = re.sub(r'\n\s*' + v + r'(\._0_1_)? = (0x)?[0-9a-f]+;', '', t)
        t = re.sub(r'\n\s*' + v + r' = CONCAT31\(' + v + r'\._1_3_,[^;]*\);', '', t)
    t = re.sub(r'\n[^\n]*DAT_008140e8 \^[^\n]*', '', t)
    t = re.sub(r'\n\s*__security_check_cookie\([^;]*\);', '', t)
    t = re.sub(r'\n\s*local_\w+ = 0xf;', '', t)  # a std::string's capacity
    # locals only ever stored constants (the exception state, string sizes): dead stores
    for v in set(re.findall(r'\n\s*(local_\w+) = (?:0x)?[0-9a-f]+;', t)):
        if len(re.findall(r'\b' + v + r'\b', t)) == len(re.findall(r'\n\s*' + v + r' = (?:0x)?[0-9a-f]+;', t)):
            t = re.sub(r'\n\s*' + v + r' = (?:0x)?[0-9a-f]+;', '', t)
    return t


def show(a):
    fs = glob.glob(f'{ROOT}/decomp/*/{a}_*.c') + glob.glob(f'{ROOT}/decomp/{a}_*.c')
    if not fs:
        print('// no decomp for', a)
        return
    t = open(fs[0], encoding='latin1').read()
    if not RAW:
        t = compact(t)
    t = comp(t)
    out = []
    for ln in t.split('\n'):
        if not ln.strip() or decl.match(ln):
            continue
        if re.search(r'(pvStack_\w+|puStack_\w+|ExceptionList|local_4 = |uStack_4 = )', ln):
            continue
        out.append(ln)
    print('// %s  %s' % (a, fname(a)))
    print('\n'.join(out))
    print()


args = sys.argv[1:]
RAW = '-r' in args
args = [a for a in args if a != '-r']
callgraph = False
if args and args[0] == '-c':
    callgraph = True
    args = args[1:]
for a in args:
    a = a.lower()[2:] if a.lower().startswith('0x') else a.lower()
    a = a.zfill(8)
    if callgraph:
        f = funcs[a]
        print('%s %s size %d' % (a, fname(a), f['size']))
        print('  callers:', ' '.join('%s:%s' % (c, fname(c)) for c in f['callers']))
        print('  callees:', ' '.join('%s:%s' % (c, fname(c)) for c in f['callees']))
    else:
        show(a)
