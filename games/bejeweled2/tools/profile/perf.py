"""Load time, memory and processor time of Bejeweled 2 Deluxe, the port and the original, for GAME_NOTES.md "Performance".

Adapted from games/astropop/tools/profile/perf.py (its method: a sample of the process every 0.1 s, the time the title screen
can take input read out of the original's memory).

From the worktree root:

    build/native/rundesk.exe <python.exe> games/bejeweled2/tools/profile/perf.py port-load <out dir> [runs]
    build/native/rundesk.exe <python.exe> games/bejeweled2/tools/profile/perf.py port-minute <out dir>
    <python.exe> games/bejeweled2/tools/profile/perf.py orig <out dir> [runs] [load]    (not under rundesk; "load" stops at the title)

The scenario is scenario/classic909.json: origcmp/record.py's events.json of a Classic game (seed 909, the profile player "Ali", 2D at
640 by 480, the play click at update 571, the first move at 1213, 36 swaps), plus its `last` update. The port plays it as a -commands file
(origcmp/to_commands.py) under -hold, which the recording's board needs (without it the first board differs and every move is illegal), the
original as origrun script lines (origcmp/to_origrun.py), so both play the same game; the original's board score (+0x15e20) and state
(+0x170c) are watched and printed as final_score and final_state, the port's `state` after the last move as final_state. PERF_MODE=3d plays
scenario/classic909_3d.json instead (the 1024 by 768 game on the GPU renderer; the original in 3D, under dgVoodoo2, as origcmp's HIRES=1 runs it). PERF_RENDER=software|gpu picks the port's
renderer (default: software in 2D, the machine's in 3D); PERF_SHOTS=1 takes pictures at 1000, 3000 and 6400; PERF_MEM=1 puts memreport
lines into the port's commands (game.log, shots/mem_*.txt).

port-load starts the port with a command file that takes a screenshot at its second update (the title has loaded by then) and quits;
the time of that file counts from the creation of the process. port-minute plays the scenario (a screenshot at update 2 marks the title,
its sample is the load's share; at the end `state` and a quit); orig plays it in the original under origrun on the host (scripted cursor,
the players and the registry key put back as origcmp/run_original.sh does, a copy of the original staged in build/origrun/bej2). With
PERF_CLOCK=real both play in real time (the port without -fast, the original on origrun's real clock; the port's boards then differ from
the recording's, so the moves are illegal ones); without it neither waits (the port's -fast, origrun's virtual clock), so the processor
time is the work done. `orig ... load` stops when the title has loaded. orig starts origrun directly: origrun puts the game under rundesk
itself.

Both sample the game's process every 0.1 s: working set, private bytes (and the system's own peaks of both), processor time (the
cycle count over the machine's clock, see sample()). port-idle plays the idle stretch (PERF_STRETCH=idle) as a -script file in real time and the
counters of a patched framework (statspatch.py) are read from the game's log. Each run prints one JSON line with the peaks and the processor time over the whole run (the original's also over the 2,000 updates
before the last input, `scripted_stretch`; the port's after the title, `play_processor_s`), the machine's busy
fraction during the run and the game processes of other sessions running when it started and ended. The original's run is stopped
from here once its update counter ([0x67ccd8] + 0x484) passes the script's last update; a run counts only if the play click landed
(`game_started`: the board pointer, +0xe68 of the app, is set). pairs.sh runs the two turn about.
"""
import ctypes
import ctypes.wintypes as W
import json
import os
import re
import shutil
import statistics
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ORIGCMP = os.path.join(HERE, '..', 'origcmp')
sys.path.insert(0, ORIGCMP)

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

