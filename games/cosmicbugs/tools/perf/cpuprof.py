# A sampling CPU profile of a 32-bit game: every millisecond or so each thread that used CPU since the last look is
# suspended and its EIP recorded, weighted by the cycles it used; EIPs map to modules and, in the exe (or any module
# with a COFF symbol table), to functions via llvm-nm (LLVM_NM, else llvm-mingw's under E:/retools). Flat profile only
# (no stacks), and a WOW64 thread in a system call shows as ntdll.dll. Run under rundesk:
# python cpuprof.py <out.txt> <seconds> [env K=V ...] -- <exe> <args>
# Cosmic Bugs' copy adds CALLERS=1: the hottest raw addresses, and for each sample outside the exe the exe function the
# nearest return address in its code on the 32-bit stack points into (a scan, so a stale value can mislead now and then) and
# the DLL the exe called into on the way (d3d11.dll, nvwgf2um.dll...). Modules without a symbol table (ntdll) are named by their
# nearest lower export, which is only a hint: read the address with llvm-objdump. SPIN=1 and MINW look at the threads without
# sleeping, so that cycles are charged to where a thread is within about 10 us of using them.
# What it cannot do: a thread that works in short bursts and then waits shows at its wait (here ZwWaitForSingleObject's return,
# 54% of the cycles of the Cosmic Bugs port), whatever MINW is, so for where a frame's cycles go time the calls themselves with
# QueryThreadCycleTime before and after (README.md, "The performance pass").
import ctypes, ctypes.wintypes as W, bisect, os, struct, subprocess, sys, time
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
                ('SegSs', W.DWORD), ('rest', ctypes.c_byte * 580)]


WOW64_CONTEXT_CONTROL = 0x10001
CALLERS = os.environ.get('CALLERS') == '1'
# SPIN=1: look at the threads without sleeping (this process takes a core), and only sample a thread once it has used
# MINW cycles since the last sample (default 30000, about 10 us): a game that works in short bursts and then waits is
# otherwise charged to its wait, where the thread is when a 1 ms look arrives
SPIN = os.environ.get('SPIN') == '1'
MINW = int(os.environ.get('MINW', '30000' if SPIN else '0'))
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]


