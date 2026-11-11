# Where a game's processor time goes by thread, without stopping any thread (cpuprof.py suspends each thread to read its
# instruction pointer, and for a thread that mostly sleeps the suspend and resume are most of the cycles it then counts): the
# kernel and user time of every thread (GetThreadTimes) at the start and the end of a window, with each thread's start address
# named by its module. Run under rundesk from the worktree root:
#   python threadcpu.py <out.txt> <from s>,<to s> -- <exe> <args>
import ctypes, ctypes.wintypes as W, os, subprocess, sys, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
ntdll = ctypes.WinDLL('ntdll')
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenThread.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.restype = W.HANDLE


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


def threads(pid):
    snap = k32.CreateToolhelp32Snapshot(4, 0)
    e = THREADENTRY32()
    e.dwSize = ctypes.sizeof(e)
    out = []
    ok = k32.Thread32First(snap, ctypes.byref(e))
    while ok:
        if e.th32OwnerProcessID == pid:
            out.append(e.th32ThreadID)
        ok = k32.Thread32Next(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


def ft(t):
    return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7


def times(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    if not k32.GetThreadTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d)):
        return None
    return ft(c), ft(d)


def start_address(h):
    addr = ctypes.c_void_p()
    ntdll.NtQueryInformationThread(h, 9, ctypes.byref(addr), ctypes.sizeof(addr), None)    # ThreadQuerySetWin32StartAddress
    return addr.value or 0


def module_of(hp, addr):
    arr = (ctypes.c_void_p * 1024)()
    need = W.DWORD()
    psapi.EnumProcessModulesEx(hp, arr, ctypes.sizeof(arr), ctypes.byref(need), 3)
    best = ('?', 0)
    for i in range(need.value // ctypes.sizeof(ctypes.c_void_p)):
        base = arr[i] or 0
        if base <= addr and base >= best[1]:
            name = ctypes.create_unicode_buffer(520)
            psapi.GetModuleFileNameExW(hp, ctypes.c_void_p(arr[i]), name, 520)
            best = (os.path.basename(name.value), base)
    return best[0]


def main():
    out = sys.argv[1]
    w0, w1 = (float(x) for x in sys.argv[2].split(','))
    p = subprocess.Popen(sys.argv[4:], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    hp = int(p._handle)
    t0 = time.time()
    first, last, info = {}, {}, {}
    while p.poll() is None:
        t = time.time() - t0
        if t >= w0:
            for tid in threads(p.pid):
                h = k32.OpenThread(0x0040 | 0x0400, False, tid)    # query information, query limited information
                if not h:
                    continue
                tm = times(h)
                if tm:
                    if tid not in info:
                        info[tid] = (module_of(hp, start_address(h)), start_address(h))
                    if tid not in first:
                        first[tid] = (t, tm)
                    last[tid] = (t, tm)
                k32.CloseHandle(h)
            if t >= w1:
                break
        time.sleep(1.0)
    wall = max((v[0] for v in last.values()), default=0) - w0
    rows = []
    for tid, (t, (kt, ut)) in last.items():
        ft0, (k0, u0) = first[tid]
        rows.append((kt - k0 + ut - u0, kt - k0, ut - u0, tid))
    rows.sort(reverse=True)
    total = sum(r[0] for r in rows)
    with open(out, 'w') as f:
        f.write('window %.1f s, process threads %d, processor time %.2f s (%.1f%% of one core)\n' % (wall, len(rows), total, 100 * total / max(wall, 1e-9)))
        for cpu_t, kt, ut, tid in rows[:16]:
            f.write('  thread %6d  %6.2f s (kernel %.2f, user %.2f)  %5.1f%% of one core  started in %s +0x%x\n' % (
                tid, cpu_t, kt, ut, 100 * cpu_t / max(wall, 1e-9), info[tid][0], info[tid][1]))
    if p.poll() is None:
        p.terminate()
    print(open(out).read())


main()
