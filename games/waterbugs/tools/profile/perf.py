"""Load time, memory and processor time of Water Bugs, the port and the original, for GAME_NOTES.md "Performance".

From the worktree root (the port's and the original's runs are real time, the port's without -fast):

    build/native/rundesk.exe <python.exe> games/waterbugs/tools/profile/perf.py port-load <out dir> [runs]
    build/native/rundesk.exe <python.exe> games/waterbugs/tools/profile/perf.py port-minute <out dir> [runs]
    <python.exe> games/waterbugs/tools/profile/perf.py orig-load <out dir> [runs]       (not under rundesk: origrun puts the
    <python.exe> games/waterbugs/tools/profile/perf.py orig-minute <out dir> [runs]      game under it itself)

Both games run in a window (fullscreen="0" in their cp.cfg: the original's full screen is DirectDraw's own mode, which the
private desktop cannot open without dgVoodoo2, and the numbers should be those of the game and not of a wrapper). The port
runs natively with the live command channel (as the real-input driver does), the original under tools/origrun on its real
clock from a copy in C:/byo-og/wbperf (build it once with `build/origrun-bin/origrun.exe --game waterbugs --run wbperf
--stage-root C:/byo-og --copy-only`; this script puts "fullscreen=0" in the copy's cp.cfg and takes dgVoodoo2 out of it).

load: the time from the creation of the process to the main menu: the first screen that takes input, after the two logos
(the port answers `state` with the screen "menu"; the original's current level, gLevels.current at 0x473138, has kind 1 after
kind 14, the logo). minute: the load, then the clicks that start the first board (Start at the menu, the first world on the
map, the story screen's button), then the board with the ship at rest for 70 s; the scripted minute is the 60 s that begin
10 s after the board has started. The same clicks in both; the original's are posted by origrun at the game steps
(--counter gFrame at 0x460a1c, 1 ms a step) with its scripted cursor, the port's are real-input lines sent live.

Both runs sample the game's process every 0.1 s: working set, private bytes (and the system's own peaks of both), processor
time (kernel + user). Each run prints one JSON line: the first screen's time, and the peaks and the processor time over the
whole run and over the scripted minute, the machine's busy fraction during the run, and the game processes of other sessions
running when it started and ended. The minute's game steps are counted at its two ends (the port by `state`, the original by
reading gFrame), to show that the game kept real time.
"""
import ctypes
import ctypes.wintypes as W
import json
import os
import shutil
import statistics
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'realplay'))

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.argtypes = [W.DWORD, W.DWORD]
k32.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, ctypes.POINTER(W.DWORD)]
k32.TerminateProcess.argtypes = [W.HANDLE, W.UINT]
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
psapi.GetProcessMemoryInfo.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD]

QUERY_LIMITED, QUERY, VM_READ, TERMINATE = 0x1000, 0x400, 0x10, 0x1
EPOCH_DELTA = 11644473600  # seconds from 1601 to 1970
CFG = '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n'
CLICKS = [(400, 303), (76, 263), (400, 467)]   # Start at the main menu, the first world on the map, the story's button
BOARD_AT = 10      # seconds from the board's start to the scripted minute
MINUTE = 60


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


class PE32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD), ('th32ParentProcessID', W.DWORD),
                ('pcPriClassBase', ctypes.c_long), ('dwFlags', W.DWORD), ('szExeFile', ctypes.c_char * 260)]


def filetime_seconds(t):
    return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7


def sample(h):
    """(working set, private bytes, peak working set, peak private, processor seconds) of the process, or None."""
    m = PMC()
    m.cb = ctypes.sizeof(PMC)
    if not psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb):
        return None
    c, e, k, u = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    if not k32.GetProcessTimes(h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(k), ctypes.byref(u)):
        return None
    return (m.WorkingSetSize, m.PrivateUsage, m.PeakWorkingSetSize, m.PeakPagefileUsage,
            filetime_seconds(k) + filetime_seconds(u))


