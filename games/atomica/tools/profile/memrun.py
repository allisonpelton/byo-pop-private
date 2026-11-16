"""Atomica's memory at its first board: the peak private bytes and a memory report, for GAME_NOTES.md ("The software screen").

Run from the worktree root, with python -I:

    python -I games/atomica/tools/profile/memrun.py <exe in build/native> <out dir> [game args...]

It starts the exe under rundesk (windowed, in a fresh user folder: ScreenMode=0 and FullScreen=0 in its registry.ini), sends
`goto menu`, `goto strategy` and `memreport <file>` through the -commands channel, holds the first board for HOLD seconds
(10 unless set), and prints one JSON line: the peak of the process's private bytes (PrivateUsage read every 50 ms, as
docs/MEMORY.md measured), the memory report's own summary and the display line the run logged. The environment is the run's:
`SOFTWARE_SCREEN=on`, `SOFTSCREEN_SCALED=1` and the rest of docs/MEMORY.md's switches go through it. The detail of the memory
report (every picture and its copies, the heaps) is <out dir>/memreport.txt.
"""
import ctypes
import ctypes.wintypes as W
import json
import os
import re
import shutil
import subprocess
import sys
import threading
import time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.TerminateProcess.argtypes = [W.HANDLE, W.UINT]
k32.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, ctypes.POINTER(W.DWORD)]
psapi.GetProcessMemoryInfo.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD]
psapi.EnumProcesses.argtypes = [ctypes.POINTER(W.DWORD), W.DWORD, ctypes.POINTER(W.DWORD)]


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


def private(h):
    p = PMC()
    p.cb = ctypes.sizeof(p)
    return p.PrivateUsage if psapi.GetProcessMemoryInfo(h, ctypes.byref(p), p.cb) else None


def find_pid(path):
    arr = (W.DWORD * 4096)()
    n = W.DWORD()
    psapi.EnumProcesses(arr, ctypes.sizeof(arr), ctypes.byref(n))
    want = os.path.normcase(os.path.abspath(path))
    for i in range(n.value // 4):
        h = k32.OpenProcess(0x1000, False, arr[i])
        if not h:
            continue
        buf = ctypes.create_unicode_buffer(1024)
        sz = W.DWORD(1024)
        ok = k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(sz))
        k32.CloseHandle(h)
        if ok and os.path.normcase(buf.value) == want:
            return arr[i]
    return None


def text(p):
    try:
        return open(p, 'rb').read().decode('utf8', 'replace')
    except OSError:
        return ''


def main():
    root = os.getcwd()
    exe = os.path.join(root, 'build', 'native', sys.argv[1])
    out = os.path.abspath(sys.argv[2])
    extra = sys.argv[3:]
    hold = float(os.environ.get('HOLD', '10'))
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    cf, lf = os.path.join(out, 'cmds.txt'), os.path.join(out, 'log.txt')
    open(cf, 'w').close()
    with open(os.path.join(out, 'user', 'registry.ini'), 'w', newline='\n') as f:
        f.write('; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\\PopCap\\Atomica]\nScreenMode=dword:0\nFullScreen=dword:0\n')
    cmd = [os.path.join(root, 'build', 'native', 'rundesk.exe'), exe, '-gamedir=' + os.path.join(root, 'assets', 'atomica'),
           '-commands=' + cf, '-userdir=' + os.path.join(out, 'user')] + extra
    res = {'exe': sys.argv[1], 'args': extra, 'ok': False}
    with open(lf, 'wb') as lfh:
        pr = subprocess.Popen(cmd, cwd=root, stdout=lfh, stderr=subprocess.STDOUT)
        t0, pid = time.time(), None
        while time.time() - t0 < 30 and pid is None and pr.poll() is None:
            pid = find_pid(exe)
            time.sleep(0.02)
        if pid is None:
            res['error'] = 'process not found'
            print(json.dumps(res))
            return 1
        h = k32.OpenProcess(0x1000 | 0x400 | 0x1, False, pid)
        peak, stop, series = [0], [False], []

        def sampler():
            while not stop[0]:
                v = private(h)
                if v is None:
                    break
                peak[0] = max(peak[0], v)
                series.append((round(time.time() - t0, 2), round(v / 1048576, 1)))
                time.sleep(0.05)
        th = threading.Thread(target=sampler)
        th.start()

        def send(line):
            with open(cf, 'a', newline='\n') as f:
                f.write(line + '\n')

        def wait_for(pat, start, timeout):
            end = time.time() + timeout
            while time.time() < end and pr.poll() is None:
                m = re.search(pat, text(lf)[start:], re.M)
                if m:
                    return m
                time.sleep(0.1)
            return None
        try:
            for _ in range(20):          # `goto` answers ERROR until the menu is up
                pos = len(text(lf))
                for c in ('goto menu', 'wait 30', 'goto strategy'):
                    send(c)
                m = wait_for(r'^(?:INFO: )?(OK|ERROR) goto strategy', pos, 8)
                if m and m.group(1) == 'OK':
                    res['goto_s'] = round(time.time() - t0, 1)
                    break
                time.sleep(1.0)
            time.sleep(hold)
            pos = len(text(lf))
            send('memreport ' + os.path.join(out, 'memreport.txt').replace('\\', '/'))
            m = wait_for(r'MEMREPORT (\{.*\})', pos, 20)
            if m:
                res['memreport'] = json.loads(m.group(1))
                res['ok'] = True
            time.sleep(0.3)
        finally:
            stop[0] = True
            th.join()
            res['peak_private_mb'] = round(peak[0] / 1048576, 1)
            dl = re.findall(r'display: .*', text(lf))
            res['display'] = dl[-1][:170] if dl else ''
            k32.TerminateProcess(h, 0)
            k32.CloseHandle(h)
            try:
                pr.wait(timeout=15)
            except Exception:
                pr.kill()
    json.dump(series, open(os.path.join(out, 'private_series.json'), 'w'))
    shutil.rmtree(os.path.join(out, 'user'), ignore_errors=True)
    print(json.dumps(res))
    return 0


if __name__ == '__main__':
    sys.exit(main())
