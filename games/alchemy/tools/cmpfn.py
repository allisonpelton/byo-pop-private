#!/usr/bin/env python3
"""cmpfn.py <address> <port source> <signature>: immcmp.py for the original's function at <address> and the port's function
whose definition line contains <signature> (up to its closing brace)."""
import sys, json, os, subprocess
here = os.path.dirname(os.path.abspath(__file__))
addr, path, sig = int(sys.argv[1], 16), sys.argv[2], sys.argv[3]
size = None
for l in open('re/alchemy/gh/functions.jsonl'):
    d = json.loads(l)
    if int(d['addr'], 16) == addr: size = d['size']
if size is None: raise SystemExit('no function at %x' % addr)
lines = open(path).read().splitlines()
start = next(i for i, l in enumerate(lines) if sig in l and l.rstrip().endswith(')'))
depth = 0; end = start; seen = False
for i in range(start, len(lines)):
    depth += lines[i].count('{') - lines[i].count('}')
    if '{' in lines[i]: seen = True
    if seen and depth == 0: end = i; break
print('%s: original %#x..%#x (%d bytes), port lines %d-%d' % (sig, addr, addr + size, size, start + 1, end + 1), flush=True)
subprocess.run([sys.executable, os.path.join(here, 'immcmp.py'), hex(addr), hex(addr + size), path, str(start + 1), str(end + 1)])