def creation_time(h):
    """When the process was created, in seconds since 1970 (as time.time() counts them)."""
    c, e, k, u = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(k), ctypes.byref(u))
    return filetime_seconds(c) - EPOCH_DELTA


def system_times():
    i, k, u = W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetSystemTimes(ctypes.byref(i), ctypes.byref(k), ctypes.byref(u))
    return filetime_seconds(i), filetime_seconds(k) + filetime_seconds(u)  # kernel time includes the idle time


def busy_fraction(a, b):
    return 1.0 - (b[0] - a[0]) / (b[1] - a[1]) if b[1] > a[1] else 0.0


def processes():
    snap = k32.CreateToolhelp32Snapshot(2, 0)
    pe = PE32()
    pe.dwSize = ctypes.sizeof(PE32)
    out = []
    ok = k32.Process32First(snap, ctypes.byref(pe))
    while ok:
        out.append((pe.th32ProcessID, pe.szExeFile.decode('mbcs', 'replace')))
        ok = k32.Process32Next(snap, ctypes.byref(pe))
    k32.CloseHandle(snap)
    return out


def image_path(pid):
    h = k32.OpenProcess(QUERY_LIMITED, False, pid)
    if not h:
        return ''
    buf = ctypes.create_unicode_buffer(1024)
    n = W.DWORD(1024)
    ok = k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(n))
    k32.CloseHandle(h)
    return buf.value if ok else ''


TOOLS = {'rundesk.exe', 'lsprobe.exe', 'origrun.exe', 'bejeweled_verify.exe'}


def game_processes(mine):
    """The game processes of other sessions: every exe in a byo-pop folder or a staged original, except tools and `mine`."""
    out = []
    for pid, name in processes():
        if name.lower() in TOOLS:
            continue
        path = image_path(pid).lower()
        if ('\\byo-pop\\' in path or '\\byo-og\\' in path) and not any(path.startswith(m.lower()) for m in mine):
            out.append(name)
    return sorted(out)


def mine_prefixes():
    return [os.getcwd() + '\\', 'C:\\byo-og\\wbperf']


def find_process(name, folder):
    for pid, n in processes():
        if n.lower() == name.lower() and folder.lower() in image_path(pid).lower():
            return pid
    return None


def read32(h, address):
    buf = ctypes.c_uint32(0)
    got = ctypes.c_size_t(0)
    if not k32.ReadProcessMemory(h, address, ctypes.byref(buf), 4, ctypes.byref(got)) or got.value != 4:
        return None
    return buf.value


def summarise(samples, lo, hi):
    """samples: (t, ws, private, peak ws, peak private, cpu) with t in seconds from the process's creation."""
    def span(a, b):
        s = [x for x in samples if x[0] >= a and (b is None or x[0] <= b)]
        if len(s) < 2:
            return None
        return {'from_s': round(s[0][0], 1), 'to_s': round(s[-1][0], 1),
                'peak_working_set_mb': round(max(x[1] for x in s) / 1048576, 1),
                'peak_private_mb': round(max(x[2] for x in s) / 1048576, 1),
                'processor_s': round(s[-1][5] - s[0][5], 2), 'wall_s': round(s[-1][0] - s[0][0], 1)}
    whole = span(0, None)
    if whole and samples:
        whole['os_peak_working_set_mb'] = round(samples[-1][3] / 1048576, 1)
        whole['os_peak_private_mb'] = round(samples[-1][4] / 1048576, 1)
    return {'whole_run': whole, 'scripted_minute': span(lo, hi) if lo is not None else None}


def port_exe():
    root = os.getcwd()
    return os.path.join(root, 'build', 'native', 'waterbugs.exe'), os.path.join(root, 'assets', 'waterbugs')


