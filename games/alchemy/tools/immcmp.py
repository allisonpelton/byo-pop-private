#!/usr/bin/env python3
"""immcmp.py <start> <stop> <port source> <first line> <last line>: the integer immediates of a range of the original's code
(operands that are not memory displacements or addresses) that the port's function does not contain, and the port's literals
the original does not contain. A first pass of the fidelity check: a coordinate that the decompiler dropped shows up here."""
import sys, re, os, subprocess
here = os.path.dirname(os.path.abspath(__file__))
start, stop, path, l1, l2 = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5])
out = subprocess.run(['bash', os.path.join(here, 'dis.sh'), start, stop], capture_output=True, text=True).stdout
orig = {}
for l in out.splitlines():
    m = re.match(r'([0-9a-f]+):\s+(.*)', l)
    if not m: continue
    a, ins = m.groups()
    ins = ins.split(' #')[0]
    s = re.sub(r'\[[^\]]*\]', '[]', ins)      # displacements are structure offsets, not values
    if ins.startswith(('call', 'j')): continue
    for v in re.findall(r'(?<![\w.])(-?0x[0-9a-f]+)\b', s):
        n = int(v, 16)
        if n >= 0x400000 or abs(n) < 3: continue
        orig.setdefault(n, a)
port = {}
for ln in open(path).read().splitlines()[l1 - 1:l2]:
    for v in re.findall(r'(?<![\w.])(0x[0-9a-fA-F]+|\d+)\b', re.sub(r'//.*', '', ln)):
        port[int(v, 0)] = 1
print('in original, not in port:', ', '.join('%#x@%s' % (n, orig[n]) for n in sorted(orig) if n not in port and -n not in port))
print('in port, not in original:', ', '.join(hex(n) for n in sorted(port) if n not in orig and -n not in orig and n >= 3))
