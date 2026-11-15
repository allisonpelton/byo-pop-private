# lostlocals.py [start end]: functions of the original's game code whose decompilation reads a scalar local that nothing assigns
# (Ghidra lost its definition: FinishBurn's `local_8 / 2` was the count of the lit fuses, and the port had read it as the number of
# rows). Each line: the function, then each such local with the first line that reads it. Arrays, structs and locals whose address
# is taken are not listed.
import re, sys
start, end = (int(sys.argv[1], 16), int(sys.argv[2], 16)) if len(sys.argv) > 2 else (0x401000, 0x434000)
funcs = {}
cur = None
for l in open('re/rocketmania/gh/all.c', encoding='latin-1'):
    m = re.match(r'^// ([0-9a-f]{8})  (.*)', l)
    if m:
        cur = (int(m.group(1), 16), m.group(2))
        funcs[cur] = []
    elif cur is not None:
        funcs[cur].append(l.rstrip('\n'))
tok = re.compile(r'\blocal_[0-9a-f]+\b')
decl = re.compile(r'^\s*([A-Za-z_][\w ]*?)\s*(\*?)\s*(local_[0-9a-f]+)\s*(\[\d+\])?;\s*$')
scalar = ('int', 'uint', 'float10', 'double', 'float', 'longlong', 'ulonglong', 'undefined4', 'undefined8', 'short', 'ushort')
assign = re.compile(r'^\s*(\[\d+\])?\s*(=(?!=)|\+=|-=|\*=|\|=|&=|\^=|<<=|>>=|\+\+|--)')
n = 0
for (a, name), lines in funcs.items():
    if not start <= a < end:
        continue
    skip, assigned, read = set(), set(), {}
    for i, l in enumerate(lines):
        d = decl.match(l)
        if d:
            if d.group(4) or d.group(2) or d.group(1).strip() not in scalar:
                skip.add(d.group(3))
            continue
        for m in tok.finditer(l):
            v = m.group(0)
            rest, before = l[m.end():], l[:m.start()].rstrip()
            if assign.match(rest) or (before.endswith('&') and not before.endswith('&&')) or before.endswith('++') or before.endswith('--'):
                assigned.add(v)
            else:
                read.setdefault(v, i)
    lost = [v for v in read if v not in assigned and v not in skip]
    if lost:
        n += 1
        print('%08x %s' % (a, name[:46]))
        for v in lost[:4]:
            print('     %s: %s' % (v, lines[read[v]].strip()[:110]))
print(n, 'functions')