SEED = 909
MINUTE = 2000   # updates of the stretch
REAL = os.environ.get('PERF_CLOCK') == 'real'
APP = 0x67CCD8                 # the original's app object: a static pointer to it
COUNTER, BOARD, TITLE = 0x484, 0xE68, 0xE78   # offsets in the app: the update counter, the board, the title screen (its loaded flag: the dword at +0x9d, the byte on top)
MODE3D = os.environ.get('PERF_MODE') == '3d'    # PERF_MODE=3d: the 1024 by 768 game on the GPU renderer, what a player's machine runs
SCENARIO = os.path.join(HERE, 'scenario', 'classic909_3d.json' if MODE3D else 'classic909.json')
# PERF_STRETCH=idle: the recording's inputs up to the click that dismisses the board's first dialog, then nothing, to update 5700 (the
# board idle, 57 s of play from the first click): the same stretch on both sides in real time, where the recorded game cannot be played
# (the first board is another one, the moves are illegal). The port plays it as a -script file: no -commands file, so no poll of one.
IDLE = os.environ.get('PERF_STRETCH') == 'idle'
IDLE_UNTIL = 738 if MODE3D else 956
IDLE_LAST = 5700
RUN = 'bej2'                   # build/origrun/bej2: the staged copy of the original
# the original's 3D (DirectDraw and Direct3D 7) runs under dgVoodoo2, its Direct3D 11 translation, as every 3D comparison of this game does
DGVOODOO = ['--dgvoodoo', os.environ.get('ORIGRUN_DGVOODOO', 'E:/retools/dgVoodoo2_87_5')] if MODE3D else []
PROFILE = os.environ.get('PROFILE', 'build/logs/bej2/profile')   # the player (make_profile.py), as the recordings have it
USERS = 'C:/ProgramData/Steam/Bejeweled2/users'   # where the original keeps its players


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


def cpu_hz():
    """The rate the cycle counters tick at (the machine's TSC: a thread that spins for a second counts this many; the registry's ~MHz
    of processor 0 is the same to 1%), to turn cycles into the seconds of one core that the notes and the README give."""
    import winreg
    with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'HARDWARE\DESCRIPTION\System\CentralProcessor\0') as k:
        return winreg.QueryValueEx(k, '~MHz')[0] * 1e6


CPU_HZ = cpu_hz()
k32.QueryProcessCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]


def sample(h):
    """(working set, private bytes, peak working set, peak private, processor seconds) of the process, or None. The processor
    seconds are the process's cycle count (QueryProcessCycleTime: every thread's QueryThreadCycleTime, the exited ones too) over
    CPU_HZ. GetProcessTimes and GetThreadTimes are charged by the scheduler's 15.6 ms tick and moved by 20% in a run of exactly
    known work (the coordinator's test, 2026-11-11), so nothing here reads them."""
    m = PMC()
    m.cb = ctypes.sizeof(PMC)
    if not psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb):
        return None
    cycles = ctypes.c_uint64()
    if not k32.QueryProcessCycleTime(h, ctypes.byref(cycles)):
        return None
    return (m.WorkingSetSize, m.PrivateUsage, m.PeakWorkingSetSize, m.PeakPagefileUsage, cycles.value / CPU_HZ)


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.Thread32First.argtypes = [W.HANDLE, ctypes.POINTER(THREADENTRY32)]
k32.Thread32Next.argtypes = [W.HANDLE, ctypes.POINTER(THREADENTRY32)]
k32.GetThreadDescription.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_wchar_p)]


