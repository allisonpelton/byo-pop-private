"""Load time, memory and processor time of Word Harmony, the port and the original, for README.md "Performance" (the
measuring of build/slots/news.md, 2026-11-08; the sampling is Mummy Maze's tools/profile/perf.py).

From the worktree root:

    build/native/rundesk.exe <python.exe> games/wordharmony/tools/profile/perf.py port-load <out dir> [runs]
    build/native/rundesk.exe <python.exe> games/wordharmony/tools/profile/perf.py port-minute <out dir> build/ph2/tour2/port.txt
    <python.exe> games/wordharmony/tools/profile/perf.py orig <out dir> build/ph2/tour2/orig.txt [runs] [load]
                                                     (not under rundesk: origrun puts the game there; "load" stops at the menu)

port-load starts the port as a player gets it (no -splashhold, no -fast) and times the process's creation to its first
`state` answer with the main menu. port-minute plays tour 2 (games/wordharmony/tools/shots/tour2.txt, made into the
port's script by make.py) in real time with -splashhold, so that its frames are the original's (the script counts
frames; each goes on the update real time runs it on, to_update). orig runs the original
under origrun on its real clock with the same tour, from a fresh copy of the staged game with phase one's patches, and
reads its state (the menu) and its update counter out of its memory. Both sample the game's process every 0.1 s: working
set, private bytes, processor time (kernel + user). Each run prints one JSON line: the menu's time, and the peaks and the
processor time over the whole run and over the scripted minute (frames 2000 to 3800 of the TSUNAMI game), the machine's
busy fraction during the run, and the game processes of other sessions when it started and ended.
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

QUERY_LIMITED, QUERY, VM_READ = 0x1000, 0x400, 0x10
EPOCH_DELTA = 11644473600  # seconds from 1601 to 1970


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


def summarise(samples, lo, hi=None):
    """samples: (t, ws, private, peak ws, peak private, cpu) with t in seconds from the process's creation; the window of the
    scripted minute is lo..hi (to the end when hi is None)."""
    def span(a, b):
        s = [x for x in samples if x[0] >= a and (b is None or x[0] <= b)]
        if len(s) < 2:
            return None
        return {'from_s': round(s[0][0], 1), 'to_s': round(s[-1][0], 1),
                'peak_working_set_mb': round(max(x[1] for x in s) / 1048576, 1),
                'peak_private_mb': round(max(x[2] for x in s) / 1048576, 1),
                'processor_s': round(s[-1][5] - s[0][5], 2), 'wall_s': round(s[-1][0] - s[0][0], 1)}
    whole = span(0, hi)
    if whole and samples:
        last = [x for x in samples if hi is None or x[0] <= hi][-1]
        whole['os_peak_working_set_mb'] = round(last[3] / 1048576, 1)
        whole['os_peak_private_mb'] = round(last[4] / 1048576, 1)
    return {'whole_run': whole, 'scripted_minute': span(lo, hi)}


ntdll = ctypes.WinDLL('ntdll')
k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.GetThreadDescription.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_wchar_p)]
k32.GetThreadDescription.restype = ctypes.c_long
ntdll.NtQueryInformationThread.argtypes = [W.HANDLE, ctypes.c_ulong, ctypes.c_void_p, ctypes.c_ulong, ctypes.c_void_p]
psapi.EnumProcessModulesEx.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD, ctypes.POINTER(W.DWORD), W.DWORD]
psapi.GetModuleBaseNameW.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_wchar_p, W.DWORD]
psapi.GetModuleInformation.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, W.DWORD]


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


class ThreadSampler:
    """Per-thread cycle counters (QueryThreadCycleTime) of a process, sampled with the process: the machine's load moves a
    thread's seconds but not its cycles. The thread list is refreshed every second (new threads are opened, their start
    address and name read; a thread that has ended keeps its last count). Used for the split by thread that the target of
    docs/COORDINATION.md asks for, since sound and the graphics driver run on threads of their own."""

    def __init__(self, pid, process):
        self.pid, self.process = pid, process
        self.handles, self.info, self.last_refresh = {}, {}, -1.0
        self.modules = []
        self.samples = []      # (t, {tid: cycles})
        self.last = {}

    def refresh(self):
        snap = k32.CreateToolhelp32Snapshot(4, 0)
        te = THREADENTRY32()
        te.dwSize = ctypes.sizeof(THREADENTRY32)
        tids = []
        ok = k32.Thread32First(snap, ctypes.byref(te))
        while ok:
            if te.th32OwnerProcessID == self.pid:
                tids.append(te.th32ThreadID)
            ok = k32.Thread32Next(snap, ctypes.byref(te))
        k32.CloseHandle(snap)
        for tid in tids:
            if tid in self.handles:
                continue
            h = k32.OpenThread(0x40 | 0x800, False, tid)   # QUERY_INFORMATION | QUERY_LIMITED_INFORMATION
            if not h:
                continue
            self.handles[tid] = h
            start = ctypes.c_void_p(0)
            ntdll.NtQueryInformationThread(h, 9, ctypes.byref(start), ctypes.sizeof(start), None)
            desc = ctypes.c_wchar_p()
            name = ''
            if k32.GetThreadDescription(h, ctypes.byref(desc)) >= 0 and desc.value:
                name = desc.value
            self.info[tid] = {'start': start.value or 0, 'name': name}
        mods = (ctypes.c_void_p * 1024)()
        need = W.DWORD()
        if psapi.EnumProcessModulesEx(self.process, mods, ctypes.sizeof(mods), ctypes.byref(need), 3):
            found = []
            for i in range(min(need.value // ctypes.sizeof(ctypes.c_void_p), 1024)):
                nm = ctypes.create_unicode_buffer(260)
                mi = (ctypes.c_size_t * 3)()    # base, size, entry point
                if psapi.GetModuleBaseNameW(self.process, mods[i], nm, 260) and psapi.GetModuleInformation(self.process, mods[i], mi, 24):
                    found.append((mi[0], mi[1], nm.value))
            if found:
                self.modules = found

    def sample(self, t):
        if t - self.last_refresh >= 1.0:
            self.last_refresh = t
            self.refresh()
        row = {}
        for tid, h in self.handles.items():
            c = ctypes.c_uint64()
            if k32.QueryThreadCycleTime(h, ctypes.byref(c)):
                row[tid] = c.value
            elif tid in self.last:      # a thread that has ended: its count stays where it was
                row[tid] = self.last[tid]
        self.last = row
        self.samples.append((round(t, 3), row))

    def where(self, tid):
        a = self.info.get(tid, {}).get('start', 0)
        for base, size, nm in self.modules:
            if base <= a < base + size:
                return '%s+0x%x' % (nm, a - base)
        return '0x%x' % a

    def summary(self, lo, hi):
        """Cycles of each thread between the samples at lo and hi (seconds from the process's creation), biggest first."""
        s = [x for x in self.samples if x[0] >= lo and (hi is None or x[0] <= hi)]
        if len(s) < 2:
            return []
        a, b = s[0][1], s[-1][1]
        out = []
        for tid in b:
            d = b[tid] - a.get(tid, 0)
            out.append({'tid': tid, 'start': self.where(tid), 'name': self.info.get(tid, {}).get('name', ''), 'cycles': d})
        total = sum(o['cycles'] for o in out) or 1
        for o in out:
            o['share'] = round(o['cycles'] / total, 4)
        out.sort(key=lambda o: -o['cycles'])
        return [o for o in out if o['share'] >= 0.001]


def add_cycles(res, threads):
    """The processor time that counts (docs/COORDINATION.md, "The target"): the cycles of all the process's threads over the
    scripted minute, in billions, from QueryThreadCycleTime. The seconds of GetProcessTimes ('processor_s') are charged by sampling
    at the 15.6 ms clock tick, so they are left in the line only to compare with the figures of before."""
    res['cycles_in_minute_g'] = round(sum(t['cycles'] for t in threads) / 1e9, 3)
    res['threads_in_minute'] = [{'start': t['start'], 'name': t['name'], 'g': round(t['cycles'] / 1e9, 3)} for t in threads]


def mine_prefixes():
    return [os.getcwd() + '\\']


def strip_script(src, dst, drop):
    with open(src) as f, open(dst, 'w', newline='\n') as g:
        for line in f:
            if not any(d in line for d in drop):
                g.write(line)
    return dst


# The scripted minute: 1800 updates (60 s at the game's 30 updates a second) of tour 2's TSUNAMI game, from its update
# 2000 to 3800 (the port's numbering; the original's script runs one update earlier, "at K-1")
MINUTE_FROM, MINUTE_TO = 2000, 3800
# WH_PERF_WINDOWED=1 runs both sides windowed: the game's "Full screen" setting off (it is on by default), in the port's registry.ini
# and in the original's registry key (which every run clears first anyway).
# WH_PERF_PORTARGS="-windowscale=1" gives the port's run more arguments (a test option: the original's window size).
# WH_PERF_FRAMES=<from>:<to> measures a shorter stretch of the same game (the port's script then ends at <to> + 60, for the
# experiments that compare builds; the original's run always ends at the minute's end)
if os.environ.get('WH_PERF_FRAMES'):
    MINUTE_FROM, MINUTE_TO = (int(x) for x in os.environ['WH_PERF_FRAMES'].split(':'))
# The original: the game is the singleton at 0x4995b8, its state at +0x1d8 (1: the main menu); the update counter
# (phase one's patched frame count) at 0x4aa600
GAME_PTR, STATE_OFF, UPDATE_COUNT = 0x4995B8, 0x1D8, 0x4AA600
TERMINATE = 0x1


def port_exe():
    root = os.getcwd()
    return (os.path.join(root, 'build', 'native', os.environ.get('WH_PERF_EXE', 'wordharmony.exe')), os.path.join(root, 'assets', 'wordharmony'))


def port_load(out, runs):
    """The port as shipped (no -splashhold, no -fast): from the process's creation to its first `state` answer with the
    main menu, asked every update by a script (the menu's own first frame)."""
    out = os.path.abspath(out)
    exe, gamedir = port_exe()
    results = []
    for i in range(runs):
        d = os.path.join(out, 'load%d' % i)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(os.path.join(d, 'user'))
        sc = os.path.join(d, 'script.txt')
        with open(sc, 'w', newline='\n') as f:
            for n in range(1, 600):
                f.write('%d state\n' % n)
            f.write('600 quit\n')
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        p = subprocess.Popen([exe, '-gamedir=' + gamedir, '-userdir=' + os.path.join(d, 'user'), '-nomods', '-script=' + sc],
                             cwd=d, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
        created = creation_time(h)
        seen = m = None
        for line in p.stdout:
            if line.startswith(b'STATE ') and b'"screen":"menu"' in line:
                seen = time.time()
                m = sample(h)
                break
        s1 = system_times()
        others1 = game_processes(mine_prefixes())
        p.kill()
        p.wait()
        k32.CloseHandle(h)
        r = {'load_s': round(seen - created, 3) if seen else None,
             'working_set_mb_at_menu': round(m[0] / 1048576, 1) if m else None,
             'private_mb_at_menu': round(m[1] / 1048576, 1) if m else None,
             'processor_s_at_menu': round(m[4], 2) if m else None,
             'machine_busy': round(busy_fraction(s0, s1), 2), 'others_start': len(others0), 'others_end': len(others1)}
        results.append(r)
        print(json.dumps(r), flush=True)
    t = [r['load_s'] for r in results if r['load_s'] is not None]
    if t:
        print(json.dumps({'port_load_s': t, 'median_s': round(statistics.median(t), 3)}))


def to_update(frame):
    return -(-frame * 33 // 10)


def port_minute(out, script):
    """Tour 2 in real time with -splashhold (so its updates are the original's), sampled every 0.1 s; screenshots at the
    minute's first and last update mark its times."""
    out, script = os.path.abspath(out), os.path.abspath(script)
    exe, gamedir = port_exe()
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    if os.environ.get('WH_PERF_WINDOWED'):
        # the game's own "Full screen" setting off (it starts full screen by default): the port's window, as the original's
        with open(os.path.join(out, 'user', 'registry.ini'), 'w', newline='\n') as f:
            f.write('[SOFTWARE\\GameHouse\\WordHarmony]\nFullScreen=dword:0\n')
    # tour 2's script counts the game's frames (the port's -fast runs a frame an update); in real time the framework
    # updates every 10 ms and a 33 ms frame runs on every third or fourth (WordHarmonyWidget::Update), so frame k falls
    # on update ceil(3.3 k)
    frames = strip_script(script, os.path.join(out, 'frames.txt'), ['screenshot'])
    script = os.path.join(out, 'script.txt')
    with open(frames) as f, open(script, 'w', newline='\n') as g:
        for line in f:
            n, rest = line.split(' ', 1)
            if os.environ.get('WH_PERF_FRAMES') and int(n) > MINUTE_TO + 60:
                continue
            g.write('%d %s' % (to_update(int(n)), rest))
    marks = [os.path.join(out, 'mark0.png'), os.path.join(out, 'mark1.png')]
    with open(script, 'a', newline='\n') as f:
        f.write('%d screenshot %s\n' % (to_update(MINUTE_FROM), marks[0].replace('\\', '/')))
        f.write('%d screenshot %s\n' % (to_update(MINUTE_TO), marks[1].replace('\\', '/')))
        if os.environ.get('WH_PERF_FRAMES'):
            f.write('%d quit\n' % to_update(MINUTE_TO + 60))
    others0 = game_processes(mine_prefixes())
    s0 = system_times()
    log = open(os.path.join(out, 'game.log'), 'wb')
    p = subprocess.Popen([exe, '-gamedir=' + gamedir, '-userdir=' + os.path.join(out, 'user'), '-nomods', '-splashhold',
                          '-seed=1', '-script=' + script] + os.environ.get('WH_PERF_PORTARGS', '').split(),
                         cwd=out, stdout=log, stderr=subprocess.STDOUT)
    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
    created = creation_time(h)
    samples = []
    ts = ThreadSampler(p.pid, h)
    while p.poll() is None:
        s = sample(h)
        if s:
            samples.append((round(time.time() - created, 3),) + s)
            ts.sample(time.time() - created)
        time.sleep(0.1)
    end = samples[-1][0] if samples else 0
    s1 = system_times()
    lo = os.path.getmtime(marks[0]) - created if os.path.exists(marks[0]) else None
    hi = os.path.getmtime(marks[1]) - created if os.path.exists(marks[1]) else None
    res = summarise(samples, lo if lo is not None else 0, hi)
    res.update({'exit': p.returncode, 'run_s': round(end, 1), 'machine_busy': round(busy_fraction(s0, s1), 2),
                'minute_marked': lo is not None and hi is not None,
                'frames_per_s_in_minute': round((MINUTE_TO - MINUTE_FROM) / (hi - lo), 1) if lo and hi else None,
                'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
    add_cycles(res, ts.summary(lo if lo is not None else 0, hi))
    json.dump({'samples': samples, 'result': res}, open(os.path.join(out, 'perf.json'), 'w'))
    k32.CloseHandle(h)
    print(json.dumps(res), flush=True)


def orig_run(out, script, runs, load_only=False):
    """The original under origrun on its real clock, from a fresh copy of the staged game (phase one's patches: the
    update counter, and srand taking the run's seed), the registry key cleared first, as tour 2's runs had it."""
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    staged = 'C:/byo-og/wh/game'
    run = 'whperf'
    results = []
    for i in range(runs):
        d = os.path.join(out, 'orig%d' % i)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        sc = strip_script(script, os.path.join(d, 'script.txt'), [' shot '])
        copy = os.path.join(root, 'build', 'origrun', run, 'game')
        shutil.rmtree(os.path.dirname(copy), ignore_errors=True)
        shutil.copytree(staged, copy)
        subprocess.run(['reg', 'delete', 'HKCU\\Software\\GameHouse\\WordHarmony', '/f'], capture_output=True)
        if os.environ.get('WH_PERF_WINDOWED'):
            # the game's own "Full screen" setting off (it starts full screen by default): a window of 800x600
            subprocess.run(['reg', 'add', 'HKCU\\Software\\GameHouse\\WordHarmony', '/v', 'FullScreen',
                            '/t', 'REG_DWORD', '/d', '0', '/f'], capture_output=True)
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        log = open(os.path.join(d, 'origrun.log'), 'wb')
        t_start = time.time()
        p = subprocess.Popen([origrun, '--exe', os.path.join(copy, 'WordHarmonyGame.exe'), '--run', run, '--script', sc,
                              '--clock', 'real', '--cursor', 'scripted', '--counter', '0x4aa600', '--update-ms', '33',
                              '--seed', '1', '--patches', os.environ.get('WH_PERF_PATCHES', 'tools/origrun/patches/wordharmony.txt'), '--timeout', '420'],
                             stdout=log, stderr=subprocess.STDOUT)
        h = None
        ts = None
        created = ready = spawned = ready_mem = None
        samples, counters = [], []
        next_sample = 0
        done = False
        deadline = time.time() + 440
        while p.poll() is None and not done and time.time() < deadline:
            if load_only and ready is not None:
                break
            if h is None:
                pid = find_process('WordHarmonyGame.exe', '\\origrun\\' + run + '\\')
                if pid:
                    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED | TERMINATE, False, pid)
                    created = creation_time(h)
                    spawned = created - t_start
                    ts = ThreadSampler(pid, h)
            if h:
                now = time.time()
                if ready is None:
                    g = read32(h, GAME_PTR)
                    if g:
                        st = read32(h, g + STATE_OFF)
                        if st == 1:
                            ready = now - created
                            ready_mem = sample(h)
                if now >= next_sample:
                    s = sample(h)
                    c = read32(h, UPDATE_COUNT)
                    if s:
                        samples.append((round(now - created, 3),) + s)
                        counters.append((round(now - created, 3), c))
                        ts.sample(now - created)
                    next_sample = now + 0.1
                    if c is not None and c >= MINUTE_TO + 30:
                        done = True
            time.sleep(0.004 if ready is None else 0.02)
        if h:
            k32.TerminateProcess(h, 0)
        s1 = system_times()
        try:
            p.wait(30)
        except subprocess.TimeoutExpired:
            p.terminate()
        lo = next((t for t, c in counters if c is not None and c >= MINUTE_FROM - 1), None)
        hi = next((t for t, c in counters if c is not None and c >= MINUTE_TO - 1), None)
        end = samples[-1][0] if samples else 0
        res = summarise(samples, lo if lo is not None else 0, hi)
        if lo is not None and hi is not None:
            c_lo = next(c for t, c in counters if t == lo)
            c_hi = next(c for t, c in counters if t == hi)
            res['updates_in_minute'] = c_hi - c_lo
            res['updates_per_s_in_minute'] = round((c_hi - c_lo) / (hi - lo), 1)
        res.update({'exit': p.returncode, 'run_s': round(end, 1), 'machine_busy': round(busy_fraction(s0, s1), 2),
                    'load_s': round(ready, 3) if ready else None, 'origrun_to_process_s': round(spawned, 1) if h else None,
                    'last_update_reached': counters[-1][1] if counters else None,
                    'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
        if ts is not None:
            add_cycles(res, ts.summary(lo if lo is not None else 0, hi))
        if ready_mem:
            res.update({'working_set_mb_at_menu': round(ready_mem[0] / 1048576, 1),
                        'private_mb_at_menu': round(ready_mem[1] / 1048576, 1), 'processor_s_at_menu': round(ready_mem[4], 2)})
        json.dump({'samples': samples, 'counters': counters, 'result': res}, open(os.path.join(d, 'perf.json'), 'w'))
        if h:
            k32.CloseHandle(h)
        log.close()
        results.append(res)
        print(json.dumps(res), flush=True)


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else ''
    if mode == 'port-load':
        port_load(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 3)
    elif mode == 'port-minute':
        port_minute(sys.argv[2], sys.argv[3])
    elif mode == 'orig':
        orig_run(sys.argv[2], sys.argv[3], int(sys.argv[4]) if len(sys.argv) > 4 else 1, len(sys.argv) > 5 and sys.argv[5] == 'load')
    else:
        sys.exit(__doc__)
