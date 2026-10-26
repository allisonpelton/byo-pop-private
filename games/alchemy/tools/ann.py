#!/usr/bin/env python3
"""ann.py <start> <end>: calls.py's output (the calls of a range with the operands pushed before each) with the call
targets replaced by the names of re/alchemy/gh/functions.jsonl (the framework's graphics methods are named there)."""
import sys, json, re, os, subprocess
here = os.path.dirname(os.path.abspath(__file__))
names = {}
for l in open('re/alchemy/gh/functions.jsonl'):
    d = json.loads(l)
    names[int(d['addr'], 16)] = d['name']
out = subprocess.run([sys.executable, os.path.join(here, 'calls.py'), sys.argv[1], sys.argv[2]], capture_output=True, text=True).stdout
for l in out.splitlines():
    m = re.match(r'([0-9a-f]+) (\S+) \((.*)\)$', l)
    if not m:
        print(l); continue
    a, t, args = m.groups()
    try:
        n = names.get(int(t, 16), t)
    except ValueError:
        n = t
    print('%s %s(%s)' % (a, n.replace('FUN_00', 'F'), args))
