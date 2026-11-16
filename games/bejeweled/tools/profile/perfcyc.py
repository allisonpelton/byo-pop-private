# One run of a game sampled the same way whether it is the port or the original, with the processor time as the process's
# CYCLE count (QueryProcessCycleTime: every thread of the process together, the driver's and the audio thread's included;
# docs/COORDINATION.md, "The target": Windows charges GetProcessTimes by the 15.6 ms clock tick, so perfmin.py's seconds are
# not used for the comparison with the original). Every 50 ms: the private bytes and the cycles of the game's process.
#
#   python perfcyc.py <out.json> <window-from>,<window-to> --attach <path part>
#
# Waits for a process whose exe path contains the text (lower case, forward slashes), samples it until it ends. The window is
# in seconds from the process's creation: the cycles it used there and the highest private bytes inside it, next to the whole
# run's peak. The game is started by the caller (under tools/pincore.py for a processor-time run), never by this script, so
# that the sampler does not share the game's core.
import ctypes, ctypes.wintypes as W, json, sys, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
PROCESS_QUERY_LIMITED, PROCESS_QUERY, PROCESS_VM_READ, SYNCHRONIZE = 0x1000, 0x400, 0x10, 0x100000
STILL_ACTIVE = 259


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD), ('th32ParentProcessID', W.DWORD), ('pcPriClassBase', W.LONG),
                ('dwFlags', W.DWORD), ('szExeFile', ctypes.c_wchar * 260)]


def filetime(t):
    return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7


def creation(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    return filetime(a)


def cycles(h):
    c = ctypes.c_ulonglong()
    k32.QueryProcessCycleTime(h, ctypes.byref(c))
    return c.value


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
    assert sys.argv[3] == '--attach'
    part = sys.argv[4].lower().replace('\\', '/')
    pid = 0
    while not pid:
        pid = find(part)
        time.sleep(0.005)
    h = k32.OpenProcess(PROCESS_QUERY | PROCESS_VM_READ | SYNCHRONIZE, False, pid)
    if not h:
        sys.exit('cannot open process %d (error %d)' % (pid, ctypes.get_last_error()))
    created = creation(h)
    samples = []
    while True:
        code = W.DWORD()
        k32.GetExitCodeProcess(h, ctypes.byref(code))
        if code.value != STILL_ACTIVE:
            break
        m = PMC()
        m.cb = ctypes.sizeof(m)
        if psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb):
            samples.append((round(now_filetime() - created, 3), m.PrivateUsage, cycles(h), m.WorkingSetSize))
        time.sleep(0.05)
    inside = [s for s in samples if w0 <= s[0] <= w1]
    MB = 1 << 20
    r = {'pid': pid, 'window': [w0, w1], 'samples': samples}
    if samples:
        r['run seconds'] = samples[-1][0]
        r['peak private MB'] = round(max(s[1] for s in samples) / MB, 2)
        r['peak working set MB'] = round(max(s[3] for s in samples) / MB, 2)
        r['G cycles, whole run'] = round(samples[-1][2] / 1e9, 3)
    if len(inside) > 1:
        r['window peak private MB'] = round(max(s[1] for s in inside) / MB, 2)
        r['window G cycles'] = round((inside[-1][2] - inside[0][2]) / 1e9, 3)
        r['window seconds'] = round(inside[-1][0] - inside[0][0], 3)
        r['window M cycles a second'] = round((inside[-1][2] - inside[0][2]) / 1e6 / r['window seconds'], 1)
    with open(out, 'w') as f:
        json.dump(r, f)
    print({k: v for k, v in r.items() if k != 'samples'})


main()
