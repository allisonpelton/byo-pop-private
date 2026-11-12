"""Load time, memory and processor time of AstroPop Deluxe, the port and the original, for GAME_NOTES.md "Performance".

Adapted from games/atomica/tools/profile/perf.py (its method: a sample of the process every 0.1 s, the time the title screen
can take input read out of the original's memory).

From the worktree root:

    build/native/rundesk.exe <python.exe> games/astropop/tools/profile/perf.py port-load <out dir> [runs]
    build/native/rundesk.exe <python.exe> games/astropop/tools/profile/perf.py port-minute <out dir>
    <python.exe> games/astropop/tools/profile/perf.py orig <out dir> [runs] [load]    (not under rundesk; "load" stops at the title)
    <python.exe> games/astropop/tools/profile/perf.py orig3d <out dir> [runs] [load]  (22:00 to 08:00: the original in 3D on the desktop)

Processor time is the sum of every thread's QueryThreadCycleTime (the coordinator's rule, 2026-11-11), in G cycles (`cycles_g`; the
nominal clock turns them into seconds); the port's SDL audio thread (its mixer) is counted apart as `audio_g`, and `cycles_check`
gives the process's own cycle count beside the threads' sum and the busiest threads. Run the alternating loop under
`tools/pincore.py` (`profile/pin2.py` for two cores; pairs.sh does it). Environment: PERF_3D=1 (the port's registry file says Is3D=1),
PERF_DESKTOP=1 (the port's window on the visible desktop, 22:00 to 08:00 only, no rundesk), PERF_ENV="K=V,K=V" (variables for the port's
process, e.g. SDL_RENDER_DRIVER=opengl), PERF_EXE (another exe in build/native), PERF_CLOCK=real. Each port result names the graphics
libraries it had loaded (`graphics_modules`: d3d9 or d3d11 or opengl) and the `display:` line of its log (`display`: client size, render
scale, windowed or full screen, renderer; the framework logs it, docs/TEST_COMMANDS.md "The window line").
PERF_WINDOW sets the port's window state: `1` (the default since 2026-11-12) is the game's own size, as the original's (registry
ScreenMode=0 and -windowscale=1); `2` is the framework's default window, twice the game's size (ScreenMode=0 only); `full` is what these
runs did before (no ScreenMode: the port started full screen on a 3840x2160 desktop, render scale 4: not like for like with the
original's 640x480 window). The original's window is its own size in every run (origrun's copy; visrun3d's, ScreenMode 0).
PERF_PRESENTLOG=1 adds -presentlog=<dir>/presents.txt (the present intervals, read by present_intervals.py; vsync.sh sets it). SDL_RENDER_VSYNC=0
in PERF_ENV turns the display's vsync off, so that the frame loop's own pace holds the presents to the refresh rate.
PERF_SPLIT=1 is the cycle split (docs/TEST_COMMANDS.md, "The cycle split"): the port runs with -cyclesplit=<dir>/split.jsonl and the
script's `cyclesplit a` at the stretch's first update and `cyclesplit b` 20 updates after the last input; orig counts the functions of count.txt with origrun's
--count (`at N count a` / `b`), orig3d with countsnap.py (the same counters read from outside); the files are compared with
`tools/cyclesplit.py <port file> <original file> --from a --to b`. A split run is for the proportions: the target's total comes from a
run without counting.

The scenario is scenario/classic11.json: the inputs (update, command) that origcmp/record.py's bot gave a classic game, seed 11,
played from the title (the click on the title's link at update 3000, the name Test, the mode, the pilot, then 60 moves). The port
plays them as -realinput lines, the original as origrun script lines (origcmp/to_origrun.py), so both play the same game.
"The stretch" is the 2000 updates before the last input, 20 seconds of play at the game's 100 updates a second.

port-load starts the port without -fast, with the live command channel (as the real-input driver does), and asks `state` until
it answers with the screen "title"; its time counts from the creation of the process. port-minute plays the scenario (cut by a
screenshot at the stretch's first update, which marks the time it began, and a quit); orig plays it in the original under origrun
on the host (scripted cursor, a fresh copy each run, the registry key wiped and Is3D=0 as the port's runs have it). With
PERF_CLOCK=real both play in real time (the port without -fast, the original on origrun's real clock); without it neither waits
(the port's -fast, origrun's virtual clock), so the processor time is the work done. `orig ... load` stops when the loading thread
has completed. orig starts origrun directly: origrun puts the game under rundesk itself.

Both sample the game's process every 0.1 s: working set, private bytes (and the system's own peaks of both), processor time
(kernel + user). Each run prints one JSON line with the peaks and the processor time over the whole run and over the stretch, the
machine's busy fraction during the run, and the game processes of other sessions running when it started and ended. The
original's run is stopped from here once its update counter ([0x5dc220] + 0x4a0) passes the script's last update, because its
`quit` does not end the game; a run counts only if the title's click landed (`game_started`: the board pointer, +0x628 of the app,
is set), since the original loads in a thread in real time and on a busy machine the click can come before the title is up.
pairs.sh runs the two turn about.
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
sys.path.insert(0, os.path.join(HERE, '..', 'origcmp'))
sys.path.insert(0, os.path.join(HERE, '..', '..', '..', '..', 'tools', 'playbymove'))

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

SEED = 11
MINUTE = 2000   # updates of the stretch
REAL = os.environ.get('PERF_CLOCK') == 'real'
THREE_D = os.environ.get('PERF_3D') == '1'          # the port in 3D (the accelerated renderer); the original in 3D is `orig3d`, on the desktop
DESKTOP = os.environ.get('PERF_DESKTOP') == '1'     # the port on the user's visible desktop (22:00 to 08:00 only), as a player runs it
WINDOW = os.environ.get('PERF_WINDOW', '1')         # the port's window: '1' the game's own size, '2' the default window, 'full' as before 2026-11-12
SPLIT = os.environ.get('PERF_SPLIT') == '1'         # the cycle split: -cyclesplit, --count, countsnap (see the docstring)
COUNT_LIST = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'count.txt')
APP = 0x5DC220                 # the original's app object: a static pointer to it
COUNTER, LOADING, BOARD = 0x4A0, 0x50C, 0x628   # offsets in the app: the update counter, the loading started and completed bytes, the board
SCENARIO = os.path.join(HERE, 'scenario', 'classic11.json')
RUN = 'apc'                    # build/origrun/apc: the staged copy of the original (origcmp/stage_original.sh)


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


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.GetThreadDescription.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_wchar_p)]
k32.LocalFree.argtypes = [ctypes.c_void_p]
k32.QueryProcessCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.GetProcessId.argtypes = [W.HANDLE]
k32.Thread32First.argtypes = k32.Thread32Next.argtypes = [W.HANDLE, ctypes.c_void_p]
THREAD_QUERY_LIMITED = 0x800


def nominal_hz():
    """The processor's nominal clock, to turn cycles into seconds (the cycle counts are a reference clock's: constant-rate)."""
    import winreg
    with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'HARDWARE\DESCRIPTION\System\CentralProcessor\0') as key:
        return winreg.QueryValueEx(key, '~MHz')[0] * 1e6


HZ = nominal_hz()


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


class Cycles:
    """The cycles the threads of one process have used, summed (QueryThreadCycleTime: exact; the coordinator's rule, 2026-11-11: GetProcessTimes
    and GetThreadTimes move in 15.6 ms scheduler ticks). Each thread's handle is kept, so a thread that has ended still gives its last count;
    one that lived and died between two looks (the thread list is read at every look) would be missed, so `process()` (QueryProcessCycleTime,
    which keeps the ended threads' cycles) is read beside it as a check."""

    def __init__(self, h):
        self.pid = k32.GetProcessId(h)
        self.created = creation_time(h)
        self.proc = h
        self.handles = {}
        self.names = {}

    def threads(self):
        return self.split()[0]

    def description(self, tid):
        """The thread's name (SDL names its threads, an audio device's "SDLAudioDev..."), read once; '' when it has none."""
        if tid not in self.names:
            text = ctypes.c_wchar_p()
            name = ''
            if k32.GetThreadDescription(self.handles[tid], ctypes.byref(text)) >= 0 and text.value:
                name = text.value
            if text:
                k32.LocalFree(ctypes.cast(text, ctypes.c_void_p))
            self.names[tid] = name
        return self.names[tid]

    def split(self):
        """(all threads' cycles, the audio threads' cycles, {thread id: (name, cycles)}): the port's mixer runs in SDL's audio thread, which
        the original's runs under origrun's stand-in do not have, so the audio threads' share is counted apart (the coordinator's rule)."""
        total = audio = 0
        each = {}
        for tid in thread_ids(self.pid):
            if tid not in self.handles:
                self.handles[tid] = k32.OpenThread(THREAD_QUERY_LIMITED, False, tid)
        for tid, hh in self.handles.items():
            c = ctypes.c_uint64()
            if hh and k32.QueryThreadCycleTime(hh, ctypes.byref(c)):
                total += c.value
                name = self.description(tid)
                each[tid] = (name, c.value)
                if name.startswith('SDLAudio'):
                    audio += c.value
        return total, audio, each

    def process(self):
        c = ctypes.c_uint64()
        k32.QueryProcessCycleTime(self.proc, ctypes.byref(c))
        return c.value

    def close(self):
        for hh in self.handles.values():
            if hh:
                k32.CloseHandle(hh)
        self.handles = {}


CYCLES = {}      # pid -> Cycles of the process sampled last under that pid


def cycles_of(h):
    pid = k32.GetProcessId(h)
    cy = CYCLES.get(pid)
    if cy is None or cy.created != creation_time(h):
        if cy:
            cy.close()
        cy = CYCLES[pid] = Cycles(h)
    return cy


def cycle_check(h):
    """The whole run's cycles two ways, in G: the threads' sum (what the samples count) and the process's own total (it keeps the threads that ended)."""
    cy = cycles_of(h)
    total, audio, each = cy.split()
    top = sorted(each.items(), key=lambda kv: -kv[1][1])[:6]
    return {'threads_g': round(total / 1e9, 2), 'process_g': round(cy.process() / 1e9, 2), 'audio_g': round(audio / 1e9, 2),
            'top_threads': [[tid, name, round(c / 1e9, 2)] for tid, (name, c) in top]}


def sample(h):
    """(working set, private bytes, peak working set, peak private, processor seconds, audio-thread processor seconds) of the process, or
    None. The processor seconds are the threads' cycle counts summed over the nominal clock."""
    m = PMC()
    m.cb = ctypes.sizeof(PMC)
    if not psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb):
        return None
    total, audio, _ = cycles_of(h).split()
    return (m.WorkingSetSize, m.PrivateUsage, m.PeakWorkingSetSize, m.PeakPagefileUsage, total / HZ, audio / HZ)


