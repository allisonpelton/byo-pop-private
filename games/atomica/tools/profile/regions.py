"""The private memory of a process by allocation: what a 32-bit game holds in big blocks, for GAME_NOTES.md ("The software screen").

    python -I games/atomica/tools/profile/regions.py <pid> [<out file>]       one snapshot, as JSON lines
    python -I games/atomica/tools/profile/regions.py sum <file> [<file>...]   the snapshots' biggest allocations, as a table

A snapshot lists every allocation (VirtualQueryEx, grouped by allocation base) of type private with its committed bytes.
A heap block of 512 KB or more is an allocation of its own (the original's pictures at 4 bytes a pixel, a few of them), and
the smaller blocks sit together in a heap's segments, one big allocation each. perf.py takes a snapshot of the original or
the port at PERF_REGIONS=<seconds into the run> and writes it as regions.json in the run's folder.
"""
import ctypes
import ctypes.wintypes as W
import json
import struct
import sys

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CloseHandle.argtypes = [W.HANDLE]
MEM_COMMIT, MEM_PRIVATE = 0x1000, 0x20000


class MBI(ctypes.Structure):
    _fields_ = [('BaseAddress', ctypes.c_void_p), ('AllocationBase', ctypes.c_void_p), ('AllocationProtect', W.DWORD),
                ('PartitionId', W.WORD), ('RegionSize', ctypes.c_size_t), ('State', W.DWORD), ('Protect', W.DWORD),
                ('Type', W.DWORD)]


def snapshot(h):
    """[(allocation base, committed bytes, regions, protect of the first committed region)] of the private allocations."""
    k32.VirtualQueryEx.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.POINTER(MBI), ctypes.c_size_t]
    out = {}
    addr = 0
    m = MBI()
    while addr < 0x80000000 and k32.VirtualQueryEx(h, addr, ctypes.byref(m), ctypes.sizeof(m)):
        if m.State == MEM_COMMIT and m.Type == MEM_PRIVATE:
            a = out.setdefault(m.AllocationBase or 0, [0, 0, m.Protect])
            a[0] += m.RegionSize
            a[1] += 1
        addr = (m.BaseAddress or 0) + m.RegionSize
    return sorted(([b, v[0], v[1], v[2]] for b, v in out.items()), key=lambda x: -x[1])


def read(h, addr, size):
    buf = ctypes.create_string_buffer(size)
    n = ctypes.c_size_t()
    k32.ReadProcessMemory(h, ctypes.c_void_p(addr), buf, size, ctypes.byref(n))
    return buf.raw[:n.value]


def original_images(h, app=0x4cfc60, size=0x4478):
    """The original's pictures: every pointer in the app object that points at a sprite, which points at a DDImage or a
    MemoryImage (vtables 0x4b42cc and 0x4b435c). Each one's size and which pixel stores it holds: mBits (+0xc, 4 bytes a
    pixel), the palette (+0x10) and indices (+0x14, one byte a pixel), the alpha data (+0x1c), the DirectDraw surface
    (+0x34). The two kinds of image, as the original's Palletize (0x45f2e0) leaves them: bits dropped for indices."""
    data = read(h, app, size)
    out, seen = [], set()
    for i in range(0, len(data) - 3, 4):
        p = struct.unpack_from('<I', data, i)[0]
        if not (0x10000 < p < 0x7fff0000):
            continue
        sp = read(h, p, 0x20)
        if len(sp) < 0x20:
            continue
        img = struct.unpack_from('<I', sp, 8)[0]
        if (img in seen) or not (0x10000 < img < 0x7fff0000):
            continue
        im = read(h, img, 0x40)
        if len(im) < 0x40:
            continue
        d = struct.unpack('<16I', im)
        if d[0] not in (0x4b42cc, 0x4b435c) or not (0 < d[1] <= 8192) or not (0 < d[2] <= 8192):
            continue
        seen.add(img)
        out.append({'field': hex(i), 'image': hex(img), 'w': d[1], 'h': d[2], 'vtable': hex(d[0]), 'bits': d[3], 'table': d[4],
                    'indices': d[5], 'alpha': d[7], 'surface': d[13]})
    return out


def take(pid, path, app=False):
    h = k32.OpenProcess(0x400 | 0x10 | 0x1000, False, pid)
    if not h:
        return None
    s = snapshot(h)
    d = {'allocations': s, 'total': sum(x[1] for x in s)}
    if app:
        d['images'] = original_images(h)
    k32.CloseHandle(h)
    json.dump(d, open(path, 'w'))
    return s


def summarise(files):
    for f in files:
        d = json.load(open(f))
        s = d['allocations']
        big = [x for x in s if x[1] >= 128 * 1024]
        print('%s: %.1f MB private in %d allocations; %d of 128 KB or more hold %.1f MB' % (
            f, d['total'] / 1048576, len(s), len(big), sum(x[1] for x in big) / 1048576))
        if 'images' in d:
            ims = d['images']
            w = lambda i: i['w'] * i['h']
            print('  the original has %d pictures: %.2f MB at 4 bytes a pixel in %d with bits, %.2f MB of indices (+ palettes) in %d, '
                  '%d with a surface; %.2f MB of pixels in all' % (
                      len(ims), sum(4 * w(i) for i in ims if i['bits']) / 1048576, sum(1 for i in ims if i['bits']),
                      sum(w(i) for i in ims if i['indices']) / 1048576, sum(1 for i in ims if i['indices']),
                      sum(1 for i in ims if i['surface']), sum(w(i) for i in ims) / 1048576))
        # blocks of one size together: a picture of w x h pixels is w*h*4 bytes (+ a page's rounding and 0x20 of heap header)
        by = {}
        for b, size, n, p in big:
            by.setdefault(size, []).append(n)
        for size in sorted(by, reverse=True)[:60]:
            print('  %9d bytes (%6.2f MB) x %d' % (size, size / 1048576, len(by[size])))


if __name__ == '__main__':
    if len(sys.argv) > 2 and sys.argv[1] == 'sum':
        summarise(sys.argv[2:])
    elif len(sys.argv) > 1:
        r = take(int(sys.argv[1]), sys.argv[2] if len(sys.argv) > 2 else 'regions.json')
        print('failed' if r is None else '%d allocations' % len(r))
    else:
        sys.exit(__doc__)