def thread_table(pid):
    """{thread id: (processor seconds from the thread's cycle count, name)} of the process's live threads: where the process's time goes by thread (the game's main
    thread, SDL's audio thread, the driver's threads). The name is what the thread says it is (SDL names its own), or ''."""
    out = {}
    snap = k32.CreateToolhelp32Snapshot(0x4, 0)      # TH32CS_SNAPTHREAD
    if not snap or snap == W.HANDLE(-1).value:
        return out
    e = THREADENTRY32()
    e.dwSize = ctypes.sizeof(e)
    ok = k32.Thread32First(snap, ctypes.byref(e))
    while ok:
        if e.th32OwnerProcessID == pid:
            th = k32.OpenThread(0x0800, False, e.th32ThreadID)      # THREAD_QUERY_LIMITED_INFORMATION
            if th:
                cycles = ctypes.c_uint64()
                if k32.QueryThreadCycleTime(th, ctypes.byref(cycles)):
                    name = ctypes.c_wchar_p()
                    nm = ''
                    if k32.GetThreadDescription(th, ctypes.byref(name)) >= 0 and name.value:
                        nm = name.value
                    out[e.th32ThreadID] = (cycles.value / CPU_HZ, nm)
                k32.CloseHandle(th)
        e.dwSize = ctypes.sizeof(e)
        ok = k32.Thread32Next(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


def thread_summary(table):
    """The threads that used the most time, as [seconds, name or id], with the share of all the threads'."""
    total = sum(v[0] for v in table.values()) or 1.0
    top = sorted(table.items(), key=lambda kv: -kv[1][0])[:6]
    return [{'thread': (v[1] or str(t)), 'cpu_s': round(v[0], 2), 'share': round(v[0] / total, 3)} for t, v in top]


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
                'processor_s': round(s[-1][5] - s[0][5], 2), 'wall_s': round(s[-1][0] - s[0][0], 1)}
    whole = span(0, hi)
    if whole and samples:
        last = [x for x in samples if hi is None or x[0] <= hi][-1]
        whole['os_peak_working_set_mb'] = round(last[3] / 1048576, 1)
        whole['os_peak_private_mb'] = round(last[4] / 1048576, 1)
    return {'whole_run': whole, 'scripted_stretch': span(lo, hi)}


def scenario():
    sc = json.load(open(SCENARIO))
    if IDLE:
        sc['events'] = [e for e in sc['events'] if e[0] <= IDLE_UNTIL]
        sc['last'] = IDLE_LAST
    return sc


def log_stats(text):
    """The temporary counters of a patched framework (instr.py, never committed): the cycles of the commands file's poll, which a test
    run pays and the original never does, and, with DRAWSTATS set, the draw calls by kind."""
    r = {}
    m = re.search(r'POLLSTATS calls (\d+) cycles (\d+)', text)
    if m:
        r['poll_calls'] = int(m.group(1))
        r['poll_s'] = round(int(m.group(2)) / CPU_HZ, 3)
    d = re.findall(r'DRAWSTATS (.+)', text)
    if d:
        r['draw_stats'] = d
    return r


def port_exe():
    # PERF_EXE names another build of the port in build/native (an exe kept from before a change)
    return os.path.join(os.getcwd(), 'build', 'native', os.environ.get('PERF_EXE', 'bejeweled2.exe'))


def port_user(d):
    """The port's user folder: the profile's players and a registry.ini asking for windowed 2D, as origcmp/record.py starts it."""
    u = os.path.join(d, 'user')
    os.makedirs(u, exist_ok=True)
    if PROFILE and os.path.isdir(os.path.join(PROFILE, 'users')):
        shutil.copytree(os.path.join(PROFILE, 'users'), os.path.join(u, 'users'), dirs_exist_ok=True)
    # the settings of the recordings (realplay/driver_base.py): windowed, and in 2D the 640 by 480 game
    with open(os.path.join(u, 'registry.ini'), 'w', newline=chr(10)) as f:
        f.write('[SOFTWARE' + chr(92) + 'Steam' + chr(92) + 'Bejeweled2]' + chr(10) + 'ScreenMode=dword:0' + chr(10))
        if not MODE3D:
            f.write('HighResolution=dword:0' + chr(10))
    return u


def port_env():
    """The port's environment. PERF_RENDER=software asks SDL's software renderer (true 2D, as the recordings run it: DDInterface::mIs3D
    false); the default is the machine's own renderer, Direct3D 11 on a GPU, which is what a player's machine runs. The software
    renderer redraws and copies the whole 640 by 480 picture on the processor and cost 20 ms a drawn frame here."""
    env = dict(os.environ)
    if os.environ.get('PERF_RENDER') == 'software' or (not MODE3D and os.environ.get('PERF_RENDER') != 'gpu'):
        env['SDL_RENDER_DRIVER'] = 'software'
    return env


