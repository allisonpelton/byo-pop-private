#!/usr/bin/env python3
"""Print the export's decompilation of functions by address.

    python games/bejeweled2/tools/dec.py 5dd6cb 5dd7a0 ...   # one or more addresses (hex)
    python games/bejeweled2/tools/dec.py -r 5dbb36 5dd000     # every function in the range, in order
    python games/bejeweled2/tools/dec.py -l 5dbb36 5dd000     # only a list: address, size, callers
Reads re/bejeweled2/gn (the export with the Mac-derived names; gh, the plain one, when there is no gn). Substitutes the
resource globals' names (re/bejeweled2/res_map.txt) and the names this session gave functions and globals
(re/bejeweled2/names.txt: '<hex address> <name>' per line) for their placeholders.
"""
import json
import os
import re
import sys

RE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 're', 'bejeweled2')
GH = os.path.join(RE, 'gn') if os.path.isdir(os.path.join(RE, 'gn', 'decomp')) else os.path.join(RE, 'gh')
_funcs = None


def funcs():
    global _funcs
    if _funcs is None:
        _funcs = {}
        for line in open(os.path.join(GH, 'functions.jsonl'), encoding='utf-8'):
            j = json.loads(line)
            _funcs[int(j['addr'], 16)] = j
    return _funcs


_names = None


def rename(text):
    global _names
    if _names is None:
        _names = {}
        for fn in ('res_map.txt', 'names.txt'):
            path = os.path.join(RE, fn)
            if os.path.exists(path):
                for line in open(path, encoding='utf-8'):
                    parts = line.split()
                    if len(parts) >= 2:
                        _names[int(parts[0], 16)] = parts[1]
    return re.sub(r'(thunk_FUN_|FUN_|DAT_|PTR_DAT_|_DAT_)([0-9a-f]{8})',
                  lambda m: _names.get(int(m.group(2), 16), m.group(0)), text)


def body(addr):
    f = funcs().get(addr)
    if f is None:
        return '/* no function at %06x */\n' % addr
    ns = '_global' if f['ns'] == 'Global' else f['ns'].replace('::', '__')
    path = os.path.join(GH, 'decomp', ns, '%s_%s.c' % (f['addr'], f['name']))
    if not os.path.exists(path):
        return '/* no decompilation at %s */\n' % path
    return rename(open(path, encoding='utf-8', errors='replace').read())


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    if a[0] in ('-r', '-l'):
        lo, hi = int(a[1], 16), int(a[2], 16)
        for addr in sorted(funcs()):
            if lo <= addr < hi:
                f = funcs()[addr]
                if a[0] == '-l':
                    print('%06x %5d %-28s callers=%d %s' % (addr, f['size'], f['full'][:28], len(f['callers']), ','.join(f['callers'][:3])))
                else:
                    print('// ---- %06x %s (%d bytes)' % (addr, f['full'], f['size']))
                    print(body(addr))
    else:
        for x in a:
            print(body(int(x, 16)))


main()
