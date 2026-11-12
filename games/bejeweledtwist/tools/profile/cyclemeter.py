"""The processor time of one game process as cycles (docs/COORDINATION.md, "The target": like for like, by cycle counts).

    python -I cyclemeter.py <out.json> <from>,<to> --attach <path part>   waits for a process whose exe path holds <path part>
    python -I cyclemeter.py <out.json> <from>,<to> -- <exe> <args...>      starts the process itself

Every quarter second it reads QueryProcessCycleTime, the cycles the process's threads have run, those that have ended
included, and the process's age from its creation time. It writes the cycles in the window <from> to <to> seconds of the
process's life (read at the first sample at or past each end) and over the whole run, until the process exits. Cycle
counts, not GetProcessTimes: Windows charges those at the 15.6 ms clock tick, so a reading moves by whole ticks.
"""
import ctypes
import ctypes.wintypes as W
import json
import os
import subprocess
import sys
import time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
SYNCHRONIZE = 0x100000


def cycles(h):
    c = ctypes.c_ulonglong()
    return c.value if k32.QueryProcessCycleTime(h, ctypes.byref(c)) else None


def created(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    return (a.dwHighDateTime << 32 | a.dwLowDateTime) / 1e7


def now():
    t = W.FILETIME()
    k32.GetSystemTimeAsFileTime(ctypes.byref(t))
    return (t.dwHighDateTime << 32 | t.dwLowDateTime) / 1e7


def find(part):
    part = part.replace('/', '\\').lower()
    arr = (W.DWORD * 4096)()
    got = W.DWORD()
    psapi.EnumProcesses(arr, ctypes.sizeof(arr), ctypes.byref(got))
    for pid in arr[:got.value // 4]:
        h = k32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, False, pid)
        if not h:
            continue
        buf = ctypes.create_unicode_buffer(1024)
        n = W.DWORD(1024)
        if k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(n)) and part in buf.value.lower():
            return pid, h
        k32.CloseHandle(h)
    return None, None


def main():
    out, win = sys.argv[1], sys.argv[2]
    w0, w1 = (float(x) for x in win.split(','))
    proc = None
    if sys.argv[3] == '--attach':
        deadline = time.time() + 120
        pid, h = find(sys.argv[4])
        while h is None and time.time() < deadline:
            time.sleep(0.05)
            pid, h = find(sys.argv[4])
        if h is None:
            sys.exit('no process matching ' + sys.argv[4])
    else:
        proc = subprocess.Popen(sys.argv[sys.argv.index('--') + 1:])
        pid = proc.pid
        h = k32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, False, pid)
    born = created(h)
    at0 = at1 = None
    last = 0
    samples = []
    while k32.WaitForSingleObject(h, 250) != 0:
        c = cycles(h)
        if c is None:
            continue
        age = now() - born
        last = c
        samples.append((round(age, 2), c))
        if at0 is None and age >= w0:
            at0 = (age, c)
        if at1 is None and age >= w1:
            at1 = (age, c)
    c = cycles(h)
    total = c if c is not None else last
    res = {'pid': pid, 'window': [w0, w1], 'total_gcycles': round(total / 1e9, 3), 'lifetime_s': round(now() - born, 2)}
    if at0 and at1:
        res['window_gcycles'] = round((at1[1] - at0[1]) / 1e9, 3)
        res['window_read_at'] = [round(at0[0], 2), round(at1[0], 2)]
    res['samples'] = samples
    with open(out, 'w') as f:
        json.dump(res, f)
    print('%s: window %s Gcycles, whole run %.3f Gcycles in %.1f s' % (
        os.path.basename(out), res.get('window_gcycles', 'n/a'), total / 1e9, res['lifetime_s']))
    if proc:
        proc.wait()


main()
