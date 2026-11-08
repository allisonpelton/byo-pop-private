#!/usr/bin/env python3
"""Print the export's decompilation of functions by address.

    python games/bejeweled2/tools/dec.py 5dd6cb 5dd7a0 ...   # one or more addresses (hex)
    python games/bejeweled2/tools/dec.py -r 5dbb36 5dd000     # every function in the range, in order
    python games/bejeweled2/tools/dec.py -l 5dbb36 5dd000     # only a list: address, size, callers
Reads re/bejeweled2/gh (the export tools/ghidra_import.sh made).
"""
import json
import os
import sys

GH = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 're', 'bejeweled2', 'gh')
_funcs = None


def funcs():
    global _funcs
    if _funcs is None:
        _funcs = {}
        for line in open(os.path.join(GH, 'functions.jsonl'), encoding='utf-8'):
            j = json.loads(line)
            _funcs[int(j['addr'], 16)] = j
    return _funcs


def body(addr):
    f = funcs().get(addr)
    if f is None:
        return '/* no function at %06x */\n' % addr
    ns = '_global' if f['ns'] == 'Global' else f['ns']
    path = os.path.join(GH, 'decomp', ns, '%s_%s.c' % (f['addr'], f['name']))
    if not os.path.exists(path):
        return '/* no decompilation at %s */\n' % path
    return open(path, encoding='utf-8', errors='replace').read()


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
