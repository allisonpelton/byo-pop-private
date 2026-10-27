#!/usr/bin/env python3
"""Show the instructions before calls in the carved game exe: the decompiler hides stack arguments of some
functions (ret N) and the double that __ftol converts. Usage:

    python games/rocketmania/tools/callargs.py <start hex> <stop hex> [target hex ...] [-n LINES]

prints, for every call to one of the targets (default: __ftol 4aad80) between start and stop, the LINES (default 6)
instructions before it. 4259c0 takes its score as a stack argument; the rest of the Board's hidden arguments
are found the same way."""
import subprocess, sys, re

args = sys.argv[1:]
n = 6
if '-n' in args:
    i = args.index('-n')
    n = int(args[i + 1])
    del args[i:i + 2]
start, stop = args[0], args[1]
targets = [t.lower().lstrip('0x') if t.lower().startswith('0x') else t.lower() for t in args[2:]] or ['4aad80']
out = subprocess.run(['bash', 'build/rm_dis.sh', start, stop], capture_output=True, text=True).stdout.splitlines()
for i, line in enumerate(out):
    m = re.search(r'\bcall\s+0x([0-9a-f]+)', line)
    if m and m.group(1).lstrip('0') in [t.lstrip('0') for t in targets]:
        print('--- %s' % line.split('\t')[0].strip())
        for p in out[max(0, i - n):i + 1]:
            parts = p.split('\t')
            print('   ', parts[0].split(':')[0].strip(), ' '.join(x.strip() for x in parts[1:]))
