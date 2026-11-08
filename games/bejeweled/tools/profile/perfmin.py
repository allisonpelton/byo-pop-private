# One run of a game, sampled the same way whether it is the port or the original: every 50 ms the working set, the private
# bytes, the processor time and the number of threads of the game's process, from its creation to its exit. For the final
# merge's performance numbers (build/slots/news.md, 2026-11-08): the original and the port back to back on the same machine.
#
#   python perfmin.py <out.json> <window-from>,<window-to> -- <exe> <args...>       starts the game (run it under rundesk)
#   python perfmin.py <out.json> <window-from>,<window-to> --attach <path part>     waits for a process whose exe path
#                                                                                   contains the text (origrun starts the original)
#
# The window is in seconds from the process's creation: the numbers it gives are the processor time used and the highest
# working set and private bytes inside it, next to the whole run's. The samples go to <out.json>.
import ctypes, ctypes.wintypes as W, json, os, subprocess, sys, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
INVALID = ctypes.c_void_p(-1).value
PROCESS_QUERY_LIMITED, PROCESS_QUERY, PROCESS_VM_READ, SYNCHRONIZE = 0x1000, 0x400, 0x10, 0x100000
STILL_ACTIVE = 259


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD), ('th32ParentProcessID', W.DWORD), ('pcPriClassBase', W.LONG),
                ('dwFlags', W.DWORD), ('szExeFile', ctypes.c_wchar * 260)]


def filetime(t):
    return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7


def times(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    return filetime(a), filetime(c) + filetime(d)   # creation (seconds since 1601), kernel + user processor time


def threads(pid):
    snap = k32.CreateToolhelp32Snapshot(0x4, 0)   # TH32CS_SNAPTHREAD
    n = 0
    te = THREADENTRY32()
    te.dwSize = ctypes.sizeof(te)
    ok = k32.Thread32First(snap, ctypes.byref(te))
    while ok:
        if te.th32OwnerProcessID == pid:
            n += 1
        ok = k32.Thread32Next(snap, ctypes.byref(te))
    k32.CloseHandle(snap)
    return n


def find(part):
    """The id of a process whose exe path contains part (lower case, forward slashes), or 0."""
    snap = k32.CreateToolhelp32Snapshot(0x2, 0)   # TH32CS_SNAPPROCESS
    pe = PROCESSENTRY32W()
    pe.dwSize = ctypes.sizeof(pe)
    found = 0
    ok = k32.Process32FirstW(snap, ctypes.byref(pe))
    while ok and not found:
        if pe.szExeFile.lower().endswith('.exe') and pe.th32ProcessID:
            h = k32.OpenProcess(PROCESS_QUERY_LIMITED, False, pe.th32ProcessID)
            if h:
                buf = ctypes.create_unicode_buffer(1024)
                n = W.DWORD(1024)
                if k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(n)) and part in buf.value.lower().replace('\\', '/'):
                    found = pe.th32ProcessID
                k32.CloseHandle(h)
        ok = k32.Process32NextW(snap, ctypes.byref(pe))
    k32.CloseHandle(snap)
    return found


def now_filetime():
    t = W.FILETIME()
    k32.GetSystemTimeAsFileTime(ctypes.byref(t))
    return filetime(t)


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
        proc = subprocess.Popen(sys.argv[4:], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        pid = proc.pid
    h = k32.OpenProcess(PROCESS_QUERY | PROCESS_VM_READ | SYNCHRONIZE, False, pid)
    if not h:
        sys.exit('cannot open process %d (error %d)' % (pid, ctypes.get_last_error()))
    created, _ = times(h)
    samples = []
    while True:
        code = W.DWORD()
        k32.GetExitCodeProcess(h, ctypes.byref(code))
        if code.value != STILL_ACTIVE:
            break
        m = PMC()
        m.cb = ctypes.sizeof(m)
        if psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb):
            _, cpu = times(h)
            samples.append((round(now_filetime() - created, 3), m.WorkingSetSize, m.PrivateUsage, round(cpu, 3), threads(pid),
                            m.PeakWorkingSetSize))
        time.sleep(0.05)
    inside = [s for s in samples if w0 <= s[0] <= w1]
    MB = 1 << 20
    r = {'pid': pid, 'window': [w0, w1], 'samples': samples}
    if samples:
        r['run seconds'] = samples[-1][0]
        r['peak working set MB'] = round(max(s[5] for s in samples) / MB, 1)
        r['peak private MB'] = round(max(s[2] for s in samples) / MB, 1)
        r['cpu seconds, whole run'] = samples[-1][3]
    if len(inside) > 1:
        r['window peak working set MB'] = round(max(s[1] for s in inside) / MB, 1)
        r['window peak private MB'] = round(max(s[2] for s in inside) / MB, 1)
        r['window cpu seconds'] = round(inside[-1][3] - inside[0][3], 3)
        r['window seconds'] = round(inside[-1][0] - inside[0][0], 3)
        r['window cpu percent of one core'] = round(100 * r['window cpu seconds'] / r['window seconds'], 2)
    with open(out, 'w') as f:
        json.dump(r, f)
    print({k: v for k, v in r.items() if k != 'samples'})


main()
