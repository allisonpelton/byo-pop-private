# Word Harmony's copy of games/bejeweled/tools/profile/cpuprof.py with a stack scan: a sample whose EIP is outside the exe (a WOW64
# system call, a driver) is also charged to the innermost return address in the exe's code found on that thread's stack, so the
# profile says which exe function the time outside it was spent for ("by exe caller" section).
# A sampling CPU profile of a 32-bit game: every millisecond or so each thread that used CPU since the last look is
# suspended and its EIP recorded, one count for each time it was found running (see the loop); EIPs map to modules and, in the exe (or any module
# with a COFF symbol table), to functions via llvm-nm (LLVM_NM, else llvm-mingw's under E:/retools). Flat profile only
# (no stacks), and a WOW64 thread in a system call shows as ntdll.dll. Run under rundesk:
# python cpuprof.py <out.txt> <seconds> [env K=V ...] -- <exe> <args>
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
                ('Ebp', W.DWORD), ('Eip', W.DWORD), ('rest', ctypes.c_byte * 600)]


WOW64_CONTEXT_CONTROL = 0x10001
rpm = k32.ReadProcessMemory
rpm.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]


def is_call_return(hp, w):
    # whether the word is the address after a call instruction (E8 rel32, or FF /2 in its forms)
    pre = ctypes.create_string_buffer(8)
    got = ctypes.c_size_t()
    if not rpm(hp, w - 8, pre, 8, ctypes.byref(got)) or got.value != 8:
        return False
    b = pre.raw
    return b[3] == 0xE8 or any(b[i] == 0xFF and (b[i + 1] >> 3) & 7 == 2 for i in (2, 4, 5, 6))


