# Memory and CPU profile of a game run: samples the child's working set, private bytes and CPU time, and at given
# seconds takes a snapshot of its resident pages by owner (each DLL/exe, heap and other private memory, mapped files).
# Run under rundesk from the worktree root: python memprof.py <out-dir> <snap-seconds,...> [env K=V ...] -- <exe> <args>
import ctypes, ctypes.wintypes as W, json, os, subprocess, sys, time
from collections import defaultdict

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


class MBI(ctypes.Structure):
    _fields_ = [('BaseAddress', ctypes.c_uint64), ('AllocationBase', ctypes.c_uint64), ('AllocationProtect', W.DWORD),
                ('__a', W.DWORD), ('RegionSize', ctypes.c_uint64), ('State', W.DWORD), ('Protect', W.DWORD),
                ('Type', W.DWORD), ('__b', W.DWORD)]


MEM_COMMIT, MEM_IMAGE, MEM_MAPPED, MEM_PRIVATE = 0x1000, 0x1000000, 0x40000, 0x20000
k32.VirtualQueryEx.argtypes = [W.HANDLE, ctypes.c_uint64, ctypes.POINTER(MBI), ctypes.c_size_t]
k32.VirtualQueryEx.restype = ctypes.c_size_t
psapi.GetMappedFileNameW.argtypes = [W.HANDLE, ctypes.c_uint64, W.LPWSTR, W.DWORD]
psapi.QueryWorkingSet.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD]


def memory(h):
    m = PMC(); m.cb = ctypes.sizeof(PMC)
    psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb)
    return m


def cpu(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    f = lambda t: ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7
    return f(c) + f(d)


def snapshot(h):
    # resident pages: QueryWorkingSet lists each page with its shared bit
    n = 1 << 18
    while True:
        buf = (ctypes.c_size_t * (n + 1))()
        if psapi.QueryWorkingSet(h, buf, ctypes.sizeof(buf)):
            break
        n *= 2
    pages = {}
    for i in range(buf[0]):
        e = buf[1 + i]
        pages[e >> 12] = (e >> 8) & 1
    regions, addr = [], 0
    mbi = MBI()
    while addr < (1 << 32) and k32.VirtualQueryEx(h, addr, ctypes.byref(mbi), ctypes.sizeof(mbi)):
        if mbi.State == MEM_COMMIT:
            name = ''
            if mbi.Type in (MEM_IMAGE, MEM_MAPPED):
                s = ctypes.create_unicode_buffer(520)
                if psapi.GetMappedFileNameW(h, mbi.AllocationBase, s, 520):
                    name = os.path.basename(s.value)
            kind = {MEM_IMAGE: 'image', MEM_MAPPED: 'mapped', MEM_PRIVATE: 'private'}.get(mbi.Type, '?')
            regions.append((mbi.BaseAddress, mbi.RegionSize, kind, name, mbi.Protect))
        addr = mbi.BaseAddress + mbi.RegionSize
    owners = defaultdict(lambda: [0, 0, 0])  # committed, resident private, resident shared
    for base, size, kind, name, prot in regions:
        key = '%s %s' % (kind, name) if name else kind
        o = owners[key]
        o[0] += size
        for vpn in range(base >> 12, (base + size) >> 12):
            if vpn in pages:
                o[2 if pages[vpn] else 1] += 4096
    return {k: v for k, v in owners.items()}


def main():
    out = sys.argv[1]
    snaps = [float(x) for x in sys.argv[2].split(',') if x]
    i = 3
    env = dict(os.environ)
    while sys.argv[i] != '--':
        kk, vv = sys.argv[i].split('=', 1)
        env[kk] = vv
        i += 1
    cmd = sys.argv[i + 1:]
    os.makedirs(out, exist_ok=True)
    p = subprocess.Popen(cmd, cwd=out, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    h = int(p._handle)
    t0 = time.time()
    samples, result = [], {'snapshots': {}}
    while p.poll() is None:
        t = time.time() - t0
        try:
            m = memory(h)
            samples.append((round(t, 2), m.WorkingSetSize, m.PrivateUsage, round(cpu(h), 3)))
            if snaps and t >= snaps[0]:
                result['snapshots'][str(snaps.pop(0))] = snapshot(h)
                result['peak'] = (m.PeakWorkingSetSize, m.PeakPagefileUsage)
        except OSError:
            pass
        time.sleep(0.2)
    result['exit'] = p.returncode
    result['samples'] = samples
    with open(os.path.join(out, 'memprof.json'), 'w') as f:
        json.dump(result, f)
    MB = 1 << 20
    print('exit %s, %.1f s, peak working set %.1f MB' % (p.returncode, samples[-1][0] if samples else 0,
          max(s[1] for s in samples) / MB if samples else 0))
    for t, s in result['snapshots'].items():
        tot = [sum(v[j] for v in s.values()) / MB for j in range(3)]
        print('at %s s: committed %.1f MB, resident private %.1f MB, shared %.1f MB' % (t, *tot))
        for key, v in sorted(s.items(), key=lambda kv: -(kv[1][1] + kv[1][2]))[:18]:
            print('   %-40s committed %7.2f  private %7.2f  shared %7.2f' % (key[:40], v[0] / MB, v[1] / MB, v[2] / MB))


main()