def commands_file(dst, events, mark_name=None, mark_at=None, quit_after=30):
    """events: [[update, command], ...] as events.json has them; a `shot mark` at mark_at becomes a screenshot <mark_name>."""
    import to_commands  # noqa: F401  (the module's main() reads argv: it is run as a script below)
    ev = [e for e in events if not e[1].startswith('shot ')]
    if mark_at is not None:
        ev.append([mark_at, 'shot ' + os.path.basename(mark_name)[:-4]])
    if os.environ.get('PERF_SHOTS'):     # PERF_SHOTS=1: also a picture at 1000, 3000 and 6400, to see what the scenario plays
        ev += [[u, 'shot s%d' % u] for u in (1000, 3000, 6400)]
    ev.sort(key=lambda e: e[0])
    tmp = dst + '.events.json'
    json.dump({'events': ev}, open(tmp, 'w'))
    subprocess.run([sys.executable, '-I', os.path.join(ORIGCMP, 'to_commands.py'), tmp, dst, '--shots', os.path.dirname(mark_name) if mark_name else 'x'],
                   check=True, capture_output=True)
    os.remove(tmp)


def port_load(out, runs):
    results = []
    for i in range(runs):
        d = os.path.abspath(os.path.join(out, 'load%d' % i))
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(os.path.join(d, 'shots'))
        mark = os.path.join(d, 'shots', 'title.png')
        with open(os.path.join(d, 'cmd.txt'), 'w', newline='\n') as f:
            f.write('wait 2\nscreenshot shots/title.png\nquit\n')
        open(os.path.join(d, 'empty.ri'), 'w').close()
        exe = port_exe()
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        p = subprocess.Popen([exe, '-gamedir=', '-userdir=' + port_user(d), '-commands=' + os.path.join(d, 'cmd.txt'),
                              '-realinput=' + os.path.join(d, 'empty.ri'), '-seed=%d' % SEED], cwd=d, env=port_env(),
                             stdout=open(os.path.join(d, 'game.log'), 'wb'), stderr=subprocess.STDOUT)
        h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
        created = creation_time(h)
        at_title = None
        while p.poll() is None:
            if at_title is None and os.path.exists(mark):
                time.sleep(0.05)
                at_title = (os.path.getmtime(mark) - created, sample(h))
            time.sleep(0.02)
        s1 = system_times()
        others1 = game_processes(mine_prefixes())
        k32.CloseHandle(h)
        if at_title is None or at_title[1] is None:
            print(json.dumps({'error': 'the title never took a screenshot', 'exit': p.returncode, 'others_start': len(others0), 'others_end': len(others1)}))
            continue
        m = at_title[1]
        r = {'load_s': round(at_title[0], 3), 'working_set_mb_at_title': round(m[0] / 1048576, 1),
             'private_mb_at_title': round(m[1] / 1048576, 1), 'processor_s_at_title': round(m[4], 2),
             'machine_busy': round(busy_fraction(s0, s1), 2), 'others_start': len(others0), 'others_end': len(others1)}
        results.append(r)
        print(json.dumps(r), flush=True)
    t = [r['load_s'] for r in results]
    if t:
        print(json.dumps({'port_load_s': t, 'median_s': round(statistics.median(t), 3), 'others_start': 0, 'others_end': 0}))


