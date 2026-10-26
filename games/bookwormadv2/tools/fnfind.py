#!/usr/bin/env python3
"""Find functions of BookwormAdventuresVol2.exe in the Ghidra export (re/bookwormadv2/gh/functions.jsonl) by the
strings they use, by namespace or by address range, and print address, size, namespace and name.

    python games/bookwormadv2/tools/fnfind.py -s "AddMethod: unable"   # functions using a string (substring)
    python games/bookwormadv2/tools/fnfind.py -n LuaLib::               # by namespace prefix
    python games/bookwormadv2/tools/fnfind.py -r 6c0000 6d0000          # by address range
    add -v to print each function's strings too (for reading only: never copy them into the repository)
"""
import json
import sys

args = sys.argv[1:]
verbose = '-v' in args
args = [a for a in args if a != '-v']
mode, rest = args[0], args[1:]
rows = []
for line in open('re/bookwormadv2/gh/functions.jsonl', encoding='utf-8'):
    j = json.loads(line)
    a = int(j['addr'], 16)
    if mode == '-s':
        ok = any(any(w in s for s in j.get('strings', [])) for w in rest)
    elif mode == '-n':
        ok = any(j.get('ns', '').startswith(w.rstrip(':')) for w in rest)
    elif mode == '-r':
        ok = int(rest[0], 16) <= a < int(rest[1], 16)
    else:
        sys.exit(__doc__)
    if ok:
        rows.append(j)
rows.sort(key=lambda j: j['addr'])
for j in rows:
    print('%s %5d %s' % (j['addr'], j['size'], j['full']))
    if verbose and j.get('strings'):
        print('        ', ' | '.join(s[:60] for s in j['strings'])[:300])
print('%d functions, %d bytes' % (len(rows), sum(j['size'] for j in rows)))
