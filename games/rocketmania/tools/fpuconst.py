# fpuconst.py <start> <end> [--exe PATH]: the x87 instructions with a constant operand (fadd, fsub, fmul, fdiv, fld, fcom... on a
# float or double in the exe's data) between two virtual addresses, with the constant's value. Ghidra's decompilation drops x87
# operations it cannot lift, so a constant added to a position (the fuse flame's - 7.0 and - 8.0) can be missing from the port;
# this lists every one of them, for a function or a whole file's range, to be set beside the port's code.
import re, struct, subprocess, sys

OBJDUMP = r'E:\retools\llvm-mingw-20261006-ucrt-x86_64\bin\llvm-objdump.exe'
args = [a for a in sys.argv[1:] if not a.startswith('--')]
exe = 're/rocketmania/bin/RocketManiaGame.exe'
if '--exe' in sys.argv:
    exe = sys.argv[sys.argv.index('--exe') + 1]
    args = [a for a in args if a != exe]
start, end = int(args[0], 16), int(args[1], 16)
data = open(exe, 'rb').read()
pe = struct.unpack_from('<I', data, 0x3c)[0]
nsec = struct.unpack_from('<H', data, pe + 6)[0]
opt = struct.unpack_from('<H', data, pe + 20)[0]
base = struct.unpack_from('<I', data, pe + 24 + 28)[0]
secs = []
off = pe + 24 + opt
for i in range(nsec):
    vsize, va, rsize, raw = struct.unpack_from('<IIII', data, off + 8)
    secs.append((base + va, max(vsize, rsize), raw))
    off += 40


def read(addr, size):
    for va, n, raw in secs:
        if va <= addr < va + n:
            return data[raw + addr - va: raw + addr - va + size]
    return None


out = subprocess.run([OBJDUMP, '-d', '--x86-asm-syntax=intel', '--start-address=0x%x' % start, '--stop-address=0x%x' % end, exe],
                     capture_output=True, text=True).stdout
pat = re.compile(r'^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*(f\w+)\s+(qword|dword) ptr \[(?:ds:)?0x([0-9a-f]+)\]')
for line in out.splitlines():
    m = pat.match(line)
    if not m:
        continue
    a, op, kind, c = m.group(1), m.group(2), m.group(3), int(m.group(4), 16)
    raw = read(c, 8 if kind == 'qword' else 4)
    if raw is None or len(raw) < (8 if kind == 'qword' else 4) or op in ('fstp', 'fst', 'fistp', 'fist'):
        continue
    if op in ('fild', 'fiadd', 'fisub', 'fimul', 'fidiv'):
        v = struct.unpack('<i', raw[:4])[0] if kind == 'dword' else struct.unpack('<q', raw)[0]
    else:
        v = struct.unpack('<d', raw)[0] if kind == 'qword' else struct.unpack('<f', raw)[0]
    print('%s %-6s %s %s' % (a, op, hex(c), v))