def port_minute(out):
    """Plays the scenario in the port: a screenshot at update 2 marks the time the title could take input (its sample is kept as the
    load's share), and the run's last lines are the game's own count of updates and draws and a `state` after the last move."""
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'shots'))
    sc = scenario()
    mark = os.path.join(out, 'shots', 'title.png')
    cmd = os.path.join(out, 'cmd.txt')
    commands_file(cmd, sc['events'], 'shots/title.png', 2)
    text = open(cmd).read().rstrip('\n').split('\n')
    assert text[-1] == 'quit'
    text[-1:] = ['state', 'wait 2', 'quit']     # the game's state after the last move, to check that the play was the recorded one
    if os.environ.get('PERF_MEM'):      # PERF_MEM=1: the game's own memory report (memreport, docs/TEST_COMMANDS.md) at the title, with the board up and at the end
        moves = [k for k, l in enumerate(text) if l.startswith('move game')]
        text[-3:-3] = ['memreport shots/mem_end.txt']
        text[moves[3]:moves[3]] = ['memreport shots/mem_board.txt']
        text[moves[0]:moves[0]] = ['memreport shots/mem_title.txt']
    open(cmd, 'w', newline='\n').write('\n'.join(text) + '\n')
    open(os.path.join(out, 'empty.ri'), 'w').close()
    others0 = game_processes(mine_prefixes())
    s0 = system_times()
    log = open(os.path.join(out, 'game.log'), 'wb')
    p = subprocess.Popen([port_exe(), '-gamedir=', '-userdir=' + port_user(out), '-commands=' + cmd,
                          '-realinput=' + os.path.join(out, 'empty.ri'), '-seed=%d' % SEED] + ['-hold'] + ([] if REAL else ['-fast']),
                         cwd=out, env=port_env(), stdout=log, stderr=subprocess.STDOUT)
    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
    created = creation_time(h)
    samples = []
    at_title = None
    threads = {}
    next_threads = 0
    while p.poll() is None:
        s = sample(h)
        if s:
            samples.append((round(time.time() - created, 3),) + s)
            if at_title is None and os.path.exists(mark):
                at_title = samples[-1]
        if time.time() >= next_threads:       # a table a second: the last one is the run's, less the last second
            t = thread_table(p.pid)
            if t:
                threads = t
            next_threads = time.time() + 1.0
        time.sleep(0.1)
    end = samples[-1][0] if samples else 0
    s1 = system_times()
    log.close()
    text = open(os.path.join(out, 'game.log'), errors='replace').read()
    m = re.search(r'Seconds = ([\d.]+), Sleep Count = (\d+), Update Count = (\d+), Draw Count = (\d+)', text)
    st = re.search(r'STATE (\{.*\})', text)
    res = summarise(samples, at_title[0] if at_title else 0)
    res['scripted_stretch'] = None      # the port has no counter to read from outside: the play is the whole run less the title's share
    play = samples[-1][5] - at_title[5] if at_title else None
    res.update({'exit': p.returncode, 'run_s': round(end, 1), 'machine_busy': round(busy_fraction(s0, s1), 2),
                'title_processor_s': round(at_title[5], 2) if at_title else None,
                'title_private_mb': round(at_title[2] / 1048576, 1) if at_title else None,
                'play_processor_s': round(play, 2) if play is not None else None,
                'play_wall_s': round(end - at_title[0], 1) if at_title else None,
                'updates': int(m.group(3)) if m else None, 'draws': int(m.group(4)) if m else None,
                'final_state': json.loads(st.group(1)) if st else None, 'threads': thread_summary(threads), **log_stats(text),
                'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
    json.dump({'samples': samples, 'result': res}, open(os.path.join(out, 'perf.json'), 'w'))
    k32.CloseHandle(h)
    print(json.dumps(res))


def port_idle(out):
    """The idle scenario in real time as a -script run (see IDLE): the last 20 s before its quit are the stretch, with the processor
    time of the SDL audio thread (the framework's mixer; the original under origrun mixes nothing) counted apart."""
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    sc = scenario()
    lines = []
    for u, cmd, *_ in sc['events']:
        verb, space, x, y = cmd.split()
        assert space == 'game', cmd
        lines.append('%d %s %s %s' % (u, verb, x, y))
    last = sc['last']
    lines += ['%d state' % (last - 5), '%d quit' % last]
    if os.environ.get('PERF_SHOT'):     # PERF_SHOT=1: a picture of the idle board at update 5000, to compare two builds' drawing
        os.makedirs(os.path.join(out, 'shots'))
        lines.insert(-2, '5000 screenshot shots/idle.png')
        lines.insert(-2, '5001 screenshotfull shots/idlefull.png')   # its size is the render scale times 640 by 480: the window state of the run
    script = os.path.join(out, 'script.txt')
    open(script, 'w', newline=chr(10)).write(chr(10).join(lines) + chr(10))
    others0 = game_processes(mine_prefixes())
    s0 = system_times()
    log = open(os.path.join(out, 'game.log'), 'wb')
    p = subprocess.Popen([port_exe(), '-gamedir=', '-userdir=' + port_user(out), '-script=' + script, '-seed=%d' % SEED] + os.environ.get('PERF_ARGS', '').split(),
                         cwd=out, env=port_env(), stdout=log, stderr=subprocess.STDOUT)
    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
    created = creation_time(h)
    samples = []
    timeline = []      # (seconds from the creation, thread table) a second
    next_threads = 0
    while p.poll() is None:
        s = sample(h)
        if s:
            samples.append((round(time.time() - created, 3),) + s)
        if time.time() >= next_threads:
            t = thread_table(p.pid)
            if t:
                timeline.append((time.time() - created, t))
            next_threads = time.time() + 1.0
        time.sleep(0.1)
    end = samples[-1][0] if samples else 0
    s1 = system_times()
    log.close()
    text = open(os.path.join(out, 'game.log'), errors='replace').read()
    m = re.search(r'Seconds = ([\d.]+), Sleep Count = (\d+), Update Count = (\d+), Draw Count = (\d+)', text)
    st = re.search(r'STATE (\{.*\})', text)
    res = summarise(samples, end - 20.0)
    # the audio thread's share of the stretch: the thread tables nearest to its two ends
    def nearest(t):
        return min(timeline, key=lambda e: abs(e[0] - t))[1] if timeline else {}
    a, b = nearest(end - 20.0), nearest(end)
    audio = sum(b[t][0] - a[t][0] for t in b if t in a and b[t][1].startswith('SDLAudio'))
    if res['scripted_stretch']:
        res['scripted_stretch']['audio_s'] = round(audio, 3)
    res.update({'exit': p.returncode, 'run_s': round(end, 1), 'machine_busy': round(busy_fraction(s0, s1), 2),
                'updates': int(m.group(3)) if m else None, 'draws': int(m.group(4)) if m else None,
                'final_state': json.loads(st.group(1)) if st else None, 'threads': thread_summary(timeline[-1][1] if timeline else {}),
                'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes())), **log_stats(text)})
    json.dump({'samples': samples, 'result': res}, open(os.path.join(out, 'perf.json'), 'w'))
    k32.CloseHandle(h)
    print(json.dumps(res))


def orig_script(dst):
    """The scenario as origrun's script lines (origcmp/to_origrun.py), led by a wait for the title's loaded flag as
    coverage/run.sh does (the recording's first input is at update 566; the title loads on the wall clock)."""
    import to_origrun
    sc = scenario()
    lines = [l for l in to_origrun.convert({'events': sc['events']}, 0, 30, None, dialog_shift=-1 if MODE3D else 0) if ' shot ' not in l and not l.endswith(' mark') and l != 'quit']
    lines = [l for l in lines if not l.startswith('#')]
    lines.insert(0, 'waitfor [[0x67ccd8]+0xe78]+0x9d >= 0x01000000 180000')
    with open(dst, 'w', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')
    return sc['last'] + 30


def reg(*args):
    subprocess.run(['reg'] + list(args), capture_output=True)


def orig_setup():
    """A run starts as a first run does but with the profile's players there (origcmp/run_original.sh's setup_state): the user
    allows changes to PopCap games' registry values and saves (CLAUDE.md)."""
    shutil.rmtree(USERS, ignore_errors=True)
    reg('delete', r'HKCU\Software\Steam\Bejeweled2', '/f')
    reg('add', r'HKCU\Software\Steam\Bejeweled2', '/v', 'ScreenMode', '/t', 'REG_DWORD', '/d', '0', '/f')
    if PROFILE and os.path.isdir(os.path.join(PROFILE, 'users')):
        os.makedirs(os.path.dirname(USERS), exist_ok=True)
        shutil.copytree(os.path.join(PROFILE, 'users'), USERS)


def orig_run(out, runs, load_only=False):
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    gameexe = os.path.join(root, 'build', 'origrun', RUN, 'game', 'Bejeweled2Game.exe')
    results = []
    for i in range(runs):
        d = os.path.abspath(os.path.join(out, 'orig%d' % i))
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        sc = os.path.join(d, 'script.txt')
        last = orig_script(sc)
        if load_only:      # the real clock: the game only waits and is stopped when the title has loaded
            open(sc, 'w', newline='\n').write('wait 30000\nquit\n')
        orig_setup()
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        log = open(os.path.join(d, 'origrun.log'), 'wb')
        t_start = time.time()
        p = subprocess.Popen([origrun, '--game', 'bejeweled2', '--run', RUN, '--exe', gameexe, '--seed', str(SEED),
                              '--clock', 'real' if (load_only or REAL) else 'virtual', '--cursor', 'scripted',
                              '--timeout', '400'] + DGVOODOO + ['--script', sc, '--watch', '[[0x67ccd8]+0xe68]+0x15e20', '--watch', '[[0x67ccd8]+0xe68]+0x170c'], stdout=log, stderr=subprocess.STDOUT)
        h = None
        created = ready = spawned = None
        started = False      # the play click landed: the board exists
        ready_mem = None
        samples = []
        counters = []   # (time, update counter)
        threads = {}
        next_threads = 0
        next_sample = 0
        done = False
        deadline = time.time() + 420
        while p.poll() is None and not done and time.time() < deadline:
            if load_only and ready is not None:
                break
            if h is None:
                pid = find_process('Bejeweled2Game.exe', '\\build\\origrun\\' + RUN + '\\')
                if pid:
                    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED | TERMINATE, False, pid)
                    created = creation_time(h)
                    spawned = created - t_start
            if h:
                now = time.time()
                app = read32(h, APP)
                if ready is None and app:
                    title = read32(h, app + TITLE)
                    flags = read32(h, title + 0x9d) if title else None
                    if flags is not None and flags >= 0x01000000:     # the title's Play link shows: it takes input
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
                    if now >= next_threads:
                        t = thread_table(pid)
                        if t:
                            threads = t
                        next_threads = now + 1.0
                    if c is not None and c >= last:   # the script's last input and its tail have run: stop the original here
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
                    'load_s': round(ready, 3) if ready else None, 'origrun_to_process_s': round(spawned, 1) if h else None,
                    'last_update_reached': counters[-1][1] if counters else None, 'game_started': started,
                    'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
        if ready_mem:
            res.update({'working_set_mb_at_title': round(ready_mem[0] / 1048576, 1),
                        'private_mb_at_title': round(ready_mem[1] / 1048576, 1), 'processor_s_at_title': round(ready_mem[4], 2)})
        watched = {}     # the board's score (+0x15e20) and state (+0x170c) as origrun logged them last: the play was the recorded one
        for m in re.finditer(r'watch (\d): \S+ -> (\S+) at update (\d+)', open(os.path.join(d, 'origrun.log'), errors='replace').read()):
            watched[m.group(1)] = m.group(2)
        res['final_score'], res['final_state'] = watched.get('1'), watched.get('2')
        res['threads'] = thread_summary(threads)
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
        port_minute(sys.argv[2])
    elif mode == 'port-idle':
        port_idle(sys.argv[2])
    elif mode == 'orig':
        orig_run(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 1, len(sys.argv) > 4 and sys.argv[4] == 'load')
    else:
        sys.exit(__doc__)