def start_port(d):
    import argparse
    import drive
    exe, gamedir = port_exe()
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(os.path.join(d, 'user'))
    with open(os.path.join(d, 'user', 'cp.cfg'), 'w', newline='\n') as f:
        f.write(CFG)
    a = argparse.Namespace(exe=exe, gamedir=gamedir, userdir=os.path.join(d, 'user'), out=d, fast=False, exe_args=[])
    return drive.Game(a)


def click(g, x, y):
    for line in ('down game %d %d left' % (x, y), 'wait 3', 'up game %d %d left' % (x, y), 'wait 3'):
        g.cmd(line)


def port_run(out, runs, minute):
    results = []
    for i in range(runs):
        d = os.path.join(out, ('minute%d' if minute else 'load%d') % i)
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        g = start_port(d)
        h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, g.proc.pid)
        created = creation_time(h)
        samples = []

        def take():
            s = sample(h)
            if s:
                samples.append((round(time.time() - created, 3),) + s)
            return s

        while True:
            st = g.state()
            if st['screen'] == 'menu':
                ready = time.time() - created
                break
            time.sleep(0.002)
        at_menu = take()
        r = {'load_s': round(ready, 3), 'working_set_mb_at_menu': round(at_menu[0] / 1048576, 1),
             'private_mb_at_menu': round(at_menu[1] / 1048576, 1), 'processor_s_at_menu': round(at_menu[4], 2)}
        lo = hi = None
        if minute:
            for x, y in CLICKS:
                g.cmd('wait 100')
                click(g, x, y)
                take()
            while True:
                st = g.state()
                if st['screen'] == 'board' and st['level'] == 1:
                    break
                g.cmd('wait 10')
            start = time.time()
            lo, hi = start - created + BOARD_AT, start - created + BOARD_AT + MINUTE
            f0 = f1 = None
            while time.time() - created < hi + 0.2:
                take()
                now = time.time() - created
                if f0 is None and now >= lo:
                    f0 = g.state()['frame']
                    f0_time = now
                time.sleep(0.1)
            f1 = g.state()['frame']
            r.update({'steps_in_minute': f1 - f0, 'steps_per_s': round((f1 - f0) / (time.time() - created - f0_time), 0)})
        s1 = system_times()
        take()
        r.update(summarise(samples, lo, hi))
        r.update({'machine_busy': round(busy_fraction(s0, s1), 2), 'others_start': len(others0),
                  'others_end': len(game_processes(mine_prefixes()))})
        json.dump({'samples': samples, 'result': r}, open(os.path.join(d, 'perf.json'), 'w'))
        k32.CloseHandle(h)
        g.close()
        results.append(r)
        print(json.dumps(r), flush=True)
    if not minute:
        t = [r['load_s'] for r in results]
        print(json.dumps({'port_load_s': t, 'median_s': round(statistics.median(t), 3)}))


# The original's current level (gLevels.current at 0x473138): its kind is at +0x08 (14 the logo, 1 the main menu), its
# number at +0x1b8; its step counter gFrame is at 0x460a1c
LEVELS, KIND, NUMBER, FRAME = 0x473138, 0x08, 0x1b8, 0x460a1c
SCRIPT = ('at 2600 click %d %d 5\nat 3600 click %d %d 5\nat 5000 click %d %d 5\n' % tuple(v for c in CLICKS for v in c))


def stage_copy(root):
    """The copy under C:/byo-og/wbperf, windowed and on Windows' own DirectDraw."""
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    shutil.rmtree('C:/byo-og/wbperf', ignore_errors=True)
    shutil.rmtree(os.path.join(root, 'build', 'origrun', 'wbperf'), ignore_errors=True)
    subprocess.run([origrun, '--game', 'waterbugs', '--run', 'wbperf', '--stage-root', 'C:/byo-og', '--copy-only'],
                   capture_output=True)
    game = 'C:/byo-og/wbperf/game'
    with open(game + '/cp.cfg', 'w', newline='\n') as f:
        f.write(CFG)
    for name in ('DDraw.dll', 'D3DImm.dll', 'dgVoodoo.conf'):
        if os.path.exists(game + '/' + name):
            os.remove(game + '/' + name)


