# A sampling CPU profile of a 32-bit game: every millisecond or so each thread that used CPU since the last look is
# suspended and its EIP recorded, weighted by the cycles it used; EIPs map to modules and, in the exe (or any module
# with a COFF symbol table), to functions via llvm-nm (LLVM_NM, else llvm-mingw's under E:/retools). Flat profile only
# (no stacks), and a WOW64 thread in a system call shows as ntdll.dll. Run under rundesk:
# python cpuprof.py <out.txt> <seconds> [env K=V ...] -- <exe> <args>   (paths absolute: the game runs in <out.txt>'s folder)
# This copy (Alchemy's) also names, for a sample outside the exe, the first return address on the thread's stack that lies in the
# exe (the framework or game code that made the call) and in another module that is not a system one (SDL2.dll, the D3D runtime):
# a stack scan that keeps only words after a call instruction, so it can name a stale frame; function names come from the exe's
# symbol table and, for a dll, from its export table (the nearest export below the address, so only a hint).
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
                ('Ebp', W.DWORD), ('Eip', W.DWORD), ('rest', ctypes.c_ubyte * 600)]


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
        out.append((mi.base or 0, mi.size, name.value))
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


def preferred_base(path):
    import struct
    with open(path, 'rb') as f:
        d = f.read(4096)
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    return struct.unpack_from('<I', d, pe + 24 + 28)[0]  # OptionalHeader.ImageBase (PE32)


_bases = [None, None]   # (the mods list the bases were made from, the bases)


def module_of(mods, a):
    if _bases[0] is not mods:
        _bases[0], _bases[1] = mods, [m[0] for m in mods]
    bases = _bases[1]
    j = bisect.bisect_right(bases, a) - 1
    return (j, mods[j]) if j >= 0 and a < mods[j][0] + mods[j][1] else (-1, None)


def is_return_address(hp, w):
    # the bytes before w end a call: E8 rel32, FF 15 [abs], FF /2 with a register or a memory operand
    b = (ctypes.c_ubyte * 8)()
    got = ctypes.c_size_t(0)
    if not k32.ReadProcessMemory(hp, ctypes.c_void_p(w - 8), b, 8, ctypes.byref(got)) or got.value != 8:
        return False
    x = bytes(b)
    return (x[3] == 0xE8 or                                                          # call rel32
            (x[2] == 0xFF and (x[3] == 0x15 or 0x90 <= x[3] <= 0x97)) or             # call [abs32], call [reg+disp32]
            (x[6] == 0xFF and (0xD0 <= x[7] <= 0xD7 or 0x10 <= x[7] <= 0x17)) or     # call reg, call [reg]
            (x[5] == 0xFF and (0x50 <= x[6] <= 0x57 or x[6] == 0x14)) or             # call [reg+disp8], call [sib]
            (x[4] == 0xFF and x[5] == 0x54))                                         # call [sib+disp8]


SYSTEM = ('ntdll.dll', 'wow64.dll', 'wow64cpu.dll', 'wow64win.dll', 'win32u.dll')


STACK_BYTES = 32768   # the driver's present path has frames of kilobytes, so the game's own frame can lie that deep


def read_stack(hp, esp):
    # the top STACK_BYTES of a suspended thread's stack, a page at a time up to the first unreadable one (the thread is stopped
    # only for these reads; the scan of the words is done after it runs again)
    out = b''
    a = esp
    while len(out) < STACK_BYTES:
        n = 4096 - (a & 4095)
        buf = (ctypes.c_ubyte * n)()
        got = ctypes.c_size_t(0)
        if not k32.ReadProcessMemory(hp, ctypes.c_void_p(a), buf, n, ctypes.byref(got)) or got.value != n:
            break
        out += bytes(buf)
        a += n
    return out


def stack_callers(hp, mods, stack, eip):
    # (the first return address inside the exe, the first inside another module that is not a system one); (None, None) if EIP
    # is in the exe itself
    if not mods:
        return (None, None)
    exe = mods[0][0]   # mods is sorted by base and the exe loads lowest
    j, m = module_of(mods, eip)
    if m is None or m[0] == exe:
        return (None, None)
    if len(stack) < 8:
        return (None, None)
    first_exe = first_other = None
    for i in range(0, len(stack) - 3, 4):
        w = int.from_bytes(stack[i:i + 4], 'little')
        jj, mm = module_of(mods, w)
        if mm is None or w == eip:
            continue
        if os.path.basename(mm[2]).lower() in SYSTEM:
            continue
        if not is_return_address(hp, w):
            continue
        if mm[0] == exe:
            first_exe = w
            break
        if first_other is None:
            first_other = w
    return (first_exe, first_other)


