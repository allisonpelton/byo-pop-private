#!/usr/bin/env python3
"""Turn a scenario file (the original's terms, see compare.py) into a script for the port: one update later, a typed text
as one character at a time, and the quit line. tools/check.sh's atomica suite uses it to replay the scenarios in the
current port and the frozen baseline (build/baseline-atomica).

    python games/atomica/tools/identical/portscript.py <scenario.txt> <script out> [--end N]
"""
import sys

end = None
lines = []
for l in open(sys.argv[1]):
    w = l.split('#')[0].split()
    if not w:
        continue
    if w[0] == 'end':
        end = int(w[1])
    else:
        lines.append((int(w[0]), w[1], w[2:]))
if '--end' in sys.argv:
    end = int(sys.argv[sys.argv.index('--end') + 1])
with open(sys.argv[2], 'w', newline='\n') as f:
    for n, c, a in lines:
        if n >= end:
            continue
        if c == 'text':
            for ch in ' '.join(a):
                f.write('%d char %s\n' % (n + 1, 'SPACE' if ch == ' ' else ch))
        else:
            f.write('%d %s %s\n' % (n + 1, c, ' '.join(a)))
    f.write('%d quit\n' % (end + 1))
