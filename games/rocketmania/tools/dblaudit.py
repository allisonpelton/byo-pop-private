# dblaudit.py [start end]: doubles the original stores as two 32-bit halves, which Ghidra shows as hex words (puVar[3] = 0x403d0000 with
# puVar[2] = 0 is 29.0). The port was read from that text, and a double left as its halves reads as 0. Each line: the function, the
# store, the double (the low half is taken as the previous line's literal when it is one, else 0) and whether the port's function
# text has the number.
import bisect, glob, json, os, re, struct, sys
here = os.path.dirname(os.path.abspath(__file__))
start, end = (int(sys.argv[1], 16), int(sys.argv[2], 16)) if len(sys.argv) > 2 else (0x401000, 0x434000)
text = {}
cur = None
for l in open('re/rocketmania/gh/all.c', encoding='latin-1'):
    m = re.match(r'^// ([0-9a-f]{8})  (.*)', l)
    if m:
        cur = int(m.group(1), 16)
        text[cur] = []
    elif cur is not None:
        text[cur].append(l.rstrip('\n'))
port = {}
for f in sorted(glob.glob(os.path.join(here, '..', 'src', '*.cpp'))):
    lines = open(f, encoding='latin-1').read().split('\n')
    marks = [(i, int(m.group(1), 16)) for i, l in enumerate(lines) for m in [re.search(r'0x(4[0-9a-f]{5})\b', l)] if m and l.strip().startswith('//')]
    for k, (i, a) in enumerate(marks):
        j = marks[k + 1][0] if k + 1 < len(marks) else len(lines)
        port.setdefault(a, []).append((os.path.basename(f), i + 1, '\n'.join(lines[i:j])))
entries = sorted(text)
hi = re.compile(r'^\s*(.+?)\s*= (0x(?:3f|40|bf|c0)[0-9a-f]{6});')
for a in sorted(text):
    if not start <= a < end:
        continue
    ls = text[a]
    for i, l in enumerate(ls):
        m = hi.match(l)
        if not m:
            continue
        lhs, h = m.group(1), int(m.group(2), 16)
        lo = 0
        for k in range(max(0, i - 3), i):
            mm = re.match(r'^\s*.+?\s*= (0x[0-9a-f]+|\d+);', ls[k])
            if mm and ls[k].strip().split('=')[0].strip() != lhs:
                pass
        v = struct.unpack('<d', struct.pack('<Q', h << 32 | lo))[0]
        forms = {'%g' % v, '%.1f' % v, '%.2f' % v, '%.3f' % v, '%.4f' % v, '%.6f' % v, '%.8f' % v, ('%d' % v) if v == int(v) else '%g' % v}
        body = '\n'.join(p[2] for p in port.get(a, []))
        # the low half carries digits for most values (0x3f70624d / 0 is 0.00390625... ): match on the first four digits too
        short = {('%.4g' % v)}
        ok = any(re.search(r'(?<![\w.])' + re.escape(t) + r'(?![\w])', body) for t in forms | short)
        loc = port[a][0][0] + ':' + str(port[a][0][1]) if a in port else '-'
        if not ok:
            print('%08x %-34s %-14s %s  port %s' % (a, l.strip()[:34], m.group(2), '%.6g' % v, loc))