def graphics_modules(h):
    """Which graphics libraries the process has loaded (the renderer SDL chose: Direct3D 9, 11 or OpenGL)."""
    arr = (ctypes.c_void_p * 2048)()
    need = W.DWORD()
    psapi.EnumProcessModulesEx(h, arr, ctypes.sizeof(arr), ctypes.byref(need), 3)
    names = set()
    for i in range(need.value // ctypes.sizeof(ctypes.c_void_p)):
        nm = ctypes.create_unicode_buffer(520)
        psapi.GetModuleFileNameExW(h, ctypes.c_void_p(arr[i]), nm, 520)
        base = os.path.basename(nm.value).lower()
        if base in ('d3d9.dll', 'd3d11.dll', 'opengl32.dll', 'nvd3dum.dll', 'nvwgf2um.dll', 'nvoglv32.dll', 'dxgi.dll', 'd3d12.dll'):
            names.add(base)
    return sorted(names)


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


TOOLS = {'rundesk.exe', 'lsprobe.exe', 'origrun.exe', 'bejeweled_verify.exe', 'python.exe', 'node.exe'}


def game_processes(mine):
    """The game processes of other sessions: every exe in a byo-pop folder or a staged original, except tools and `mine`."""
    out = []
    for pid, name in processes():
        if name.lower() in TOOLS:
            continue
        path = image_path(pid).lower()
        if '\\byo-pop\\' in path and not any(path.startswith(m.lower()) for m in mine):
            out.append(name)
    return sorted(out)


def mine_prefixes():
    return [os.getcwd() + '\\']


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
    """samples: (t, ws, private, peak ws, peak private, cpu) with t in seconds from the process's creation; the stretch is
    lo..hi (to the end when hi is None)."""
    def span(a, b):
        s = [x for x in samples if x[0] >= a and (b is None or x[0] <= b)]
        if len(s) < 2:
            return None
        return {'from_s': round(s[0][0], 1), 'to_s': round(s[-1][0], 1),
                'peak_working_set_mb': round(max(x[1] for x in s) / 1048576, 1),
                'peak_private_mb': round(max(x[2] for x in s) / 1048576, 1),
                'processor_s': round(s[-1][5] - s[0][5], 2), 'cycles_g': round((s[-1][5] - s[0][5]) * HZ / 1e9, 2), 'wall_s': round(s[-1][0] - s[0][0], 1),
                'audio_g': round((s[-1][6] - s[0][6]) * HZ / 1e9, 2) if len(s[0]) > 6 else None}
    whole = span(0, hi)
    if whole and samples:
        last = [x for x in samples if hi is None or x[0] <= hi][-1]
        whole['os_peak_working_set_mb'] = round(last[3] / 1048576, 1)
        whole['os_peak_private_mb'] = round(last[4] / 1048576, 1)
    return {'whole_run': whole, 'scripted_stretch': span(lo, hi)}


def scenario():
    return json.load(open(SCENARIO))


def port_script(dst, mark):
    """The scenario as -realinput lines (an update, a command), a screenshot at the stretch's first update and a quit after the end."""
    sc = scenario()
    with open(dst, 'w', newline='\n') as f:
        for u, cmd in sc['events']:
            f.write('%d %s\n' % (u, cmd))
        f.write('%d screenshot %s\n' % (sc['last'] - MINUTE, mark.replace('\\', '/')))
        if SPLIT:
            f.write('%d cyclesplit a\n' % (sc['last'] - MINUTE))
            f.write('%d cyclesplit b\n' % (sc['last'] + 20))
        f.write('%d quit\n' % (sc['last'] + 30))
    return sc['last']


def night_only():
    """The visible desktop is for 22:00 to 08:00 (CLAUDE.md, "Overnight, desktop and sound allowed")."""
    import datetime
    now = datetime.datetime.now()
    if 8 <= now.hour < 22:
        raise SystemExit('perf: the visible desktop is for 22:00 to 08:00; it is %s' % now.strftime('%H:%M'))


def port_exe():
    # PERF_EXE names another build of the port in build/native (an exe kept from before a change)
    return os.path.join(os.getcwd(), 'build', 'native', os.environ.get('PERF_EXE', 'astropop.exe'))


def port_user(d):
    os.makedirs(os.path.join(d, 'user'), exist_ok=True)
    with open(os.path.join(d, 'user', 'registry.ini'), 'w', newline='\n') as f:   # 2D, as the original runs in these comparisons (PERF_3D=1: 3D)
        f.write('[SOFTWARE\\PopCap\\AstroPop]\nIs3D=dword:%d\n' % (1 if THREE_D else 0))
        if WINDOW != 'full':       # windowed: with no ScreenMode the port starts full screen (render scale 4 on a 3840x2160 desktop)
            f.write('ScreenMode=dword:0\n')
    return os.path.join(d, 'user')


def display_line(log_path):
    """The last `display:` line of the port's log (client size, render scale, windowed or full screen, renderer), or None."""
    try:
        lines = [l.strip() for l in open(log_path, errors='replace') if 'display:' in l]
    except OSError:
        return None
    return lines[-1] if lines else None


def window_args():
    """-windowscale=1 (the game's own size, the original's) for PERF_WINDOW=1; the default window (2x) and full screen take no option."""
    return ['-windowscale=1'] if WINDOW == '1' else []


def port_load(out, runs):
    import argparse
    import playbymove
    exe = port_exe()
    results = []
    for i in range(runs):
        d = os.path.abspath(os.path.join(out, 'load%d' % i))
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        a = argparse.Namespace(dir=d, timeout=120, exe=exe, realtime=True, game_args=['-seed=%d' % SEED, '-userdir=' + port_user(d)] + window_args())
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        if DESKTOP:        # no rundesk: the game opens its window on the desktop, with SDL's silent audio driver
            night_only()
            os.environ['BYOPOP_TEST_RUN'] = '1'
            os.environ['SDL_AUDIODRIVER'] = 'dummy'
        g = playbymove.Game(a)
        pid = g.proc.pid
        if not image_path(pid).lower().endswith('astropop.exe'):     # started through rundesk: its child is the game
            pid = find_process('astropop.exe', os.getcwd() + '\\build\\native') or pid
        h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, pid)
        created = creation_time(h)
        while True:
            answer = g.send('state')[0]
            if '"screen":"title"' in answer:
                seen = time.time()
                break
        m = sample(h)
        s1 = system_times()
        others1 = game_processes(mine_prefixes())
        k32.CloseHandle(h)
        g.finish(None)
        r = {'load_s': round(seen - created, 3), 'working_set_mb_at_title': round(m[0] / 1048576, 1),
             'private_mb_at_title': round(m[1] / 1048576, 1), 'processor_s_at_title': round(m[4], 2),
             'machine_busy': round(busy_fraction(s0, s1), 2), 'others_start': len(others0), 'others_end': len(others1)}
        results.append(r)
        print(json.dumps(r), flush=True)
    t = [r['load_s'] for r in results]
    print(json.dumps({'port_load_s': t, 'median_s': round(statistics.median(t), 3), 'others_start': 0, 'others_end': 0}))


