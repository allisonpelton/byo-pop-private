"""Load time, memory and processor time of AstroPop Deluxe, the port and the original, for GAME_NOTES.md "Performance".

Adapted from games/atomica/tools/profile/perf.py (its method: a sample of the process every 0.1 s, the time the title screen
can take input read out of the original's memory).

From the worktree root:

    build/native/rundesk.exe <python.exe> games/astropop/tools/profile/perf.py port-load <out dir> [runs]
    build/native/rundesk.exe <python.exe> games/astropop/tools/profile/perf.py port-minute <out dir>
    <python.exe> games/astropop/tools/profile/perf.py orig <out dir> [runs] [load]    (not under rundesk; "load" stops at the title)

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
    return json.load(open(SCENARIO))


def port_script(dst, mark):
    """The scenario as -realinput lines (an update, a command), a screenshot at the stretch's first update and a quit after the end."""
    sc = scenario()
    with open(dst, 'w', newline='\n') as f:
        for u, cmd in sc['events']:
            f.write('%d %s\n' % (u, cmd))
        f.write('%d screenshot %s\n' % (sc['last'] - MINUTE, mark.replace('\\', '/')))
        f.write('%d quit\n' % (sc['last'] + 30))
    return sc['last']


def port_exe():
    # PERF_EXE names another build of the port in build/native (an exe kept from before a change)
    return os.path.join(os.getcwd(), 'build', 'native', os.environ.get('PERF_EXE', 'astropop.exe'))


def port_user(d):
    os.makedirs(os.path.join(d, 'user'), exist_ok=True)
    with open(os.path.join(d, 'user', 'registry.ini'), 'w', newline='\n') as f:   # 2D, as the original runs in these comparisons
        f.write('[SOFTWARE\\PopCap\\AstroPop]\nIs3D=dword:0\n')
    return os.path.join(d, 'user')


def port_load(out, runs):
    import argparse
    import playbymove
    exe = port_exe()
    results = []
    for i in range(runs):
        d = os.path.abspath(os.path.join(out, 'load%d' % i))
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        a = argparse.Namespace(dir=d, timeout=120, exe=exe, realtime=True, game_args=['-seed=%d' % SEED, '-userdir=' + port_user(d)])
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
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
    p = subprocess.Popen([exe, '-userdir=' + user, '-seed=%d' % SEED, '-realinput=' + os.path.join(out, 'script.txt')] +
                         ([] if REAL else ['-fast']), cwd=out, stdout=log, stderr=subprocess.STDOUT)
    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
    created = creation_time(h)
    samples = []
    while p.poll() is None:
        s = sample(h)
        if s:
            samples.append((round(time.time() - created, 3),) + s)
        time.sleep(0.1)
    end = samples[-1][0] if samples else 0
    s1 = system_times()
    begin = os.path.getmtime(mark) - created if os.path.exists(mark) else None
    res = summarise(samples, begin if begin is not None else end - 60)
    res.update({'exit': p.returncode, 'run_s': round(end, 1), 'machine_busy': round(busy_fraction(s0, s1), 2),
                'stretch_start_marked': begin is not None, 'updates_in_stretch': MINUTE + 30,
                'updates_per_s_in_stretch': round((MINUTE + 30) / (end - begin), 1) if begin is not None else None,
                'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
    json.dump({'samples': samples, 'result': res}, open(os.path.join(out, 'perf.json'), 'w'))
    k32.CloseHandle(h)
    log.close()
    print(json.dumps(res))


def orig_script(dst):
    """The scenario as origrun's script lines (origcmp/to_origrun.py): a mark 30 updates after the last input, then quit."""
    import to_origrun
    sc = scenario()
    lines = to_origrun.convert(sc['events'], 0, 30, None, sc['last'])
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
        p = subprocess.Popen([origrun, '--game', 'astropop', '--run', RUN, '--exe', gameexe, '--seed', str(SEED),
                              '--clock', 'real' if (load_only or REAL) else 'virtual', '--cursor', 'scripted',
                              '--reg-map', 'popcap', '--timeout', '400', '--script', sc], stdout=log, stderr=subprocess.STDOUT)
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
    elif mode == 'orig':
        orig_run(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 1, len(sys.argv) > 4 and sys.argv[4] == 'load')
    else:
        sys.exit(__doc__)
