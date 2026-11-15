"""nonelines.py [--report report.md] [--tsv out.tsv]: the coverage check's `none` lines for the code the port has by
other means, found from the export (re/bookwormadv/gh) alone: printed for coverage.txt, ranges where the functions
of one kind follow each other in the exe.

  wrapper   a slot of a LuaLib::MethodWrapper<> / RawMethodWrapper<> / MethodWrapperBase vtable (the Lua call and the
            deleting destructor of each bound method's wrapper): lualib/LuaBind.h's templates make them
  repeated  a body the compiler emitted three or more times, the same but for the addresses it calls (one template
            instance for each type), with no strings but the C++ library's own and in no vtable but the C++
            library's: C++ library code
  deleting  a scalar deleting destructor: the destructor, then free when bit 0 of its flag is set (compiler-made)
  binding   the rest of the Lua binding templates' instances: each wrapper's constructor (it sets a MethodWrapper<>
            vtable) and each Class<T>::Method<F>(name), which makes the wrapper and adds it to the class (AddMethod)

With --report, the report's first list ("Reached in the original, with no definition in the port") is sorted too, and
--tsv writes each of its functions with its kind (the three above, or "read": the rest, to be read)."""
import json
import os
import re
import sys
from collections import defaultdict

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
GH = os.path.join(ROOT, 're', 'bookwormadv', 'gh')
if not os.path.isdir(os.path.join(GH, 'decomp')):   # a worktree's re/ may lack the decompiled code: the main checkout's
    GH = os.path.normpath(os.path.join(ROOT, '..', '..', '..', 're', 'bookwormadv', 'gh'))
GAME = ((0x401000, 0x4cce30), (0x527ea0, 0x52efc0), (0x650000, 0x71efb0))
STD_STRINGS = {'string too long', 'invalid string position', 'vector<T> too long', 'invalid map/set<T> iterator',
               'list<T> too long', 'deque<T> too long', 'map/set<T> too long', 'invalid vector<T> subscript',
               'bad allocation', 'invalid deque<T> subscript', 'invalid argument'}
FREE = '00733dad'   # free (operator delete)
WHY = {
    'wrapper': 'LuaLib::MethodWrapper<> slots (the Lua call and the deleting destructor of each bound method\'s '
               'wrapper): lualib/LuaBind.h\'s templates',
    'repeated': 'a body the compiler emitted for several types (template instances): C++ library code',
    'deleting': 'scalar deleting destructor (compiler-made: the destructor, then free)',
    'binding': "LuaLib::Class<>::Method<> instances (a bound method's wrapper constructed and added to its Lua class): "
               "lualib/LuaBind.h's templates",
}
ADD_METHOD = '006d2400'     # LuaSupport's AddMethod ("AddMethod: unable to find class")
# what a Method<>(name) instance calls besides its wrapper's constructor: new, free, the string's assigns, AddMethod,
# the stack cookie's check and the checked iterators' report
METHOD_CALLS = {'00733ee7', FREE, '00401e00', '00401c30', ADD_METHOD, '00733ac6', '007341dd'}


def in_game(a):
    return any(lo <= a <= hi for lo, hi in GAME)


def body(text):
    """A function's decompiled body with its addresses, its own name and the decompiler's numbering made neutral."""
    text = text.split('\n', 1)[1] if text.startswith('//') else text
    text = re.sub(r'\b(FUN|DAT|LAB|PTR|thunk_FUN|switchD|caseD|s|u|_?ptr)_[0-9a-fA-F]{6,8}\w*', 'X', text)
    text = re.sub(r'0x[0-9a-f]{6,8}\b', 'A', text)
    return re.sub(r'\s+', ' ', text).strip()


def main():
    args = sys.argv[1:]
    report = args[args.index('--report') + 1] if '--report' in args else None
    tsv = args[args.index('--tsv') + 1] if '--tsv' in args else None
    fns = {}
    for line in open(os.path.join(GH, 'functions.jsonl'), encoding='utf-8'):
        f = json.loads(line)
        if not f['thunk']:
            fns[f['addr']] = f
    paths = {}
    for d, _, files in os.walk(os.path.join(GH, 'decomp')):
        for n in files:
            paths[n[:8]] = os.path.join(d, n)
    kind, virtual = {}, set()
    for line in open(os.path.join(GH, 'vtables.jsonl'), encoding='utf-8'):
        v = json.loads(line)
        if not v['class'].startswith('std::'):
            virtual.update(s['addr'] for s in v['slots'])
        if v['class'].split('<')[0] in ('LuaLib::MethodWrapper', 'LuaLib::RawMethodWrapper', 'LuaLib::MethodWrapperBase'):
            for s in v['slots']:
                if s['addr'] in fns and in_game(int(s['addr'], 16)):
                    kind[s['addr']] = 'wrapper'
    bodies = defaultdict(list)
    for a, f in fns.items():
        if not in_game(int(a, 16)) or a in kind or a not in paths:
            continue
        if any(s not in STD_STRINGS for s in f['strings']):
            continue
        text = open(paths[a], encoding='utf-8', errors='replace').read()
        b = body(text)
        bodies[b].append(a)
        if f['size'] < 80 and FREE in f['callees'] and re.search(r'\(\w+ & 1\) != 0', b):
            kind[a] = 'deleting'
        elif f['size'] <= 120 and re.search(r'MethodWrapper<.*:: ?vftable', b):
            kind[a] = 'binding'     # a wrapper's constructor: its vtable set
    for a, f in fns.items():
        if in_game(int(a, 16)) and a not in kind and ADD_METHOD in f['callees'] and len(set(f['strings'])) <= 1 and \
                all(c in METHOD_CALLS or kind.get(c) == 'binding' for c in f['callees']):
            kind[a] = 'binding'     # Class<T>::Method<F>(name): the wrapper made and added to the class
    for b, addrs in bodies.items():
        if len(addrs) >= 3:
            for a in addrs:
                if a not in virtual:    # a game class's virtual (an empty one, a constant) is read, not assumed
                    kind.setdefault(a, 'repeated')
    # the lines: runs of one kind with nothing else of the export between them
    order = sorted(a for a in fns if in_game(int(a, 16)))
    lines, run = [], None
    for a in order + [None]:
        k = kind.get(a) if a else None
        if run and k == run[0]:
            run[2] = a
            continue
        if run:
            lines.append('none %s %s' % (run[1] if run[1] == run[2] else '%s-%s' % (run[1], run[2]), WHY[run[0]]))
        run = [k, a, a] if k else None
    counts = defaultdict(int)
    for k in kind.values():
        counts[k] += 1
    print('# nonelines.py: %s' % ', '.join('%d %s' % (n, k) for k, n in sorted(counts.items())))
    print('\n'.join(lines))
    if report:
        rows, section = [], False
        for line in open(report, encoding='utf-8'):
            if line.startswith('## '):
                section = line.startswith('## Reached in the original')
            elif section and line.startswith('| 0'):
                a = line.split('|')[1].strip()
                rows.append((a, kind.get(a, 'read')))
        c = defaultdict(int)
        for _, k in rows:
            c[k] += 1
        sys.stderr.write('the report\'s first list: %s (%d)\n' % (', '.join('%d %s' % (n, k) for k, n in sorted(c.items())), len(rows)))
        if tsv:
            with open(tsv, 'w', encoding='utf-8', newline='\n') as o:
                for a, k in rows:
                    f = fns.get(a, {})
                    o.write('%s\t%s\t%d\t%d\n' % (a, k, f.get('size', 0), len(f.get('callers', []))))


main()
