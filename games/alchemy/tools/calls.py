#!/usr/bin/env python3
"""calls.py <start> <end>: the calls of a range of Alchemy.exe's code with the operands pushed before each (left-to-right
argument order), from llvm-objdump. A reading aid next to the decompilation, whose elided stack arguments it recovers."""
import subprocess, sys, re
out = subprocess.run(['bash', 'games/alchemy/tools/dis.sh', sys.argv[1], sys.argv[2]], capture_output=True, text=True).stdout
pushes = []
for l in out.splitlines():
    m = re.match(r'([0-9a-f]+):\s+(.*)', l)
    if not m: continue
    a, ins = m.groups()
    if ins.startswith('push '):
        pushes.append(ins[5:].strip())
    elif ins.startswith('call'):
        print('%s %s (%s)' % (a, ins[5:].strip(), ', '.join(reversed(pushes))))
        pushes = []
    elif ins.startswith(('j', 'ret')):
        pushes = []