def pe_exports(path):
    # (rva, name) of a PE file's named exports, sorted
    d = open(path, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    magic = struct.unpack_from('<H', d, pe + 24)[0]
    dd = pe + 24 + (112 if magic == 0x20b else 96)
    nsec, optsz = struct.unpack_from('<H', d, pe + 6)[0], struct.unpack_from('<H', d, pe + 20)[0]
    secs = [struct.unpack_from('<IIII', d, pe + 24 + optsz + 40 * i + 8) for i in range(nsec)]

    def off(rva):
        for vs, va, rs, rp in secs:
            if va <= rva < va + max(vs, rs):
                return rva - va + rp
        return None
    exp_rva = struct.unpack_from('<I', d, dd)[0]
    if not exp_rva:
        return None
    e = off(exp_rva)
    (_c, _t, _a, _b, _n, _base, nfun, nnames, afun, anames, aord) = struct.unpack_from('<IIHHIIIIIII', d, e)
    out = []
    for i in range(nnames):
        n = off(struct.unpack_from('<I', d, off(anames) + 4 * i)[0])
        name = d[n:d.index(b'\0', n)].decode('ascii', 'replace')
        o = struct.unpack_from('<H', d, off(aord) + 2 * i)[0]
        out.append((struct.unpack_from('<I', d, off(afun) + 4 * o)[0], name))
    out.sort()
    return out


def stub_numbers(path):
    # {service number: name} of a 32-bit system-call stub module: each Nt/Zw export starts "mov eax, N" (B8 imm32)
    d = open(path, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec, optsz = struct.unpack_from('<H', d, pe + 6)[0], struct.unpack_from('<H', d, pe + 20)[0]
    secs = [struct.unpack_from('<IIII', d, pe + 24 + optsz + 40 * i + 8) for i in range(nsec)]
    out = {}
    for rva, name in pe_exports(path) or []:
        if not name.startswith(('Nt', 'Zw')):
            continue
        for vs, va, rs, rp in secs:
            if va <= rva < va + max(vs, rs):
                o = rva - va + rp
                if d[o] == 0xB8:
                    n = struct.unpack_from('<I', d, o + 1)[0]
                    if n not in out or name.startswith('Nt'):
                        out[n] = name
                break
    return out


def text_range(path):
    # (rva, size) of the exe's .text section
    d = open(path, 'rb').read(4096)
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec, optsz = struct.unpack_from('<H', d, pe + 6)[0], struct.unpack_from('<H', d, pe + 20)[0]
    for i in range(nsec):
        o = pe + 24 + optsz + 40 * i
        if d[o:o + 8].rstrip(bytes(1)) == b'.text':
            return struct.unpack_from('<II', d, o + 12)[0], struct.unpack_from('<I', d, o + 8)[0]
    return 0, 0


def is_return(hp, v):
    # a return address follows a call instruction: E8 rel32, FF /2 (call reg, [reg], [mem], [reg+disp])
    buf = ctypes.create_string_buffer(8)
    got = ctypes.c_size_t()
    if not k32.ReadProcessMemory(hp, ctypes.c_void_p(v - 8), buf, 8, ctypes.byref(got)):
        return False
    b = buf.raw
    return b[3] == 0xE8 or (b[2] == 0xFF and (b[3] & 0x38) == 0x10) or (b[1] == 0xFF and (b[2] & 0x38) == 0x10) or         (b[0] == 0xFF and (b[1] & 0x38) == 0x10) or (b[5] == 0xFF and (b[6] & 0x38) == 0x10) or (b[6] == 0xFF and (b[7] & 0x38) == 0x10)


def read_stack(hp, esp, count=192):
    buf = (ctypes.c_uint32 * count)()
    got = ctypes.c_size_t()
    k32.ReadProcessMemory(hp, ctypes.c_void_p(esp), buf, count * 4, ctypes.byref(got))
    return list(buf)[:got.value // 4]
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
    callers, sysnames = Counter(), Counter()  # (exe return address, via) and the native RIP, cycles
    text = None
    eax_by_addr = Counter()  # (eip, eax) outside the exe, cycles: eax is the service number while a thread is in a system call
    t0 = time.time()
    n = 0
    mods = []
    mods32, bases32 = [], []
    while p.poll() is None and time.time() - t0 < seconds:
        if n % 200 == 50:   # often enough that a run of under a second still has its modules named
            mods = modules(hp) or mods
            mods32 = [m for m in mods if m[0] < 0x100000000]
            bases32 = [m[0] for m in mods32]
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
            if weight == 0 or weight < MINW:
                continue
            last[tid] = c
            if k32.Wow64SuspendThread(h) == 0xFFFFFFFF:
                continue
            ctx = WOW64_CONTEXT(); ctx.ContextFlags = WOW64_CONTEXT_CONTROL | (2 if CALLERS else 0)  # CALLERS: Eax too
            if k32.Wow64GetThreadContext(h, ctypes.byref(ctx)):
                samples[(tid, ctx.Eip)] += weight
                if CALLERS:
                    exe = [m for m in mods if m[2].lower().endswith('.exe')]
                    if exe and not (exe[0][0] <= ctx.Eip < exe[0][0] + exe[0][1]):
                        if not text:
                            tr = text_range(exe[0][2])
                            text = (exe[0][0] + tr[0], exe[0][0] + tr[0] + tr[1])
                        eax_by_addr[(ctx.Eip, ctx.Eax)] += weight
                        # the nearest return address into the exe's code, and the first one into another module (not the system's
                        # base libraries) on the way: the DLL the exe called into
                        via, found, checked = '', 0, 0
                        for v in read_stack(hp, ctx.Esp, 1024):
                            if checked >= 40:
                                break
                            inexe = text[0] <= v < text[1]
                            j = bisect.bisect_right(bases32, v) - 1
                            m = os.path.basename(mods32[j][2]).lower() if j >= 0 and v < mods32[j][0] + mods32[j][1] else ''
                            if not inexe and (not m or m in ('ntdll.dll', 'kernelbase.dll', 'kernel32.dll', 'wow64cpu.dll')):
                                continue
                            checked += 1
                            if not is_return(hp, v):
                                continue
                            if inexe:
                                found = v
                                break
                            if not via:
                                via = m
                        callers[(found, via, ctx.Eip)] += weight
            k32.ResumeThread(h)
        time.sleep(0 if SPIN else 0.001)
    if p.poll() is None:
        p.wait(timeout=120)
    bases = [m[0] for m in mods]
    syms, fallback = {}, {}
    by_mod, by_fn, by_thread = Counter(), Counter(), defaultdict(Counter)
    by_addr = Counter()  # (thread, module, rva)
    total = sum(samples.values())
    for (tid, eip), cnt in samples.items():
        j = bisect.bisect_right(bases, eip) - 1
        mod = os.path.basename(mods[j][2]) if j >= 0 and eip < mods[j][0] + mods[j][1] else '?'
        by_mod[mod] += cnt
        by_addr[(tid, mod, eip - mods[j][0] if j >= 0 else eip)] += cnt
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
            elif CALLERS:
                if path not in fallback:
                    try:
                        ex = pe_exports(path)
                    except Exception:
                        ex = None
                    fallback[path] = ([e[0] for e in ex], ex) if ex else None
                if fallback[path]:
                    k = bisect.bisect_right(fallback[path][0], eip - mods[j][0]) - 1
                    if k >= 0:
                        fn = '%s!%s' % (mod, fallback[path][1][k][1])
        by_fn[fn] += cnt
        by_thread[tid][mod] += cnt
    caller_fn, caller_mod, sysnames = Counter(), Counter(), Counter()
    if CALLERS:
        exe = [m for m in mods if m[2].lower().endswith('.exe')]
        if exe:
            st, pref = syms.get(exe[0][2]) or (symbols(exe[0][2]), preferred_base(exe[0][2]))
            for (ret, via, eip), cnt in callers.items():
                where = '(no exe address on the stack)'
                if ret and st:
                    k = bisect.bisect_right(st[0], ret - exe[0][0] + pref) - 1
                    where = st[1][k][:90] if k >= 0 else '?'
                j = bisect.bisect_right(bases, eip) - 1
                inmod = os.path.basename(mods[j][2]) if j >= 0 else '?'
                caller_fn[where + (' via ' + via if via else '')] += cnt
                caller_mod['%s <- %s%s' % (inmod, where, ' via ' + via if via else '')] += cnt
        # the 32-bit system-call stubs' service numbers: a stub is "mov eax, N" at its export
        numbers = {}
        for m in mods32:
            if os.path.basename(m[2]).lower() in ('ntdll.dll', 'win32u.dll'):
                try:
                    numbers.update(stub_numbers(m[2]))
                except Exception:
                    pass
        for (eip, eax), cnt in eax_by_addr.items():
            j = bisect.bisect_right(bases, eip) - 1
            inmod = os.path.basename(mods[j][2]).lower() if j >= 0 and eip < mods[j][0] + mods[j][1] else '?'
            if inmod in ('ntdll.dll', 'win32u.dll') and eax in numbers:
                sysnames['%s (service %#x)' % (numbers[eax], eax)] += cnt
            else:
                sysnames['(not in a stub of a known number: %s)' % inmod] += cnt
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
        if CALLERS:
            f.write('\nhottest addresses (thread, module, rva):\n')
            for (tid, m, rva), c in by_addr.most_common(25):
                f.write('  %5.1f%%  %d %s +0x%x\n' % (100.0 * c / total, tid, m, rva))
            f.write('\nsamples outside the exe, by the exe function whose call is the nearest return address on the stack:\n')
            for fn, c in caller_fn.most_common(25):
                f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))
            f.write('\nthe same, with the module the sample was in:\n')
            for fn, c in caller_mod.most_common(30):
                f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))
            f.write('\nsystem calls (the service number in eax, named by the 32-bit stubs), cycles outside the exe:\n')
            for fn, c in sysnames.most_common(30):
                f.write('  %5.1f%%  %s\n' % (100.0 * c / total, fn))
    print('%.2f G cycles; exit %s' % (total / 1e9, p.returncode))


main()
