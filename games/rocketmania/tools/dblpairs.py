# dblpairs.py [start end]: the doubles the original builds out of two 32-bit immediate stores (mov [reg+n], lo; mov [reg+n+4], hi),
# read from the disassembly so that a decompiler showing them as two halves cannot hide them, set against the numbers in the port's
# function of the same address. Prints one line for each value the port's function does not contain.
import bisect, glob, os, re, struct, subprocess, sys
OBJDUMP = r'E:\retools\llvm-mingw-20261006-ucrt-x86_64\bin\llvm-objdump.exe'
here = os.path.dirname(os.path.abspath(__file__))
start, end = (int(sys.argv[1], 16), int(sys.argv[2], 16)) if len(sys.argv) > 2 else (0x401000, 0x434000)
out = subprocess.run([OBJDUMP, '-d', '--x86-asm-syntax=intel', '--start-address=%#x' % start, '--stop-address=%#x' % end,
                      're/rocketmania/bin/RocketManiaGame.exe'], capture_output=True, text=True).stdout
funcs = sorted(int(l.split('"addr":"')[1][:8], 16) for l in open('re/rocketmania/gh/functions.jsonl') if '"addr":"' in l)
store = re.compile(r'^\s*([0-9a-f]+):\s.*mov\s+dword ptr \[(\w+)(?: ([+-]) (0x[0-9a-f]+))?\], (-?0x[0-9a-f]+)')
stores = []
for l in out.split('\n'):
    m = store.match(l)
    if m:
        a = int(m.group(1), 16)
        off = int(m.group(4), 16) * (-1 if m.group(3) == '-' else 1) if m.group(4) else 0
        stores.append((a, m.group(2), off, int(m.group(5), 16) & 0xffffffff))
port = {}
for f in sorted(glob.glob(os.path.join(here, '..', 'src', '*.cpp')) + glob.glob(os.path.join(here, '..', 'src', '*.inc'))):
    lines = open(f, encoding='latin-1').read().split('\n')
    marks = [(i, int(m.group(1), 16)) for i, l in enumerate(lines) for m in [re.search(r'0x(4[0-9a-f]{5})\b', l)] if m and l.strip().startswith('//')]
    for k, (i, a) in enumerate(marks):
        j = marks[k + 1][0] if k + 1 < len(marks) else len(lines)
        port.setdefault(a, []).append((os.path.basename(f), i + 1, '\n'.join(lines[i:j])))
seen = set()
for i, (a, reg, off, v) in enumerate(stores):
    for j in range(i + 1, min(i + 14, len(stores))):
        a2, reg2, off2, v2 = stores[j]
        if reg2 == reg and off2 == off + 4 and a2 - a < 0x60:
            d = struct.unpack('<d', struct.pack('<Q', v2 << 32 | v))[0]
            if v2 >> 20 in range(0x3e0, 0x41f) or v2 >> 20 in range(0xbe0, 0xc1f):
                if (a, off) in seen: break
                seen.add((a, off))
                fn = funcs[bisect.bisect_right(funcs, a) - 1]
                body = '\n'.join(p[2] for p in port.get(fn, []))
                forms = {'%.15g' % d, '%.10g' % d, '%.6g' % d, '%.4g' % d, '%g' % d, repr(d)}
                ok = any(re.search(r'(?<![\w.])' + re.escape(t) + r'(?![\w])', body) for t in forms)
                if not ok:
                    loc = port[fn][0][0] + ':' + str(port[fn][0][1]) if fn in port else 'no port function'
                    print('%08x +%#x %-22s %s' % (fn, off, repr(d), loc))
            break
