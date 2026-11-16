"""Rocket Mania's memory at its first board, 1x: peak private bytes (sampled every 50 ms) and the game's own memreport.
Run from the worktree root, one run at a time (the machine's game count is the caller's to check):

    python -I games/rocketmania/tools/profile/mem.py <out dir> <Is3D 0|1> [<exe, default build/native/rocketmania.exe>]

The port runs under rundesk, windowed at the game's own size (-windowscale=1), with a fresh user dir whose registry has ScreenMode=0
and the given Is3D. It plays the standard `goto classic 2` and, ten seconds into the game, `memreport`. One RESULT line: the
peak private megabytes, the display line of the log (renderer or software) and memreport's totals. Files: <out dir>/log.txt.
"""
import ctypes, ctypes.wintypes as W, json, os, re, shutil, subprocess, sys, threading, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.CloseHandle.argtypes = [W.HANDLE]
k32.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, ctypes.POINTER(W.DWORD)]
psapi.EnumProcesses.argtypes = [ctypes.POINTER(W.DWORD), W.DWORD, ctypes.POINTER(W.DWORD)]
k32.TerminateProcess.argtypes = [W.HANDLE, W.UINT]
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
psapi.GetProcessMemoryInfo.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD]


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


def private(handle):
    counters = PMC()
    counters.cb = ctypes.sizeof(counters)
    return counters.PrivateUsage if psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb) else None


def find_pid(path):
    """the process whose image is <path> (the game under rundesk), found without a process listing's cost"""
    ids = (W.DWORD * 4096)()
    count = W.DWORD()
    psapi.EnumProcesses(ids, ctypes.sizeof(ids), ctypes.byref(count))
    want = os.path.normcase(os.path.abspath(path))
    for index in range(count.value // 4):
        handle = k32.OpenProcess(0x1000, False, ids[index])
        if not handle:
            continue
        name = ctypes.create_unicode_buffer(1024)
        size = W.DWORD(1024)
        found = k32.QueryFullProcessImageNameW(handle, 0, name, ctypes.byref(size))
        k32.CloseHandle(handle)
        if found and os.path.normcase(name.value) == want:
            return ids[index]
    return None


def main():
    out, is3d = os.path.abspath(sys.argv[1]), sys.argv[2]
    exe = os.path.abspath(sys.argv[3] if len(sys.argv) > 3 else 'build/native/rocketmania.exe')
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    with open(os.path.join(out, 'user', 'registry.ini'), 'w', newline='\n') as f:
        f.write('; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\\PopCap\\RocketMania]\nScreenMode=dword:0\n'
                'FullScreen=dword:0\nIs3D=dword:%s\n' % is3d)
    cmds = os.path.join(out, 'cmds.txt')
    open(cmds, 'w').close()
    log = os.path.join(out, 'log.txt')

    def text():
        try:
            return open(log, 'rb').read().decode('utf8', 'replace')
        except OSError:
            return ''
    cmd = [os.path.abspath('build/native/rundesk.exe'), exe, '-commands=' + cmds, '-userdir=' + os.path.join(out, 'user'), '-windowscale=1']
    peak = [0]
    stop = [False]
    samples = []
    with open(log, 'wb') as logfile:
        proc = subprocess.Popen(cmd, stdout=logfile, stderr=subprocess.STDOUT)
        pid = None
        t0 = time.time()
        while pid is None and time.time() - t0 < 40 and proc.poll() is None:
            pid = find_pid(exe)
            time.sleep(0.01)
        if pid is None:
            print('RESULT error: process not found')
            proc.kill()
            return 1
        handle = k32.OpenProcess(0x1000 | 0x400 | 0x1, False, pid)

        def sampler():
            while not stop[0]:
                value = private(handle)
                if value is None:
                    break
                peak[0] = max(peak[0], value)
                samples.append((time.time(), value))
                time.sleep(0.05)
        sampling = threading.Thread(target=sampler)
        sampling.start()
        result = {}
        try:
            end = time.time() + 120
            while time.time() < end and proc.poll() is None:
                pos = len(text())
                with open(cmds, 'a', newline='\n') as f:
                    f.write('goto classic 2\n')
                found = None
                for _ in range(60):
                    found = re.search(r'^(?:INFO: )?(OK|ERROR) goto', text()[pos:], re.M)
                    if found:
                        break
                    time.sleep(0.1)
                if found and found.group(1) == 'OK':
                    break
                time.sleep(1)
            time.sleep(10)
            pos = len(text())
            with open(cmds, 'a', newline='\n') as f:
                f.write('memreport\n')
            for _ in range(150):
                found = re.search(r'MEMREPORT (\{.*\})', text()[pos:])
                if found:
                    result = json.loads(found.group(1))
                    break
                time.sleep(0.1)
            time.sleep(0.3)
        finally:
            stop[0] = True
            sampling.join()
            display = re.findall(r'display: .*', text())
            k32.TerminateProcess(handle, 0)
            proc.kill()
    summary = {key: value for key, value in result.items() if not isinstance(value, (list, dict))}
    print('RESULT peak_mb=%.1f display=%s memreport=%s' % (peak[0] / 1048576, (display[-1][:90] if display else ''), json.dumps(summary)[:600]))
    json.dump(result, open(os.path.join(out, 'memreport.json'), 'w'))
    with open(os.path.join(out, 'samples.csv'), 'w') as f:
        for stamp, value in samples:
            f.write('%.2f,%.1f\n' % (stamp - samples[0][0], value / 1048576))
    return 0


sys.exit(main())