def caller_in(hp, esp, lo, hi, depth=4):
    """The exe's frames on a thread's stack, innermost first (up to depth return addresses), for an exe built with frame
    pointers (-fno-omit-frame-pointer): a saved EBP is a word on the stack that points up the stack at a pair (the next
    saved EBP, a return address in the exe's code after a call), and the chain goes on at least two frames. The first such
    chain from the stack pointer upward is the innermost exe frame that is live; stale words below it are not taken."""
    buf = ctypes.create_string_buffer(16384)
    got = ctypes.c_size_t()
    n = 0
    for size in (16384, 4096, 1024, 256):   # a read that crosses the top of the stack fails as a whole: try shorter ones
        if rpm(hp, esp, buf, size, ctypes.byref(got)):
            n = got.value // 4
            break
    words = ctypes.cast(buf, ctypes.POINTER(ctypes.c_uint32))
    end = esp + n * 4

    def pair(addr):  # (saved ebp, return address) at addr, from the buffer
        k = (addr - esp) // 4
        if addr % 4 or addr < esp or k + 1 >= n:
            return None
        return words[k], words[k + 1]

    for k in range(n):
        addr = esp + 4 * k
        first = pair(addr)
        if first is None or not (lo <= first[1] < hi) or first[0] <= addr or first[0] >= end:
            continue
        frames, cur, ok = [], addr, True
        for _ in range(depth):
            pr = pair(cur)
            if pr is None or not (lo <= pr[1] < hi):
                break
            frames.append(pr[1])
            if pr[0] <= cur:
                break
            cur = pr[0]
        if len(frames) >= 2 and all(is_call_return(hp, w) for w in frames[:2]):
            return tuple(frames)
    return ()


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
    callers = Counter()  # (tid, the exe's frames) for the samples outside the exe
    exe_lo = exe_hi = 0
    t0 = time.time()
    n = 0
    mods = []
    while p.poll() is None and time.time() - t0 < seconds:
        if n % 200 == 50:   # often enough that a run of under a second still has its modules named
            mods = modules(hp) or mods
            for m in mods:
                if m[2].lower().endswith(os.path.basename(cmd[0]).lower()):
                    exe_lo, exe_hi = m[0], m[0] + m[1]
        if n % 200 == 0:
            for tid in threads(p.pid):
                if tid not in handles:
                    h = k32.OpenThread(0x0002 | 0x0008 | 0x0040 | 0x0800, False, tid)  # suspend, get context, query
                    if h:
                        handles[tid] = h; last[tid] = thread_cpu(h)
        n += 1
        for tid, h in list(handles.items()):
            # a thread is sampled only while it is running: its cycle count moves between two reads a moment apart (a
            # thread that waits does not), and each such look counts one. Weighting by the cycles a thread used since the
            # last look instead puts the work of a burst on whatever the thread waits in when the sampler gets to it
            c = thread_cpu(h)
            weight = 1
            if thread_cpu(h) - c < 200:
                continue
            if k32.Wow64SuspendThread(h) == 0xFFFFFFFF:
                continue
            ctx = WOW64_CONTEXT(); ctx.ContextFlags = WOW64_CONTEXT_CONTROL
            if k32.Wow64GetThreadContext(h, ctypes.byref(ctx)):
                samples[(tid, ctx.Eip)] += weight
                if exe_hi and not exe_lo <= ctx.Eip < exe_hi:
                    esp = int.from_bytes(ctypes.string_at(ctypes.addressof(ctx.rest) + 8, 4), 'little')
                    callers[(tid, caller_in(hp, esp, exe_lo, exe_hi))] += weight
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
        f.write('%d samples of running threads in %.1f s\n\nby module:\n' % (total, time.time() - t0))
        for m, c in by_mod.most_common(25):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, m))
        f.write('\nby thread:\n')
        for tid, cs in sorted(by_thread.items(), key=lambda kv: -sum(kv[1].values())):
            s = sum(cs.values())
            f.write('  %5.1f%%  thread %d: %s\n' % (100.0 * s / total, tid, ', '.join('%s %d%%' % (m, 100 * c // s) for m, c in cs.most_common(4))))
        f.write('\nby function:\n')
        for fn, c in by_fn.most_common(60):
            f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))
    exe_mod = [m for m in mods if m[2].lower().endswith(os.path.basename(cmd[0]).lower())]
    if exe_mod:
        path = exe_mod[0][2]
        if path not in syms:
            syms[path] = (symbols(path), preferred_base(path))
        st, pref = syms[path]
        def fname(w):
            k = bisect.bisect_right(st[0], w - exe_mod[0][0] + pref) - 1
            return (st[1][k][:70] if k >= 0 else '?')

        by_chain, by_inner = Counter(), Counter()
        for (tid, frames), cnt in callers.items():
            if frames and st:
                names = [fname(w) for w in frames[:3]]
                by_chain[(tid, ' <- '.join(names))] += cnt
                by_inner[(tid, names[0])] += cnt
            else:
                by_chain[(tid, '(no exe frame found)')] += cnt
                by_inner[(tid, '(no exe frame found)')] += cnt
        # the hottest addresses inside the exe (function + offset), to read against llvm-objdump's listing of the exe
        by_addr = Counter()
        for (tid, eip), cnt in samples.items():
            if exe_mod[0][0] <= eip < exe_mod[0][0] + exe_mod[0][1]:
                a = eip - exe_mod[0][0] + pref
                k = bisect.bisect_right(st[0], a) - 1
                by_addr[(st[1][k][:60] if k >= 0 else '?', a - (st[0][k] if k >= 0 else 0), a)] += cnt
        with open(out, 'a') as f:
            f.write('\nhottest addresses in the exe (function+offset, address):\n')
            for (name, off, a), c in by_addr.most_common(40):
                f.write('  %5.1f%%  %s+0x%x  0x%x\n' % (100.0 * c / total, name, off, a))
        with open(out, 'a') as f:
            f.write('\nby innermost exe frame (samples outside the exe, charged to the exe function that called out):\n')
            for (tid, name), c in by_inner.most_common(30):
                f.write('  %5.1f%%  thread %d  %s\n' % (100.0 * c / total, tid, name))
            f.write('\nby exe frames, three deep (innermost first):\n')
            for (tid, name), c in by_chain.most_common(40):
                f.write('  %5.1f%%  thread %d  %s\n' % (100.0 * c / total, tid, name))
    print('%d samples; exit %s' % (total, p.returncode))


main()
