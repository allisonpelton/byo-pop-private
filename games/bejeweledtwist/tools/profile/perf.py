"""Load time, memory and processor time of Bejeweled Twist, the port and the original, for NOTES.md "Performance".

From the worktree root:

    build/native/rundesk.exe <python.exe> games/bejeweledtwist/tools/profile/perf.py port <out dir> [runs]
    <python.exe> games/bejeweledtwist/tools/profile/perf.py orig <out dir> [runs]      (not under rundesk)

Both run windowed on the real clock, the port in 2D (`-nod3d9`), with the game's own Classic autoplay (`-autoplay=classic
-seed=1234` for the port; the original has no -seed switch), or with "menu" as the fourth argument on the main menu with
no autoplay. The original's autoplay crashes about 12 seconds in on this copy, so NOTES.md compares the two on the menu. The port starts with a fresh user folder and the live
command channel and asks `state` until the screen is no longer the loader; the original starts through origrun (its own
copy under build/origrun/bjtperf/game, a script that only waits and quits). The installed retail copy starts through
PopCap's CD launcher, so the copy is staged first (`origrun --game bejeweledtwist --run bjtperf --copy-only`) and its
BejeweledTwist.exe replaced with the game carved out of the launcher (re/bejeweledtwist/bin/BejeweledTwistGame.exe,
PORT_PLAN.md) and its app's main menu pointer (app +0x864, the app at
0xe0b698) is read every few milliseconds until it is set. Either way the load time counts from the process's creation to
the main menu's making, the moment the loader screen hands over.

The game's process is sampled every 0.1 s: working set, private bytes (and the system's own peaks of both) and processor
time (kernel + user). Each run prints one JSON line: the load time, the peaks over the whole run, the processor time over
the minute after loading (60 s on the real clock) and the game processes of other sessions running when it
started.
"""
import ctypes
import ctypes.wintypes as W
import json
import os
import shutil
import subprocess
import sys
import time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.argtypes = [W.DWORD, W.DWORD]
k32.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, ctypes.POINTER(W.DWORD)]
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t,
                                  ctypes.POINTER(ctypes.c_size_t)]
psapi.GetProcessMemoryInfo.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD]

QUERY_LIMITED, QUERY, VM_READ = 0x1000, 0x400, 0x10
EPOCH_DELTA = 11644473600  # seconds from 1601 to 1970
ROOT = os.getcwd()
EXE = os.path.join(ROOT, 'build', 'native', 'bejeweledtwist.exe')
GAMEDIR = os.path.join(ROOT, 'assets', 'bejeweledtwist')
ORIGRUN = os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe')
APP, MAIN_MENU = 0xe0b698, 0x864
MINUTE = 60.0
AUTOPLAY = True  # "menu" as the fourth argument: no autoplay, the minute on the main menu


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


class PE32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD),
                ('th32DefaultHeapID', ctypes.c_size_t), ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD),
                ('th32ParentProcessID', W.DWORD), ('pcPriClassBase', ctypes.c_long), ('dwFlags', W.DWORD),
                ('szExeFile', ctypes.c_wchar * 260)]


def processes():
    """(pid, exe name) of every process"""
    snap = k32.CreateToolhelp32Snapshot(2, 0)
    e = PE32()
    e.dwSize = ctypes.sizeof(PE32)
    out = []
    ok = k32.Process32FirstW(snap, ctypes.byref(e))
    while ok:
        out.append((e.th32ProcessID, e.szExeFile))
        ok = k32.Process32NextW(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


def image(pid):
    h = k32.OpenProcess(QUERY_LIMITED, False, pid)
    if not h:
        return ''
    buf = ctypes.create_unicode_buffer(1024)
    n = W.DWORD(1024)
    ok = k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(n))
    k32.CloseHandle(h)
    return buf.value if ok else ''


def game_count():
    """the game processes of the whole machine (tools/gamecount.py's count)"""
    r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'gamecount.py'), '--limit', '99'],
                       capture_output=True, text=True)
    return sum(1 for line in r.stdout.splitlines() if line.strip()[:1].isdigit())


class Sampler:
    def __init__(self, pid):
        self.h = k32.OpenProcess(QUERY | VM_READ, False, pid)
        ft = [W.FILETIME() for _ in range(4)]
        k32.GetProcessTimes(self.h, *[ctypes.byref(f) for f in ft])
        self.created = (ft[0].dwHighDateTime << 32 | ft[0].dwLowDateTime) / 1e7 - EPOCH_DELTA
        self.peak_ws = self.peak_private = 0
        self.sys_peak_ws = self.sys_peak_private = 0

    def cpu(self):
        ft = [W.FILETIME() for _ in range(4)]
        if not k32.GetProcessTimes(self.h, *[ctypes.byref(f) for f in ft]):
            return None
        return sum((f.dwHighDateTime << 32 | f.dwLowDateTime) for f in ft[2:]) / 1e7

    def sample(self):
        m = PMC()
        m.cb = ctypes.sizeof(PMC)
        if psapi.GetProcessMemoryInfo(self.h, ctypes.byref(m), m.cb):
            self.peak_ws = max(self.peak_ws, m.WorkingSetSize)
            self.peak_private = max(self.peak_private, m.PrivateUsage)
            self.sys_peak_ws = max(self.sys_peak_ws, m.PeakWorkingSetSize)
            self.sys_peak_private = max(self.sys_peak_private, m.PeakPagefileUsage)

    def read32(self, addr):
        v = ctypes.c_uint32()
        n = ctypes.c_size_t()
        ok = k32.ReadProcessMemory(self.h, addr, ctypes.byref(v), 4, ctypes.byref(n))
        return v.value if ok and n.value == 4 else None

    def result(self, load, cpu_minute):
        mb = 1 << 20
        return {'load_s': None if load is None else round(load, 2), 'peak_ws_mb': round(self.peak_ws / mb, 1),
                'peak_private_mb': round(self.peak_private / mb, 1),
                'sys_peak_ws_mb': round(self.sys_peak_ws / mb, 1),
                'sys_peak_private_mb': round(self.sys_peak_private / mb, 1),
                'cpu_minute_s': None if cpu_minute is None else round(cpu_minute, 2)}


