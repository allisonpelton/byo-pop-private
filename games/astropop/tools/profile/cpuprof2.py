# cpuprof2.py: games/bejeweled/tools/profile/cpuprof.py plus the callers of the samples that are not in the exe: for each
# such sample (a system call, the driver, the C runtime) the stack above ESP is scanned for return addresses into the exe
# that follow a call instruction, and the nearest three are the sample's caller chain (by caller in the report). The
# frames are a guess (stale words on the stack can pass as return addresses), so read the shares, not single lines.
# The original's header:
# A sampling CPU profile of a 32-bit game: every millisecond or so each thread that used CPU since the last look is
# suspended and its EIP recorded, weighted by the cycles it used; EIPs map to modules and, in the exe (or any module
# with a COFF symbol table), to functions via llvm-nm (LLVM_NM, else llvm-mingw's under E:/retools). Flat profile only
# (no stacks), and a WOW64 thread in a system call shows as ntdll.dll. Run under rundesk:
# python cpuprof.py <out.txt> <seconds> [env K=V ...] -- <exe> <args>
# Differences here: the 32-bit ntdll is read through SysWOW64, and the last line also gives the process's own user and kernel
# time. With <seconds> 0 there is no sampler (its suspends cost the game cycles of their own and charge a system call's cycles
# to the wait it returns from, so its shares cannot be trusted for a total): each thread's cycle count is read every quarter
# second and the four largest are printed beside the process's times. The comparison that counts is that one, with the builds
# alternating (the performance pass, GAME_NOTES.md).
import ctypes, ctypes.wintypes as W, bisect, os, subprocess, sys, time
from collections import Counter, defaultdict

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
EXE_NAME = os.environ.get('PROF_EXE', 'astropop.exe')
NM = os.environ.get('LLVM_NM', 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-nm.exe')


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


class WOW64_CONTEXT(ctypes.Structure):  # only the part up to Eip matters; the rest is padding
    _fields_ = [('ContextFlags', W.DWORD), ('pad', W.DWORD * 6), ('FloatSave', ctypes.c_byte * 112),
                ('SegGs', W.DWORD), ('SegFs', W.DWORD), ('SegEs', W.DWORD), ('SegDs', W.DWORD), ('Edi', W.DWORD),
                ('Esi', W.DWORD), ('Ebx', W.DWORD), ('Edx', W.DWORD), ('Ecx', W.DWORD), ('Eax', W.DWORD),
                ('Ebp', W.DWORD), ('Eip', W.DWORD), ('SegCs', W.DWORD), ('EFlags', W.DWORD), ('Esp', W.DWORD),
                ('SegSs', W.DWORD), ('rest', ctypes.c_byte * 600)]


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


def export_symbols(path):
    # a DLL without a symbol table: its exported names, so that a sample in ntdll.dll is at least named by the export before it
    import struct
    try:
        d = open(path, 'rb').read()
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        nsec = struct.unpack_from('<H', d, pe + 6)[0]
        osz = struct.unpack_from('<H', d, pe + 20)[0]
        opt = pe + 24
        pref = struct.unpack_from('<I', d, opt + 28)[0]
        erva = struct.unpack_from('<I', d, opt + 96)[0]
        secs = [struct.unpack_from('<IIII', d, opt + osz + 40 * i + 8) for i in range(nsec)]  # vsize, va, rawsize, rawptr

        def off(rva):
            for vs, va, rs, rp in secs:
                if va <= rva < va + max(vs, rs):
                    return rva - va + rp
            return None
        e = off(erva)
        nfun, nnames = struct.unpack_from('<II', d, e + 20)
        funcs, names, ords = (struct.unpack_from('<III', d, e + 28))
        fo, no, oo = off(funcs), off(names), off(ords)
        out = {}
        for i in range(nnames):
            nm = struct.unpack_from('<I', d, no + 4 * i)[0]
            o = struct.unpack_from('<H', d, oo + 2 * i)[0]
            rva = struct.unpack_from('<I', d, fo + 4 * o)[0]
            n0 = off(nm)
            name = d[n0:d.index(b'\0', n0)].decode('ascii', 'replace')
            if off(rva) is not None:
                out[pref + rva] = name
        keys = sorted(out)
        return (keys, [out[k] for k in keys]) if keys else None
    except Exception:
        return None


def preferred_base(path):
    import struct
    with open(path, 'rb') as f:
        d = f.read(4096)
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    return struct.unpack_from('<I', d, pe + 24 + 28)[0]  # OptionalHeader.ImageBase (PE32)


def read_mem(hp, addr, size):
    buf = ctypes.create_string_buffer(size)
    n = ctypes.c_size_t()
    if not k32.ReadProcessMemory(hp, ctypes.c_void_p(addr), buf, size, ctypes.byref(n)) or n.value == 0:
        return b''
    return buf.raw[:n.value]


def exe_frames(hp, esp, base, size, depth=3, scan=16384):
    # the nearest return addresses into the exe's image on the stack that follow a call instruction
    stack = read_mem(hp, esp, scan)
    out = []
    for i in range(0, len(stack) - 3, 4):
        d = int.from_bytes(stack[i:i + 4], 'little')
        if base + 16 <= d < base + size:
            b = read_mem(hp, d - 6, 6)
            if len(b) == 6 and (b[1] == 0xE8 or (b[4] == 0xFF and (b[5] & 0x38) == 0x10) or
                                (b[3] == 0xFF and (b[4] & 0x38) == 0x10) or (b[0] == 0xFF and (b[1] & 0x38) == 0x10)):
                if d not in out:
                    out.append(d)
                    if len(out) == depth:
                        break
    return tuple(out)


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
    callers = Counter()  # (eip, frames)
    exe = (0, 0)
    t0 = time.time()
    n = 0
    mods = []
    while p.poll() is None and time.time() - t0 < seconds:
        if n % 200 == 50:   # often enough that a run of under a second still has its modules named
            mods = modules(hp) or mods
            for m in mods:
                if m[2].lower().endswith(EXE_NAME):
                    exe = (m[0], m[1])
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
                if exe[1] and not (exe[0] <= ctx.Eip < exe[0] + exe[1]):
                    callers[(ctx.Eip, exe_frames(hp, ctx.Esp, exe[0], exe[1]))] += weight
            k32.ResumeThread(h)
        time.sleep(0.001)
    # seconds 0: no sampler, only each thread's own cycles, read every quarter second (reading perturbs nothing) and
    # printed with the process's times below
    thread_cycles = {}
    if seconds == 0:
        while p.poll() is None:
            for tid in threads(p.pid):
                if tid not in handles:
                    h = k32.OpenThread(0x0040 | 0x0800, False, tid)
                    if h:
                        handles[tid] = h
            for tid, h in handles.items():
                thread_cycles[tid] = max(thread_cycles.get(tid, 0), thread_cpu(h))
            time.sleep(0.25)
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
                # a 32-bit module's path is reported under System32; its file is the one under SysWOW64
                real = path.replace('SYSTEM32', 'SysWOW64').replace('System32', 'SysWOW64') if mods[j][0] < 2 ** 32 else path
                if not os.path.exists(real):
                    real = path
                syms[path] = (symbols(real) or export_symbols(real), preferred_base(real))
            st, pref = syms[path]
            if st:
                a = eip - mods[j][0] + pref
                k = bisect.bisect_right(st[0], a) - 1
                if k >= 0:
                    fn = '%s!%s' % (mod, st[1][k][:90])
        by_fn[fn] += cnt
        by_thread[tid][mod] += cnt
    by_caller = Counter()
    exe_path = next((m[2] for m in mods if m[2].lower().endswith(EXE_NAME)), None)
    if exe_path and exe[1]:
        if exe_path not in syms:
            syms[exe_path] = (symbols(exe_path), preferred_base(exe_path))
        st, pref = syms[exe_path]

        def fname(a):
            if not st:
                return '%x' % a
            k = bisect.bisect_right(st[0], a - exe[0] + pref) - 1
            return st[1][k][:60] if k >= 0 else '%x' % a
        for (eip, frames), cnt in callers.items():
            j = bisect.bisect_right(bases, eip) - 1
            mod = os.path.basename(mods[j][2]) if j >= 0 else '?'
            by_caller['%s <- %s' % (mod, ' < '.join(fname(a) for a in frames) if frames else 'no exe frame')] += cnt
    if os.environ.get('PROF_DEBUG'):
        for m in mods:
            if 'ntdll' in m[2].lower():
                print('module', hex(m[0]), hex(m[1]), m[2])
        for (tid, eip), cnt in samples.most_common(6):
            print('eip', hex(eip), cnt)
    with open(out, 'w') as f:
        f.write('%.2f G cycles sampled in %.1f s\n\nby module:\n' % (total / 1e9, time.time() - t0))
        for m, c in by_mod.most_common(25):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, m))
        f.write('\nby thread:\n')
        for tid, cs in sorted(by_thread.items(), key=lambda kv: -sum(kv[1].values())):
            s = sum(cs.values())
            f.write('  %5.1f%%  thread %d: %s\n' % (100.0 * s / total, tid, ', '.join('%s %d%%' % (m, 100 * c // s) for m, c in cs.most_common(4))))
        f.write('\nby caller (samples outside the exe: module <- nearest exe frames):\n')
        for fn, c in by_caller.most_common(45):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))
        f.write('\nby function:\n')
        for fn, c in by_fn.most_common(60):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))
    # the process's own processor time (what a player's machine spends): seconds is 0 for a run without the sampler, which
    # suspends every thread a millisecond and so costs the game cycles of its own
    ft = [W.FILETIME() for _ in range(4)]
    k32.GetProcessTimes(ctypes.c_void_p(hp), *[ctypes.byref(f) for f in ft])
    secs = lambda f: ((f.dwHighDateTime << 32) | f.dwLowDateTime) / 1e7
    if thread_cycles:
        top = sorted(thread_cycles.items(), key=lambda kv: -kv[1])[:4]
        print('threads (G cycles): ' + ', '.join('%d: %.2f' % (t, c / 1e9) for t, c in top))
    print('%.2f G cycles; exit %s; cpu user %.2f kernel %.2f s over %.1f s' % (total / 1e9, p.returncode, secs(ft[3]), secs(ft[2]), time.time() - t0))


main()