def exports(path):
    # (sorted rvas, names) of a PE32 dll's export table, for the dlls with no COFF symbols
    import struct
    try:
        d = open(path, 'rb').read()
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        magic = struct.unpack_from('<H', d, pe + 24)[0]
        dd = pe + 24 + (96 if magic == 0x10b else 112)
        rva, size = struct.unpack_from('<II', d, dd)
        nsec = struct.unpack_from('<H', d, pe + 6)[0]
        sec = pe + 24 + struct.unpack_from('<H', d, pe + 20)[0]
        secs = [struct.unpack_from('<IIII', d, sec + 40 * k + 8) for k in range(nsec)]   # vsize, vaddr, rawsize, rawptr

        def off(r):
            for vs, va, rs, rp in secs:
                if va <= r < va + max(vs, rs):
                    return r - va + rp
            return None
        e = off(rva)
        nnames = struct.unpack_from('<I', d, e + 24)[0]
        aof, aon, aoo = struct.unpack_from('<III', d, e + 28)
        funcs, names, ords = off(aof), off(aon), off(aoo)
        out = []
        for k in range(nnames):
            nr = struct.unpack_from('<I', d, names + 4 * k)[0]
            o = struct.unpack_from('<H', d, ords + 2 * k)[0]
            fr = struct.unpack_from('<I', d, funcs + 4 * o)[0]
            out.append((fr, d[off(nr):d.index(b'\0', off(nr))].decode()))
        out.sort()
        return ([a for a, _ in out], [n for _, n in out])
    except Exception:
        return None


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
    callers = Counter()  # (tid, eip, (first return address in the exe, first in another non-system module))
    t0 = time.time()
    n = 0
    mods = []
    while p.poll() is None and time.time() - t0 < seconds:
        if n % 2000 == 500:
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
            stack = b''
            try:   # whatever happens, the thread is resumed (an exception here once left the game stopped for good)
                ok = k32.Wow64GetThreadContext(h, ctypes.byref(ctx))
                if ok:
                    stack = read_stack(hp, int.from_bytes(bytes(ctx.rest[8:12]), 'little'))
            finally:
                k32.ResumeThread(h)
            if ok:
                samples[(tid, ctx.Eip)] += weight
                callers[(tid, ctx.Eip, stack_callers(hp, mods, stack, ctx.Eip))] += weight
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
                syms[path] = (symbols(path), preferred_base(path))
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
        for fn, c in by_fn.most_common(60):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))

        exp = {}

        def name_of(a):
            if a is None:
                return None
            j, m = module_of(mods, a)
            if m is None:
                return '?'
            path = m[2]
            mod = os.path.basename(path)
            if path not in syms:
                syms[path] = (symbols(path), preferred_base(path))
            st, pref = syms[path]
            if st:
                k = bisect.bisect_right(st[0], a - m[0] + pref) - 1
                if k >= 0:
                    return '%s!%s' % (mod, st[1][k][:90])
            if path not in exp:
                exp[path] = exports(path)
            ex = exp[path]
            if ex:
                k = bisect.bisect_right(ex[0], a - m[0]) - 1
                if k >= 0:
                    return '%s!%s+0x%x' % (mod, ex[1][k], a - m[0] - ex[0][k])
            return '%s+0x%x' % (mod, a - m[0])
        by_exe, by_pair, by_eip = Counter(), Counter(), Counter()
        outside = 0
        for (tid, eip, fr), cnt in callers.items():
            if fr == (None, None) and name_of(eip).startswith('alchemy.exe'):
                continue
            by_eip[name_of(eip)] += cnt
        for (tid, eip, (fe, fo)), cnt in callers.items():
            if fe is None and fo is None:
                continue
            outside += cnt
            by_exe[name_of(fe) or '(no frame of the exe found)'] += cnt
            by_pair[(name_of(fe) or '-') + '  <-  ' + (name_of(fo) or '-')] += cnt
        f.write('\nsamples outside the exe, by the first frame of the exe on the stack (%.1f%% of all):\n' % (100.0 * outside / total))
        for fn, c in by_exe.most_common(40):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))
        f.write('\nsamples outside the exe, by the instruction (a system module: the nearest export below it, so only a hint):\n')
        for fn, c in by_eip.most_common(25):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))
        f.write('\nthe busiest threads by where they started (NtQueryInformationThread, Win32StartAddress):\n')
        ntq = ctypes.WinDLL('ntdll').NtQueryInformationThread
        for tid, cs in sorted(by_thread.items(), key=lambda kv: -sum(kv[1].values()))[:8]:
            a = ctypes.c_void_p(0)
            ntq(handles[tid], 9, ctypes.byref(a), ctypes.sizeof(a), None)
            f.write('  %5.1f%%  thread %d starts at %s\n' % (100.0 * sum(cs.values()) / total, tid, name_of(a.value)))
        f.write('\nthe same with the first frame of another module (the library the exe called):\n')
        for fn, c in by_pair.most_common(40):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))
    print('%.2f G cycles; exit %s' % (total / 1e9, p.returncode))


main()