def measure(s, loaded, alive):
    """samples until loaded() says the main menu is made, then for a minute; the load time and the minute's processor time"""
    load = None
    while load is None:
        s.sample()
        if not alive():
            return None, None
        if loaded():
            load = time.time() - s.created
        time.sleep(0.005)
    start = time.time()
    c0 = s.cpu()
    while time.time() - start < MINUTE and alive():
        s.sample()
        time.sleep(0.1)
    c1 = s.cpu()
    s.sample()
    return load, (c1 - c0) if c0 is not None and c1 is not None else None


def run_port(out, n):
    user = os.path.join(out, 'user%d' % n)
    shutil.rmtree(user, ignore_errors=True)
    os.makedirs(user)
    with open(os.path.join(user, 'registry.ini'), 'w') as f:
        f.write('[SOFTWARE\\Steam\\BejeweledTwist]\nScreenMode=dword:0\n')
    cf, ri, log = (os.path.join(out, x) for x in ('commands.txt', 'realinput.txt', 'stdout%d.txt' % n))
    open(cf, 'w').close()
    open(ri, 'w').close()
    others = game_count()
    with open(log, 'w') as lf:
        p = subprocess.Popen([EXE, '-gamedir=' + GAMEDIR, '-userdir=' + user, '-commands=' + cf, '-realinput=' + ri,
                              '-nod3d9'] + (['-autoplay=classic', '-seed=1234'] if AUTOPLAY else []),
                             stdout=lf, stderr=subprocess.STDOUT)
    s = Sampler(p.pid)
    asked = [0.0]

    def loaded():
        # asks for the state every 20 ms; the menu (or a game, or a new player's name) means the loader is gone
        now = time.time()
        if now - asked[0] > 0.02:
            asked[0] = now
            with open(cf, 'a') as f:
                f.write('state\n')
        with open(log, errors='replace') as f:
            text = f.read()
        return any(('"screen":"%s"' % k) in text for k in ('menu', 'game', 'newplayer'))

    load, cpu = measure(s, loaded, lambda: p.poll() is None)
    p.kill()
    p.wait()
    r = s.result(load, cpu)
    r.update(side='port', run=n, others=others)
    return r


def run_orig(out, n):
    script = os.path.join(out, 'orig.txt')
    with open(script, 'w') as f:
        f.write('wait %d\nquit\n' % int((MINUTE + 40) * 1000))
    others = game_count()
    run = 'bjtperf'
    exe = os.path.join(ROOT, 'build', 'origrun', run, 'game', 'BejeweledTwist.exe')
    p = subprocess.Popen([ORIGRUN, '--exe', exe, '--run', run, '--script', script, '--clock', 'real', '--timeout', '180']
                         + (['--arg', '-autoplay'] if AUTOPLAY else []),
                         stdout=open(os.path.join(out, 'origrun%d.txt' % n), 'w'), stderr=subprocess.STDOUT)
    copy = os.path.normcase(os.path.join(ROOT, 'build', 'origrun', run))
    pid = None
    t0 = time.time()
    while pid is None and time.time() - t0 < 60 and p.poll() is None:
        for q, name in processes():
            if name.lower() == 'bejeweledtwist.exe' and os.path.normcase(image(q)).startswith(copy):
                pid = q
        time.sleep(0.005)
    if pid is None:
        p.wait()
        return {'side': 'orig', 'run': n, 'error': 'no game process'}
    s = Sampler(pid)

    def loaded():
        app = s.read32(APP)
        return bool(app) and bool(s.read32(app + MAIN_MENU))

    load, cpu = measure(s, loaded, lambda: s.cpu() is not None and p.poll() is None)
    p.wait()
    r = s.result(load, cpu)
    r.update(side='orig', run=n, others=others)
    return r


def main():
    side, out = sys.argv[1], os.path.abspath(sys.argv[2])
    runs = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    global AUTOPLAY
    AUTOPLAY = not (len(sys.argv) > 4 and sys.argv[4] == 'menu')
    os.makedirs(out, exist_ok=True)
    for n in range(runs):
        r = run_port(out, n) if side == 'port' else run_orig(out, n)
        print(json.dumps(r), flush=True)


if __name__ == '__main__':
    main()
