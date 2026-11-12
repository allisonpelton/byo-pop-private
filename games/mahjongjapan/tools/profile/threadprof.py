# Where a game's processor time goes by thread, for the port and for an original alike, without stopping any thread: for
# every thread of the process, the cycles it used (QueryThreadCycleTime, exact) and its kernel and user time
# (GetThreadTimes) between two moments of the process's life, with the thread's name (GetThreadDescription) and the
# module its start address lies in. The process's own kernel and user time (GetProcessTimes) is printed beside the
# sum of the threads, so a thread that came and went inside the window shows as the difference.
#   python threadprof.py <out.json> <from s>,<to s> -- <exe> <args>      starts the game (run it under rundesk)
#   python threadprof.py <out.json> <from s>,<to s> --attach <path part>  waits for a process whose exe path contains
#                                                                        the text (origrun starts an original)
# The seconds are from the process's creation, as perfmin.py's window. A thread is read every second inside the window
# and exactly at its two ends.
import ctypes, ctypes.wintypes as W, json, os, subprocess, sys, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
ntdll = ctypes.WinDLL('ntdll')
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenThread.restype = W.HANDLE
k32.OpenProcess.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.LocalFree.argtypes = [ctypes.c_void_p]
PROCESS_QUERY, PROCESS_VM_READ, PROCESS_LIMITED = 0x400, 0x10, 0x1000
STILL_ACTIVE = 259


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD), ('th32ParentProcessID', W.DWORD), ('pcPriClassBase', W.LONG),
                ('dwFlags', W.DWORD), ('szExeFile', ctypes.c_wchar * 260)]


def ft(t):
    return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7


def thread_ids(pid):
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


def thread_times(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    if not k32.GetThreadTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d)):
        return None
    cyc = ctypes.c_ulonglong()
    if not k32.QueryThreadCycleTime(h, ctypes.byref(cyc)):
        return None
    return ft(c), ft(d), cyc.value


def start_address(h):
    addr = ctypes.c_void_p()
    ntdll.NtQueryInformationThread(h, 9, ctypes.byref(addr), ctypes.sizeof(addr), None)
    return addr.value or 0


def description(h):
    p = ctypes.c_void_p()
    try:
        if k32.GetThreadDescription(h, ctypes.byref(p)) == 0 and p.value:
            s = ctypes.wstring_at(p.value)
            k32.LocalFree(p)
            return s
    except Exception:
        pass
    return ''


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
    return '%s+0x%x' % (best[0], addr - best[1]) if best[1] else '?'


def modules(hp):
    arr = (ctypes.c_void_p * 1024)()
    need = W.DWORD()
    psapi.EnumProcessModulesEx(hp, arr, ctypes.sizeof(arr), ctypes.byref(need), 3)
    out = []
    for i in range(need.value // ctypes.sizeof(ctypes.c_void_p)):
        name = ctypes.create_unicode_buffer(520)
        psapi.GetModuleFileNameExW(hp, ctypes.c_void_p(arr[i]), name, 520)
        out.append(os.path.basename(name.value).lower())
    return out


def find(part):
    snap = k32.CreateToolhelp32Snapshot(0x2, 0)
    pe = PROCESSENTRY32W()
    pe.dwSize = ctypes.sizeof(pe)
    found = 0
    ok = k32.Process32FirstW(snap, ctypes.byref(pe))
    while ok and not found:
        if pe.szExeFile.lower().endswith('.exe') and pe.th32ProcessID:
            h = k32.OpenProcess(PROCESS_LIMITED, False, pe.th32ProcessID)
            if h:
                buf = ctypes.create_unicode_buffer(1024)
                n = W.DWORD(1024)
                if k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(n)) and part in buf.value.lower().replace('\\', '/'):
                    found = pe.th32ProcessID
                k32.CloseHandle(h)
        ok = k32.Process32NextW(snap, ctypes.byref(pe))
    k32.CloseHandle(snap)
    return found


