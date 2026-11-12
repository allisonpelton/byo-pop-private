# fpuaudit.py [start end]: the x87 constants of the original's game code (fpuconst.py) that Ghidra's decompilation of the function does
# not show. The port is translated from the decompilation, so a constant the decompiler dropped (the fuse flame's 7.0 and 8.0 added
# to its position) is missing from the port unless someone reads the disassembly. Each line: the instruction, its constant, the
# function's entry, and where the port has that entry in a comment.
import bisect, glob, json, os, re, subprocess, sys
here = os.path.dirname(os.path.abspath(__file__))
start, end = (sys.argv[1], sys.argv[2]) if len(sys.argv) > 2 else ('401000', '440000')
consts = subprocess.run([sys.executable, os.path.join(here, 'fpuconst.py'), start, end], capture_output=True, text=True).stdout.splitlines()
entries = sorted(int(json.loads(l)['addr'], 16) for l in open('re/rocketmania/gh/functions.jsonl') if l.strip())
text = {}
cur = None
for l in open('re/rocketmania/gh/all.c', encoding='latin-1'):
    m = re.match(r'^// ([0-9a-f]{8})  ', l)
    if m:
        cur = int(m.group(1), 16)
        text[cur] = []
    elif cur is not None:
        text[cur].append(l)
port = {}
for f in sorted(glob.glob(os.path.join(here, '..', 'src', '*.cpp'))):
    for i, l in enumerate(open(f, encoding='latin-1')):
        for m in re.finditer(r'0x(4[0-9a-f]{5})\b', l):
            port.setdefault(int(m.group(1), 16), '%s:%d' % (os.path.basename(f), i + 1))


def forms(v):
    out = set()
    for fmt in ('%g', '%.1f', '%.2f', '%.3f', '%.4f', '%.5f', '%.6f', '%.7f', '%.8f', '%.10f', '%.12f'):
        t = fmt % v
        out.add(t)
        if '.' in t:
            out.add(t.rstrip('0').rstrip('.'))
            out.add(t.rstrip('0').rstrip('.') + '.0')
    if v == int(v):
        out.add('%d' % v)
        out.add('0x%x' % int(v))
    return out


hidden = 0
for c in consts:
    a, op, addr, val = c.split()
    v = float(val)
    entry = entries[bisect.bisect_right(entries, int(a, 16)) - 1]
    body = ''.join(text.get(entry, []))
    if any(re.search(r'(?<![\w.])' + re.escape(t) + r'(?![\w.]*\w)', body) for t in forms(v)):
        continue
    hidden += 1
    print('%s %-6s %-14s function 0x%x port %s' % (a, op, val, entry, port.get(entry, '-')))
print('%d of %d constants are not in the decompilation' % (hidden, len(consts)))
