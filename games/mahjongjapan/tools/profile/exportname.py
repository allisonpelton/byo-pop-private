# Names a system DLL's code addresses by its export table: the nearest export at or below each offset (an offset is the one
# callprof.py prints after "+0x"). Only exports are known (no symbol files), so a name is the function the address lies in
# only when that function is exported or the next export is nearby.
#   python exportname.py C:/Windows/SysWOW64/ntdll.dll 0x1234 0x5678 ...
import struct
import sys

path, offsets = sys.argv[1], [int(a, 16) for a in sys.argv[2:]]
d = open(path, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]
opt = pe + 24
magic = struct.unpack_from('<H', d, opt)[0]
dirs = opt + (96 if magic == 0x10b else 112)
exp_rva, exp_size = struct.unpack_from('<II', d, dirs)
secs = []
s = opt + struct.unpack_from('<H', d, pe + 20)[0]
for i in range(nsec):
    vsize, va, rawsize, raw = struct.unpack_from('<IIII', d, s + 40 * i + 8)
    secs.append((va, vsize, raw))


def off(rva):
    for va, vsize, raw in secs:
        if va <= rva < va + max(vsize, 1):
            return rva - va + raw
    raise ValueError(rva)


e = off(exp_rva)
n_funcs, n_names = struct.unpack_from('<II', d, e + 20)
funcs_rva, names_rva, ords_rva = struct.unpack_from('<III', d, e + 28)
names = {}
for i in range(n_names):
    nr = struct.unpack_from('<I', d, off(names_rva) + 4 * i)[0]
    o = struct.unpack_from('<H', d, off(ords_rva) + 2 * i)[0]
    end = d.index(b'\0', off(nr))
    names[o] = d[off(nr):end].decode()
table = []
for o in range(n_funcs):
    rva = struct.unpack_from('<I', d, off(funcs_rva) + 4 * o)[0]
    if rva and not (exp_rva <= rva < exp_rva + exp_size):
        table.append((rva, names.get(o, 'ordinal %d' % o)))
table.sort()
import bisect
keys = [t[0] for t in table]
for a in offsets:
    k = bisect.bisect_right(keys, a) - 1
    print('0x%x  %s +0x%x' % (a, table[k][1] if k >= 0 else '?', a - table[k][0] if k >= 0 else 0))