def orig_run(out, runs, minute):
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    results = []
    for i in range(runs):
        d = os.path.join(out, ('orig-minute%d' if minute else 'orig-load%d') % i)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        stage_copy(root)
        sc = os.path.join(d, 'script.txt')
        with open(sc, 'w', newline='\n') as f:
            f.write(SCRIPT if minute else 'wait 600000\n')
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        log = open(os.path.join(d, 'origrun.log'), 'wb')
        p = subprocess.Popen([origrun, '--game', 'waterbugs', '--run', 'wbperf', '--stage-root', 'C:/byo-og', '--script', sc,
                              '--counter', '0x460a1c', '--update-ms', '1', '--cursor', 'scripted', '--clock', 'real',
                              '--timeout', '300'], stdout=log, stderr=subprocess.STDOUT)
        h = None
        created = ready = board = None
        saw_logo = False
        samples = []
        f0 = f1 = None
        lo = hi = None
        r = {}
        next_sample = 0
        deadline = time.time() + 320
        while p.poll() is None and time.time() < deadline:
            if h is None:
                pid = find_process('WaterBugs.exe', '\\byo-og\\wbperf\\')
                if pid:
                    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED | TERMINATE, False, pid)
                    created = creation_time(h)
            if h:
                now = time.time()
                lv = read32(h, LEVELS)
                kind = read32(h, lv + KIND) if lv else None
                if kind == 14:
                    saw_logo = True
                if ready is None and saw_logo and kind == 1:
                    ready = now - created
                    m = sample(h)
                    r = {'load_s': round(ready, 3), 'working_set_mb_at_menu': round(m[0] / 1048576, 1),
                         'private_mb_at_menu': round(m[1] / 1048576, 1), 'processor_s_at_menu': round(m[4], 2)}
                    if not minute:
                        break
                if minute and ready is not None and board is None and kind == 0 and lv and read32(h, lv + NUMBER) == 1:
                    board = now - created
                    lo, hi = board + BOARD_AT, board + BOARD_AT + MINUTE
                if now >= next_sample:
                    s = sample(h)
                    if s:
                        samples.append((round(now - created, 3),) + s)
                    next_sample = now + 0.1
                    if lo is not None:
                        if f0 is None and now - created >= lo:
                            f0, f0_time = read32(h, FRAME), now - created
                        if now - created >= hi:
                            f1 = read32(h, FRAME)
                            r['steps_in_minute'] = f1 - f0
                            r['steps_per_s'] = round((f1 - f0) / (now - created - f0_time), 0)
                            break
            time.sleep(0.004 if ready is None else 0.02)
        s1 = system_times()
        if h:
            k32.TerminateProcess(h, 0)
            k32.CloseHandle(h)
        try:
            p.wait(30)
        except Exception:
            p.kill()
        if not r:
            r = {'error': 'the main menu was not reached'}
        r.update(summarise(samples, lo, hi))
        r.update({'machine_busy': round(busy_fraction(s0, s1), 2), 'others_start': len(others0),
                  'others_end': len(game_processes(mine_prefixes()))})
        json.dump({'samples': samples, 'result': r}, open(os.path.join(d, 'perf.json'), 'w'))
        results.append(r)
        print(json.dumps(r), flush=True)
    if not minute:
        t = [r['load_s'] for r in results if 'load_s' in r]
        if t:
            print(json.dumps({'orig_load_s': t, 'median_s': round(statistics.median(t), 3)}))


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(2)
    what, out = sys.argv[1], os.path.abspath(sys.argv[2])
    runs = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    if what == 'port-load':
        port_run(out, runs, False)
    elif what == 'port-minute':
        port_run(out, runs, True)
    elif what == 'orig-load':
        orig_run(out, runs, False)
    elif what == 'orig-minute':
        orig_run(out, runs, True)
    else:
        print(__doc__)
        sys.exit(2)


if __name__ == '__main__':
    main()