def port_minute(out):
    exe = port_exe()
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    user = port_user(out)
    mark = os.path.join(out, 'mark.png')
    last = port_script(os.path.join(out, 'script.txt'), mark)
    others0 = game_processes(mine_prefixes())
    s0 = system_times()
    log = open(os.path.join(out, 'game.log'), 'wb')
    extra = {}
    if DESKTOP:     # as a player runs it: a window on the desktop that takes no focus, below-normal priority, SDL's silent audio driver
        night_only()
        si = subprocess.STARTUPINFO()
        si.dwFlags = 1            # STARTF_USESHOWWINDOW
        si.wShowWindow = 4        # SW_SHOWNOACTIVATE
        env = dict(os.environ, SDL_AUDIODRIVER='dummy', BYOPOP_TEST_RUN='1')
        for kv in filter(None, os.environ.get('PERF_ENV', '').split(',')):     # PERF_ENV="SDL_RENDER_DRIVER=opengl,...": an experiment's variables
            env[kv.split('=', 1)[0]] = kv.split('=', 1)[1]
        extra = dict(startupinfo=si, creationflags=0x4000, env=env)
    split = ['-cyclesplit=' + os.path.join(out, 'split.jsonl')] if SPLIT else []
    if os.environ.get('PERF_PRESENTLOG') == '1':      # the time of each present (docs/TEST_COMMANDS.md, "The present log"): presents.txt
        split += ['-presentlog=' + os.path.join(out, 'presents.txt')]
    p = subprocess.Popen([exe, '-userdir=' + user, '-seed=%d' % SEED, '-realinput=' + os.path.join(out, 'script.txt')] +
                         window_args() + split + ([] if REAL else ['-fast']), cwd=out, stdout=log, stderr=subprocess.STDOUT, **extra)
    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
    created = creation_time(h)
    samples = []
    gfx = None
    while p.poll() is None:
        s = sample(h)
        if s:
            samples.append((round(time.time() - created, 3),) + s)
            if gfx is None and samples[-1][0] > 15:
                gfx = graphics_modules(h)
        time.sleep(0.1)
    end = samples[-1][0] if samples else 0
    s1 = system_times()
    begin = os.path.getmtime(mark) - created if os.path.exists(mark) else None
    res = summarise(samples, begin if begin is not None else end - 60)
    res.update({'exit': p.returncode, 'run_s': round(end, 1), 'machine_busy': round(busy_fraction(s0, s1), 2),
                'stretch_start_marked': begin is not None, 'updates_in_stretch': MINUTE + 30,
                'updates_per_s_in_stretch': round((MINUTE + 30) / (end - begin), 1) if begin is not None else None,
                'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
    res['cycles_check'] = cycle_check(h)
    res['graphics_modules'] = gfx
    res['window'] = WINDOW
    res['display'] = display_line(os.path.join(out, 'game.log'))
    json.dump({'samples': samples, 'result': res}, open(os.path.join(out, 'perf.json'), 'w'))
    k32.CloseHandle(h)
    log.close()
    print(json.dumps(res))


def split_lines(lines, extra):
    """Puts `at N <cmd>` lines for (N, cmd) in `extra` into a script's lines in the order of their updates (the lines that are not
    `at` lines, the mark's `quit`, stay last)."""
    at = [l for l in lines if l.startswith('at ')]
    rest = [l for l in lines if not l.startswith('at ')]
    at += ['at %d %s' % (n, c) for n, c in extra]
    at.sort(key=lambda l: int(l.split()[1]))     # stable: the mark stays after a count line at its update
    return at + rest


def orig_script(dst):
    """The scenario as origrun's script lines (origcmp/to_origrun.py): a mark 30 updates after the last input, then quit."""
    import to_origrun
    sc = scenario()
    lines = to_origrun.convert(sc['events'], 0, 30, None, sc['last'])
    if SPLIT:     # snapshots of the counters at the stretch's first update and just before the mark, in the script's order
        lines = split_lines(lines, [(sc['last'] - MINUTE, 'count a'), (sc['last'] + 20, 'count b')])
    with open(dst, 'w', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')
    return sc['last'] + 30


def reg(*args):
    subprocess.run(['reg'] + list(args), capture_output=True)


def orig_run(out, runs, load_only=False):
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    gameexe = os.path.join(root, 'build', 'origrun', RUN, 'game', 'AstroPopGame.exe')
    results = []
    for i in range(runs):
        d = os.path.abspath(os.path.join(out, 'orig%d' % i))
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        sc = os.path.join(d, 'script.txt')
        last = orig_script(sc)
        if load_only:      # the real clock: the game only waits and is stopped when it has loaded
            open(sc, 'w', newline='\n').write('wait 30000\nquit\n')
        # a fresh game: no player files, the PopCap key wiped (the user allows it, CLAUDE.md) and 2D as the port's runs have it
        users = os.path.join(root, 'build', 'origrun', RUN, 'game', 'users')
        for f in (os.listdir(users) if os.path.isdir(users) else []):
            os.remove(os.path.join(users, f))
        reg('delete', r'HKCU\Software\PopCap\AstroPop', '/f')
        reg('add', r'HKCU\Software\PopCap\AstroPop', '/v', 'Is3D', '/t', 'REG_DWORD', '/d', '0', '/f')
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        log = open(os.path.join(d, 'origrun.log'), 'wb')
        t_start = time.time()
        counts = os.path.join(root, 'build', 'origrun', RUN, 'counts.jsonl')
        if os.path.exists(counts):
            os.remove(counts)      # origrun appends its snapshots
        p = subprocess.Popen([origrun, '--game', 'astropop', '--run', RUN, '--exe', gameexe, '--seed', str(SEED),
                              '--clock', 'real' if (load_only or REAL) else 'virtual', '--cursor', 'scripted',
                              '--reg-map', 'popcap', '--timeout', '400', '--script', sc] +
                             (['--count', COUNT_LIST] if SPLIT and not load_only else []), stdout=log, stderr=subprocess.STDOUT)
        h = None
        created = ready = spawned = load_began = None
        started = False      # the title's click landed: the board exists
        ready_mem = None
        samples = []
        counters = []   # (time, update counter)
        next_sample = 0
        done = False
        deadline = time.time() + 420
        while p.poll() is None and not done and time.time() < deadline:
            if load_only and ready is not None:
                break
            if h is None:
                pid = find_process('AstroPopGame.exe', '\\build\\origrun\\' + RUN + '\\')
                if pid:
                    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED | TERMINATE, False, pid)
                    created = creation_time(h)
                    spawned = created - t_start
            if h:
                now = time.time()
                app = read32(h, APP)
                if ready is None and app:
                    flags = read32(h, app + LOADING)
                    if flags is not None and flags & 0xff and load_began is None:
                        load_began = now - created                    # the loading thread has started
                    if flags is not None and (flags >> 8) & 0xff:     # the loading thread has completed
                        ready = now - created
                        ready_mem = sample(h)
                if now >= next_sample:
                    s = sample(h)
                    c = read32(h, app + COUNTER) if app else None
                    if app and not started and (read32(h, app + BOARD) or 0) != 0:
                        started = True
                    if s:
                        samples.append((round(now - created, 3),) + s)
                        counters.append((round(now - created, 3), c))
                    next_sample = now + 0.1
                    if c is not None and c >= last:   # the script's mark has run: the original keeps running, so stop it here
                        done = True
            time.sleep(0.004 if ready is None else 0.02)
        if h:
            k32.TerminateProcess(h, 0)
        s1 = system_times()
        try:
            p.wait(30)
        except subprocess.TimeoutExpired:
            p.terminate()
        # the stretch: from the sample where the counter reached its first update to the one at the last input's update
        lo = next((t for t, c in counters if c is not None and c >= last - 30 - MINUTE), None)
        hi = next((t for t, c in counters if c is not None and c >= last), None)
        end = samples[-1][0] if samples else 0
        res = summarise(samples, lo if lo is not None else end - 60, hi)
        if lo is not None and hi is not None and hi > lo:
            c_lo = next(c for t, c in counters if t == lo)
            c_hi = next(c for t, c in counters if t == hi)
            res['updates_in_stretch'] = c_hi - c_lo
            res['updates_per_s_in_stretch'] = round((c_hi - c_lo) / (hi - lo), 1)
        res.update({'exit': p.returncode, 'run_s': round(end, 1), 'machine_busy': round(busy_fraction(s0, s1), 2),
                    'load_s': round(ready, 3) if ready else None, 'load_began_s': round(load_began, 3) if load_began else None, 'origrun_to_process_s': round(spawned, 1) if h else None,
                    'last_update_reached': counters[-1][1] if counters else None, 'game_started': started,
                    'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
        if ready_mem:
            res.update({'working_set_mb_at_title': round(ready_mem[0] / 1048576, 1),
                        'private_mb_at_title': round(ready_mem[1] / 1048576, 1), 'processor_s_at_title': round(ready_mem[4], 2)})
        if h:
            res['cycles_check'] = cycle_check(h)
        json.dump({'samples': samples, 'counters': counters, 'result': res}, open(os.path.join(d, 'perf.json'), 'w'))
        if os.path.exists(counts):
            shutil.copy(counts, os.path.join(d, 'counts.jsonl'))
        if h:
            k32.CloseHandle(h)
        log.close()
        results.append(res)
        print(json.dumps(res), flush=True)


def orig3d_run(out, runs, load_only=False):
    """The original in 3D on the visible desktop, in real time (origcmp/visrun3d.py's way to run it: the sandbox and dgVoodoo give it no
    Direct3D), 22:00 to 08:00 only. The scenario's inputs are posted to the game's window by update; the game runs silent (the stand-in's
    sound stub: no mixing in its number), at below-normal priority, its registry key set to Is3D 1 and put back. A run counts only if the
    game's 3D flag ([[0x5dc220]+0x320]+0x10) is 1 at the end."""
    night_only()
    import threading
    import uuid
    import visrun3d as v
    root = os.getcwd()
    v.SOURCE = os.path.join(root, 'build', 'origrun', RUN, 'game')
    v.WORK = os.path.join(root, 'build', 'ap-3d-perf')
    v.GAME = os.path.join(v.WORK, 'game')
    os.makedirs(v.WORK, exist_ok=True)
    for i in range(runs):
        d = os.path.abspath(os.path.join(out, 'orig3d%d' % i))
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        sc = os.path.join(d, 'script.txt')
        last = orig_script(sc)
        log = open(os.path.join(d, 'run.log'), 'w')
        backup = os.path.join(v.WORK, 'registry-backup')
        v.prepare()
        v.stage_registry(backup, True)
        run_id = uuid.uuid4().hex[:12]
        cur = v.Cursor(run_id)
        env = dict(os.environ)
        env.update({'ORIGRUN_DIR': os.path.join(v.WORK, 'logs'), 'ORIGRUN_SOUND': 'stub', 'ORIGRUN_REGMAP': 'popcap',
                    'ORIGRUN_CURSOR': 'scripted', 'ORIGRUN_CURSOR_ID': run_id})
        os.makedirs(env['ORIGRUN_DIR'], exist_ok=True)
        ctr = None
        counts_file = os.path.join(d, 'counts.jsonl')
        if SPLIT and not load_only:       # the stand-in counts the functions of count.txt; the block's counters are read from here
            import countsnap
            resolved = os.path.join(d, 'count-resolved.txt')
            countsnap.resolve_list(os.path.join(v.GAME, 'AstroPopGame.exe'), COUNT_LIST, resolved)
            map_name = 'Local\\apcount_' + uuid.uuid4().hex[:12]
            env.update({'ORIGRUN_COUNT': resolved, 'ORIGRUN_COUNT_MAP': map_name})
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        g = None
        samples, counters = [], []
        state = {'ready': None, 'ready_mem': None, 'stop': False, 'started': False}
        try:
            g = v.Game(os.path.join(v.GAME, 'AstroPopGame.exe'), env)
            created = creation_time(g.h)
            if SPLIT and not load_only:
                ctr = countsnap.Counter(g.h, map_name)

            def sampler():
                next_sample = 0
                while not state['stop']:
                    now = time.time()
                    app = g.rd(APP)
                    if state['ready'] is None and app:
                        flags = g.rd(app + LOADING)
                        if flags is not None and (flags >> 8) & 0xff:          # the loading thread has completed
                            state['ready'] = now - created
                            state['ready_mem'] = sample(g.h)
                    if now >= next_sample:
                        s = sample(g.h)
                        c = g.rd(app + COUNTER) if app else None
                        if app and not state['started'] and (g.rd(app + BOARD) or 0) != 0:
                            state['started'] = True
                        if s:
                            samples.append((round(now - created, 3),) + s)
                            counters.append((round(now - created, 3), c))
                        next_sample = now + 0.1
                    time.sleep(0.02)
            th = threading.Thread(target=sampler, daemon=True)
            th.start()
            t0 = time.time()
            while not g.window() and time.time() - t0 < 90 and g.p.poll() is None:
                time.sleep(0.05)
            if g.window():
                v.post_input(g, cur, 'focus', ['regain'])        # the game believes it has the focus
                while time.time() - t0 < 120 and g.p.poll() is None and state['ready'] is None:
                    time.sleep(0.02)
                if not load_only:
                    for line in open(sc):
                        w = line.split()
                        if not w or w[0] == 'quit':
                            if w:
                                break
                            continue
                        if w[0] != 'at':
                            continue
                        n, cmd, args = int(w[1]), w[2], w[3:]
                        if v.wait_counter(g, n, log) is None:
                            break
                        if cmd == 'mark':
                            if SPLIT:       # the split's last snapshot comes 20 updates after the last input, past the mark
                                continue
                            break
                        if cmd == 'count':       # a snapshot of the counters at this update; nothing is held, as origrun's
                            if not ctr.ready():
                                print('count %s: the stand-in has made no counters: %s' % (args[0], ctr.error() if ctr.mm else 'no block'), file=log, flush=True)
                            else:
                                ctr.snapshot(args[0], g.counter(), counts_file)
                            continue
                        g.hold()
                        try:
                            v.post_input(g, cur, cmd, args)
                        finally:
                            g.release()
            flag = g.flag3d()
            s1 = system_times()
            state['stop'] = True
            th.join()
        finally:
            if g and g.p.poll() is None:
                hw = g.window()
                if hw:
                    v.u32.PostMessageW(hw, 0x0010, 0, 0)
                try:
                    g.p.wait(20)
                except subprocess.TimeoutExpired:
                    g.p.kill()
                    g.p.wait(10)
            time.sleep(0.5)
            v.restore_registry(backup)
            log.close()
        lo = next((t for t, c in counters if c is not None and c >= last - 30 - MINUTE), None)
        hi = next((t for t, c in counters if c is not None and c >= last), None)
        end = samples[-1][0] if samples else 0
        res = summarise(samples, lo if lo is not None else end - 60, hi)
        if lo is not None and hi is not None and hi > lo:
            c_lo = next(c for t, c in counters if t == lo)
            c_hi = next(c for t, c in counters if t == hi)
            res['updates_in_stretch'] = c_hi - c_lo
            res['updates_per_s_in_stretch'] = round((c_hi - c_lo) / (hi - lo), 1)
        res.update({'exit': g.p.returncode if g else None, 'run_s': round(end, 1), 'machine_busy': round(busy_fraction(s0, s1), 2),
                    'load_s': round(state['ready'], 3) if state['ready'] else None, 'flag3d': flag,
                    'last_update_reached': counters[-1][1] if counters else None, 'game_started': state['started'],
                    'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
        if state['ready_mem']:
            m = state['ready_mem']
            res.update({'working_set_mb_at_title': round(m[0] / 1048576, 1), 'private_mb_at_title': round(m[1] / 1048576, 1),
                        'processor_s_at_title': round(m[4], 2)})
        if g:
            res['cycles_check'] = cycle_check(g.h)
        json.dump({'samples': samples, 'counters': counters, 'result': res}, open(os.path.join(d, 'perf.json'), 'w'))
        print(json.dumps(res), flush=True)


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else ''
    if mode == 'port-load':
        port_load(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 3)
    elif mode == 'port-minute':
        port_minute(sys.argv[2])
    elif mode == 'orig3d':
        orig3d_run(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 1, len(sys.argv) > 4 and sys.argv[4] == 'load')
    elif mode == 'orig':
        orig_run(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 1, len(sys.argv) > 4 and sys.argv[4] == 'load')
    else:
        sys.exit(__doc__)