def process_times(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    return ft(a), ft(c), ft(d)     # creation, kernel, user


def now():
    t = W.FILETIME()
    k32.GetSystemTimeAsFileTime(ctypes.byref(t))
    return ft(t)


def main():
    out = sys.argv[1]
    w0, w1 = (float(x) for x in sys.argv[2].split(','))
    proc = None
    if sys.argv[3] == '--attach':
        part = sys.argv[4].lower().replace('\\', '/')
        pid = 0
        while not pid:
            pid = find(part)
            time.sleep(0.005)
    else:
        # the game's own log goes beside the result (its display mode, for one, is in it)
        log = open(out + '.log', 'w')
        proc = subprocess.Popen(sys.argv[4:], stdout=log, stderr=subprocess.STDOUT)
        pid = proc.pid
    hp = k32.OpenProcess(PROCESS_QUERY | PROCESS_VM_READ | 0x100000, False, pid)
    if not hp:
        sys.exit('cannot open process %d' % pid)
    created = process_times(hp)[0]
    first, last, info, pfirst, plast = {}, {}, {}, None, None
    while True:
        code = W.DWORD()
        k32.GetExitCodeProcess(hp, ctypes.byref(code))
        if code.value != STILL_ACTIVE:
            break
        age = now() - created
        if age >= w0:
            for tid in thread_ids(pid):
                h = k32.OpenThread(0x0040 | 0x0400, False, tid)
                if not h:
                    continue
                tm = thread_times(h)
                if tm:
                    if tid not in info:
                        info[tid] = (module_of(hp, start_address(h)), description(h))
                    if tid not in first:
                        first[tid] = (age, tm)
                    last[tid] = (age, tm)
                k32.CloseHandle(h)
            pt = process_times(hp)
            mods = modules(hp)
            if pfirst is None:
                pfirst = (age, pt)
            plast = (age, pt)
            if age >= w1:
                break
            time.sleep(max(0.0, min(1.0, w1 - age - 0.002)))
        else:
            time.sleep(max(0.002, min(0.5, w0 - age - 0.002)))
    if pfirst is None:
        sys.exit('the process ended before the window')
    wall = plast[0] - pfirst[0]
    rows = []
    for tid, (t, tm) in last.items():
        t0, tm0 = first[tid]
        rows.append({'tid': tid, 'start': info[tid][0], 'name': info[tid][1], 'kernel': tm[0] - tm0[0], 'user': tm[1] - tm0[1],
                     'cycles': tm[2] - tm0[2], 'seen': round(t - t0, 2)})
    rows.sort(key=lambda r: -r['cycles'])
    pcpu = (plast[1][1] - pfirst[1][1]) + (plast[1][2] - pfirst[1][2])
    r = {'pid': pid, 'window': [w0, w1], 'wall': round(wall, 3), 'process cpu': round(pcpu, 3),
         'process percent of one core': round(100 * pcpu / wall, 2),
         'threads cpu': round(sum(x['kernel'] + x['user'] for x in rows), 3),
         'modules': mods,
         'threads cycles G': round(sum(x['cycles'] for x in rows) / 1e9, 3), 'threads': rows}
    with open(out, 'w') as f:
        json.dump(r, f)
    print('window %.1f s: process %.2f s (%.2f%% of one core), threads %.2f s, %.3f G cycles' % (
        wall, pcpu, r['process percent of one core'], r['threads cpu'], r['threads cycles G']))
    for x in rows[:12]:
        print('  %6d %6.3f Gcyc  %5.2f s (k %.2f u %.2f)  %5.2f%% core  %s %s' % (
            x['tid'], x['cycles'] / 1e9, x['kernel'] + x['user'], x['kernel'], x['user'],
            100 * (x['kernel'] + x['user']) / wall, x['start'], x['name']))
    if proc is not None and proc.poll() is None:
        proc.terminate()


main()
