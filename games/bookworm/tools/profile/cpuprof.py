# A sampling CPU profile of a 32-bit game: every millisecond or so each thread that used CPU since the last look is
# suspended and its EIP recorded, weighted by the cycles it used; EIPs map to modules and, in the exe (or any module
# with a COFF symbol table), to functions via llvm-nm (LLVM_NM, else llvm-mingw's under E:/retools). Flat profile only
# (no stacks), and a WOW64 thread in a system call shows as ntdll.dll. Run under rundesk:
# python cpuprof.py <out.txt> <seconds> [env K=V ...] -- <exe> <args>
# (BookWorm's copy of games/bookworm/../bejeweled/tools/profile/cpuprof.py, with two changes: a module without a symbol table (the
# system's DLLs, taken from SysWOW64 for a 32-bit process) is named by its export table, so a system call shows as
# ntdll.dll!NtXxx, and the report lists 90 functions.)
import ctypes, ctypes.wintypes as W, bisect, os, subprocess, sys, time
from collections import Counter, defaultdict

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
NM = os.environ.get('LLVM_NM', 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-nm.exe')


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


class WOW64_CONTEXT(ctypes.Structure):  # only the part up to Eip matters; the rest is padding
    _fields_ = [('ContextFlags', W.DWORD), ('pad', W.DWORD * 6), ('FloatSave', ctypes.c_byte * 112),
                ('SegGs', W.DWORD), ('SegFs', W.DWORD), ('SegEs', W.DWORD), ('SegDs', W.DWORD), ('Edi', W.DWORD),
                ('Esi', W.DWORD), ('Ebx', W.DWORD), ('Edx', W.DWORD), ('Ecx', W.DWORD), ('Eax', W.DWORD),
                ('Ebp', W.DWORD), ('Eip', W.DWORD), ('SegCs', W.DWORD), ('EFlags', W.DWORD), ('Esp', W.DWORD),
                ('rest', ctypes.c_byte * 580)]


WOW64_CONTEXT_CONTROL = 0x10001
k32.OpenThread.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.restype = W.HANDLE


def threads(pid):
    snap = k32.CreateToolhelp32Snapshot(4, 0)
    e = THREADENTRY32(); e.dwSize = ctypes.sizeof(e)
    out = []
    ok = k32.Thread32First(snap, ctypes.byref(e))
    while ok:
        if e.th32OwnerProcessID == pid:
            out.append(e.th32ThreadID)
        ok = k32.Thread32Next(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


def thread_cpu(h):
    # cycles, exact up to the call (GetThreadTimes moves only in scheduler ticks)
    c = ctypes.c_uint64()
    k32.QueryThreadCycleTime(h, ctypes.byref(c))
    return c.value


def modules(hp):
    arr = (ctypes.c_void_p * 1024)()
    need = W.DWORD()
    psapi.EnumProcessModulesEx(hp, arr, ctypes.sizeof(arr), ctypes.byref(need), 3)
    out = []
    for i in range(need.value // ctypes.sizeof(ctypes.c_void_p)):
        name = ctypes.create_unicode_buffer(520)
        psapi.GetModuleFileNameExW(hp, ctypes.c_void_p(arr[i]), name, 520)

        class MI(ctypes.Structure):
            _fields_ = [('base', ctypes.c_void_p), ('size', W.DWORD), ('entry', ctypes.c_void_p)]
        mi = MI()
        psapi.GetModuleInformation(hp, ctypes.c_void_p(arr[i]), ctypes.byref(mi), ctypes.sizeof(mi))
        path = name.value
        if path.lower().startswith('c:\\windows\\system32\\'):    # a 32-bit process's system DLLs are the SysWOW64 ones
            wow = 'C:\\Windows\\SysWOW64\\' + path[len('c:\\windows\\system32\\'):]
            if os.path.exists(wow):
                path = wow
        out.append((mi.base or 0, mi.size, path))
    return sorted(out)


def symbols(path):
    try:
        r = subprocess.run([NM, '-C', '--defined-only', '-n', path], capture_output=True, text=True, timeout=120)
    except Exception:
        return None
    addrs, names = [], []
    for l in r.stdout.splitlines():
        p = l.split(' ', 2)
        if len(p) == 3 and p[1] in 'tTwW':
            addrs.append(int(p[0], 16)); names.append(p[2])
    return (addrs, names) if addrs else None


def exports(path):
    """(addresses, names) of a PE32 file's export table, by address, for the modules that have no symbol table."""
    import struct
    try:
        d = open(path, 'rb').read()
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        nsec = struct.unpack_from('<H', d, pe + 6)[0]
        opt = struct.unpack_from('<H', d, pe + 20)[0]
        secs = pe + 24 + opt
        sections = [struct.unpack_from('<IIII', d, secs + 40 * i + 8) for i in range(nsec)]  # vsize, va, rawsize, rawptr

        def off(rva):
            for vs, va, rs, rp in sections:
                if va <= rva < va + max(vs, rs):
                    return rva - va + rp
            return None
        exp_rva = struct.unpack_from('<I', d, pe + 24 + 96)[0]
        o = off(exp_rva)
        nfun, nnames, faddr, naddr, oaddr = struct.unpack_from('<IIIII', d, o + 20)
        fa, na, oa = off(faddr), off(naddr), off(oaddr)
        addrs, names = [], []
        for i in range(nnames):
            nm = struct.unpack_from('<I', d, na + 4 * i)[0]
            idx = struct.unpack_from('<H', d, oa + 2 * i)[0]
            rva = struct.unpack_from('<I', d, fa + 4 * idx)[0]
            s = off(nm)
            e = d.index(b'\0', s)
            addrs.append(rva); names.append(d[s:e].decode('ascii', 'replace'))
        order = sorted(range(len(addrs)), key=lambda i: addrs[i])
        base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
        return ([addrs[i] + base for i in order], [names[i] for i in order])
    except Exception:
        return None


def preferred_base(path):
    import struct
    with open(path, 'rb') as f:
        d = f.read(4096)
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    return struct.unpack_from('<I', d, pe + 24 + 28)[0]  # OptionalHeader.ImageBase (PE32)


def main():
    out, seconds = sys.argv[1], float(sys.argv[2])
    i = 3
    env = dict(os.environ)
    while sys.argv[i] != '--':
        kk, vv = sys.argv[i].split('=', 1); env[kk] = vv; i += 1
    cmd = sys.argv[i + 1:]
    p = subprocess.Popen(cmd, cwd=os.path.dirname(os.path.abspath(out)), env=env, stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
    hp = int(p._handle)
    handles, last = {}, {}
    samples = Counter()  # (tid, eip)
    t0 = time.time()
    n = 0
    mods = []
    while p.poll() is None and time.time() - t0 < seconds:
        if n % 200 == 50:   # often enough that a run of under a second still has its modules named
            mods = modules(hp) or mods
        if n % 200 == 0:
            for tid in threads(p.pid):
                if tid not in handles:
                    h = k32.OpenThread(0x0002 | 0x0008 | 0x0040 | 0x0800, False, tid)  # suspend, get context, query
                    if h:
                        handles[tid] = h; last[tid] = thread_cpu(h)
        n += 1
        for tid, h in list(handles.items()):
            c = thread_cpu(h)
            weight = c - last[tid]
            if weight == 0:
                continue
            last[tid] = c
            if k32.Wow64SuspendThread(h) == 0xFFFFFFFF:
                continue
            ctx = WOW64_CONTEXT(); ctx.ContextFlags = WOW64_CONTEXT_CONTROL
            if k32.Wow64GetThreadContext(h, ctypes.byref(ctx)):
                samples[(tid, ctx.Eip)] += weight
            k32.ResumeThread(h)
        time.sleep(0.001)
    if p.poll() is None:
        p.wait(timeout=120)
    bases = [m[0] for m in mods]
    syms = {}
    by_mod, by_fn, by_thread = Counter(), Counter(), defaultdict(Counter)
    total = sum(samples.values())
    for (tid, eip), cnt in samples.items():
        j = bisect.bisect_right(bases, eip) - 1
        mod = os.path.basename(mods[j][2]) if j >= 0 and eip < mods[j][0] + mods[j][1] else '?'
        by_mod[mod] += cnt
        fn = mod
        if j >= 0 and mod != '?':
            path = mods[j][2]
            if path not in syms:
                try:
                    syms[path] = (symbols(path) or exports(path), preferred_base(path))
                except OSError:     # a module whose file is not at the path the process reported
                    syms[path] = (None, 0)
            st, pref = syms[path]
            if st:
                a = eip - mods[j][0] + pref
                k = bisect.bisect_right(st[0], a) - 1
                if k >= 0:
                    fn = '%s!%s' % (mod, st[1][k][:90])
        by_fn[fn] += cnt
        by_thread[tid][mod] += cnt
    with open(out, 'w') as f:
        f.write('%.2f G cycles sampled in %.1f s\n\nby module:\n' % (total / 1e9, time.time() - t0))
        for m, c in by_mod.most_common(25):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, m))
        f.write('\nby thread:\n')
        for tid, cs in sorted(by_thread.items(), key=lambda kv: -sum(kv[1].values())):
            s = sum(cs.values())
            f.write('  %5.1f%%  thread %d: %s\n' % (100.0 * s / total, tid, ', '.join('%s %d%%' % (m, 100 * c // s) for m, c in cs.most_common(4))))
        f.write('\nby function:\n')
        for fn, c in by_fn.most_common(90):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))
    print('%.2f G cycles; exit %s' % (total / 1e9, p.returncode))


main()
