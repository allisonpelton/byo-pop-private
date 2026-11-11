"""Load time, memory and processor time of Typer Shark Deluxe, the port and the original, for GAME_NOTES.md "Performance".

Adapted from games/atomica/tools/profile/perf.py (its method: a sample of the process every 0.1 s, the loaded flag read out of
the original's memory).

From the worktree root:

    build/native/rundesk.exe <python.exe> games/typershark/tools/profile/perf.py port-load <out dir> [runs]
    build/native/rundesk.exe <python.exe> games/typershark/tools/profile/perf.py port-minute <out dir> [port script]
    <python.exe> games/typershark/tools/profile/perf.py orig <out dir> [port script] [runs] [load]   (not under rundesk;
                                                                                                    "load" stops at the loaded flag)

The scenario is a port script (`<update> char|click|key ...`): by default build/ts-p2/rec/rt2.in.txt, the whole-adventure
recording of phase 2 (a new player named Tester, the Adventure button, Easy, the hint box, then the port's own typist playing
the first levels), cut at update PERF_END (7000 unless set) with a quit after it. "The stretch" is its last 2000 updates, 20
seconds of play at the game's 100 updates a second.

port-load starts the port without -fast with a script that takes one screenshot at update 1 and quits: its file time minus
the process's creation time is the load (the framework loads before the first update). port-minute plays the scenario; orig
plays the same one in the original under origrun (scripted cursor, a fresh users folder and registry key each run, origrun's
patch list for the registration check, the seed and the registry key). With PERF_CLOCK=real both play it in real time (the
port without -fast, the original on origrun's real clock); without it neither waits (the port's -fast, origrun's virtual
clock), so the processor time is the work done. The original loads in a thread on the wall clock: its script first waits for
the app's loading-completed flag (app+0x525, the app a static pointer at 0x565380), takes `mark` there and counts every
update from it, so the first typed letter always meets a loaded game, and the load time is the time from the process's
creation to that flag, read out of the game's memory every few milliseconds. orig starts origrun directly: origrun puts the game
under rundesk itself.

Both sample the game's process every 0.1 s: working set, private bytes (and the system's own peaks of both), processor time
(kernel + user). Each run prints one JSON line: the load, the peaks and the processor time over the whole run and over the
scripted stretch (the port marks its first update with a screenshot's file time, the original's is found from its update
counter, the app's +0x4b8), the machine's busy fraction during the run, and the game processes of other sessions running when
it started and ended. The original's run is stopped from here once its counter passes the script's last update, because its
`quit` does not end the game. pairs.sh runs the two turn about.
"""

import ctypes
import ctypes.wintypes as W
import json
import os
import glob
import re
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
    return {'whole_run': whole, 'scripted_stretch': span(lo, hi)}




def mine_prefixes():
    return [os.getcwd() + '\\']


DEFAULT_SCENARIO = os.path.join('build', 'ts-p2', 'rec', 'rt2.in.txt')
STRETCH = int(os.environ.get('PERF_STRETCH', '2000'))        # updates of the scripted stretch: 20 s at the game's 100 updates a second
REAL = os.environ.get('PERF_CLOCK') == 'real'                # the scenario in real time, for both games
INTERVAL = 0.1 if REAL else 0.02                             # between samples of the process (a run without waiting is over in seconds)
TERMINATE = 0x1

# The original: the app is the static pointer at 0x565380 (GAME_NOTES.md, section 8); its update counter is +0x4b8 and the loading
# thread's finished flag +0x525.
APP = 0x565380
COUNTER_EXPR = '[0x565380]+0x4b8'
COUNTER_OFFSET, LOADED_OFFSET = 0x4B8, 0x525
STATE_EXPRS = ('[[0x565380]+0x634]+0x64 [[0x565380]+0x634]+0x1c4 [[[0x565380]+0x634]+0xd68]+0xf4 '
               '[[[0x565380]+0x634]+0xd68]+0x110')
# waitfor reads a dword: the flag is the top byte of the one at +0x522 (the dword at +0x525 already reads 0x10000 at the start)
LOADED_WAIT = '[0x565380]+0x522 >= 0x1000000 120000'
GAME = os.path.join('build', 'origrun', 'typershark', 'game')


def scenario(path):
    """The lines (update, command, arguments) of a port script (`<update> char|click|... args`) up to update PERF_END (7000 unless
    set), and that end."""
    lines = []
    for l in open(path):
        w = l.split('#')[0].split()
        if len(w) < 2 or w[1] == 'quit':
            continue
        lines.append((int(w[0]), w[1], w[2:]))
    cap = int(os.environ.get('PERF_END', '7000'))
    return [l for l in lines if l[0] <= cap], cap


def port_script(src, dst):
    """The scenario as the port's -script lines, with a quit at its end."""
    lines, end = scenario(src)
    with open(dst, 'w', newline='\n') as f:
        for n, c, a in lines:
            f.write('%d %s %s\n' % (n, c, ' '.join(a)))
        f.write('%d quit\n' % end)
    return dst, end


def orig_script(src, dst):
    """The scenario as origrun's script lines (tools/phase2/p2lib.py: the port's line N is origrun's update N - 1, a space is key 32),
    behind a wait for the loading thread's flag, from which the updates count (`mark`)."""
    lines, end = scenario(src)
    steps = []
    for n, c, a in lines:
        u = n - 1
        if c == 'char':
            steps.append((u, 'key 32' if a[0] == 'SPACE' else 'text %s' % a[0]))
        else:
            steps.append((u, '%s %s' % (c, ' '.join(a))))
    # the board's state at the stretch's start and end (phase 2's peeks: the board, its +0x64 and +0x1c4, two fields of the sharks'
    # list), to see that two runs played the same game; in their place among the inputs, as a line waits for its update
    for u in (end - 1 - STRETCH, end - 1):
        steps.append((u, 'peek %s %s' % (COUNTER_EXPR, STATE_EXPRS)))
    with open(dst, 'w', newline='\n') as f:
        f.write('at 0 focus regain\nwaitfor %s\nmark\n' % LOADED_WAIT)
        for u, s in sorted(steps, key=lambda x: x[0]):
            f.write('at +%d %s\n' % (u, s))
        f.write('quit\n')
    return dst, end


def port_exe():
    root = os.getcwd()
    # PERF_EXE names another build of the port in build/native (an exe kept from before a change)
    return os.path.join(root, 'build', 'native', os.environ.get('PERF_EXE', 'typershark.exe')), os.path.join(root, 'assets', 'typershark')


def fresh_users(d):
    """A users folder for the port: the clean fixture of phase 1 (no player), as every phase 2 run starts from."""
    shutil.rmtree(d, ignore_errors=True)
    shutil.copytree(os.path.join('build', 'ts-p1', 'fixtures', 'port-win'), d)


def port_run(out, extra):
    exe, gamedir = port_exe()
    ud = os.path.join(out, 'ud')
    fresh_users(ud)
    sc = os.path.join(out, 'script.txt')
    others0 = game_processes(mine_prefixes())
    s0 = system_times()
    log = open(os.path.join(out, 'game.log'), 'wb')
    p = subprocess.Popen([exe, '-gamedir=' + gamedir, '-userdir=' + ud, '-seed=1', '-script=' + sc] + extra, cwd=out,
                         stdout=log, stderr=subprocess.STDOUT)
    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
    created = creation_time(h)
    samples = []
    while p.poll() is None:
        s = sample(h)
        if s:
            samples.append((round(time.time() - created, 3),) + s)
        time.sleep(INTERVAL)
    s1 = system_times()
    return p, h, created, samples, others0, s0, s1


def port_load(out, runs):
    """The port's load: the process's creation to the first update, whose screenshot's file time marks it."""
    out = os.path.abspath(out)
    results = []
    for i in range(runs):
        d = os.path.join(out, 'load%d' % i)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        mark = os.path.join(d, 'mark.png')
        open(os.path.join(d, 'script.txt'), 'w', newline='\n').write('1 screenshot %s\n3 quit\n' % mark.replace('\\', '/'))
        p, h, created, samples, others0, s0, s1 = port_run(d, [])
        at = os.path.getmtime(mark) - created if os.path.exists(mark) else None
        snap = next((x for x in samples if at is not None and x[0] >= at), None)
        r = {'load_s': round(at, 3) if at is not None else None, 'exit': p.returncode,
             'working_set_mb_at_title': round(snap[1] / 1048576, 1) if snap else None,
             'private_mb_at_title': round(snap[2] / 1048576, 1) if snap else None,
             'processor_s_at_title': round(snap[5], 2) if snap else None,
             'machine_busy': round(busy_fraction(s0, s1), 2), 'others_start': len(others0),
             'others_end': len(game_processes(mine_prefixes()))}
        k32.CloseHandle(h)
        results.append(r)
        print(json.dumps(r), flush=True)
    t = [r['load_s'] for r in results if r['load_s'] is not None]
    if t:
        print(json.dumps({'port_load_s': t, 'median_s': round(statistics.median(t), 3)}))


def port_minute(out, src):
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    sc, end = port_script(src, os.path.join(out, 'script.txt'))
    mark = os.path.join(out, 'mark.png')
    with open(sc, 'a', newline='\n') as f:     # one picture at the stretch's first update marks the time it began
        f.write('%d screenshot %s\n' % (end - STRETCH, mark.replace('\\', '/')))
    # without waiting: -fast draws every tenth update, the original on origrun's virtual clock every update (PERF_DRAW=every: -drawevery)
    p, h, created, samples, others0, s0, s1 = port_run(out, [] if REAL else ['-fast'] + (['-drawevery'] if os.environ.get('PERF_DRAW') == 'every' else []))
    run = samples[-1][0] if samples else 0
    begin = os.path.getmtime(mark) - created if os.path.exists(mark) else None
    res = summarise(samples, begin if begin is not None else run - 20)
    res.update({'exit': p.returncode, 'run_s': round(run, 2), 'machine_busy': round(busy_fraction(s0, s1), 2),
                'stretch_start_marked': begin is not None, 'updates_in_run': end, 'updates_in_stretch': STRETCH,
                'updates_per_s_in_stretch': round(STRETCH / (run - begin), 1) if begin is not None and run > begin else None,
                'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
    json.dump({'samples': samples, 'result': res}, open(os.path.join(out, 'perf.json'), 'w'))
    k32.CloseHandle(h)
    print(json.dumps(res))


def web_minute(out, src):
    """The web build (build/web/typershark.js) under Node, as the browser's code runs but with no canvas, sound or GPU: the scenario
    with -fast -drawevery (a draw every update, as the original does), a memreport at its start of play and end, and the node
    process sampled like a game. Node's own memory (about 40 MB) is in the figures: the report's heap size is the game's."""
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'mem'))
    fresh_users(os.path.join(out, 'ud'))
    lines, end = scenario(src)
    sc = os.path.join(out, 'script.txt')
    with open(sc, 'w', newline='\n') as f:
        for n, c, a in sorted([(n, c, a) for n, c, a in lines] + [(m, 'memreport', ['mem/mem%d.txt' % m]) for m in (300, end - STRETCH, end - 1)],
                              key=lambda x: x[0]):
            f.write('%d %s %s\n' % (n, c, ' '.join(a)))
        f.write('%d quit\n' % end)
    mark = 'mem/mark.png'
    with open(sc, 'a', newline='\n') as f:
        f.write('%d screenshot %s\n' % (end - STRETCH, mark))
    node = sorted(glob.glob(os.environ.get('EMSDK_ROOT', 'E:/retools/emsdk') + '/node/*/node.exe'))[0]
    wasmrun = os.path.join(os.getcwd(), 'games', 'bejeweled', 'tools', 'verify', 'wasmrun.js')
    others0 = game_processes(mine_prefixes())
    s0 = system_times()
    log = open(os.path.join(out, 'run.out'), 'wb')
    p = subprocess.Popen([node, wasmrun, 'game', os.path.join(os.getcwd(), 'build', 'web', 'typershark.js'), '-fast', '-drawevery', '-seed=1',
                          '-gamedir=' + os.path.join(os.getcwd(), 'assets', 'typershark'), '-userdir=ud', '-script=script.txt', '--timeout', '1500'],
                         cwd=out, stdout=log, stderr=subprocess.STDOUT)
    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
    created = creation_time(h)
    samples = []
    while p.poll() is None:
        s = sample(h)
        if s:
            samples.append((round(time.time() - created, 3),) + s)
        time.sleep(INTERVAL)
    s1 = system_times()
    run = samples[-1][0] if samples else 0
    marked = os.path.join(out, 'mem', 'mark.png')
    begin = os.path.getmtime(marked) - created if os.path.exists(marked) else None
    res = summarise(samples, begin if begin is not None else run - 20)
    heaps = re.findall(r'MEMREPORT (\{.*\})', open(os.path.join(out, 'run.out'), errors='replace').read())
    res.update({'exit': p.returncode, 'run_s': round(run, 2), 'machine_busy': round(busy_fraction(s0, s1), 2), 'updates_in_run': end,
                'updates_in_stretch': STRETCH, 'stretch_start_marked': begin is not None,
                'updates_per_s_in_stretch': round(STRETCH / (run - begin), 1) if begin is not None and run > begin else None,
                'memreports': [json.loads(x) for x in heaps],
                'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
    json.dump({'samples': samples, 'result': res}, open(os.path.join(out, 'perf.json'), 'w'))
    k32.CloseHandle(h)
    print(json.dumps(res))


def orig_run(out, src, runs, load_only=False):
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    exe = os.path.join(root, GAME, 'TyperSharkGame.exe')
    # the registry check and the seed, not phase 2's loading patch (PERF_PATCHES=build/ts-p2/patches.txt adds it, to check that the
    # two ways of loading play the same game)
    patches = os.path.join(root, os.environ.get('PERF_PATCHES', os.path.join('tools', 'origrun', 'patches', 'typershark.txt')))
    results = []
    for i in range(runs):
        d = os.path.join(out, 'orig%d' % i)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        sc, end = orig_script(src, os.path.join(d, 'script.txt'))
        if load_only:        # the real clock: the game loads, and is stopped at the flag
            open(sc, 'w', newline='\n').write('at 0 focus regain\nwaitfor %s\nquit\n' % LOADED_WAIT)
        # a fresh game: no players (the fixture of phase 1), no registry values
        users = os.path.join(root, GAME, 'users')
        shutil.rmtree(users, ignore_errors=True)
        shutil.copytree(os.path.join(root, 'build', 'ts-p1', 'fixtures', 'users-clean'), users)
        subprocess.run(['reg', 'delete', r'HKCU\Software\PopCap\TyperShark', '/f'], capture_output=True)
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        log = open(os.path.join(d, 'origrun.log'), 'wb')
        p = subprocess.Popen([origrun, '--exe', exe, '--run', 'typershark', '--script', sc,
                              '--clock', 'real' if (load_only or REAL) else 'virtual', '--cursor', 'scripted', '--reg-map', 'popcap',
                              '--seed', '1', '--counter', COUNTER_EXPR, '--patches', patches, '--timeout', '900'],
                             stdout=log, stderr=subprocess.STDOUT)
        h = None
        created = ready = None
        ready_mem = None
        c_ready = None
        samples, counters = [], []
        next_sample = 0
        done = False
        deadline = time.time() + 1200
        while p.poll() is None and not done and time.time() < deadline:
            if h is None:
                pid = find_process('TyperSharkGame.exe', '\\' + GAME.replace('/', '\\') + '\\')
                if pid:
                    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED | TERMINATE, False, pid)
                    created = creation_time(h)
            if h:
                now = time.time()
                if ready is None:
                    app = read32(h, APP)
                    flag = read32(h, app + LOADED_OFFSET) if app else None
                    if flag is not None and (flag & 0xff):
                        ready = now - created
                        ready_mem = sample(h)
                        c_ready = read32(h, app + COUNTER_OFFSET)
                        if load_only:
                            break
                if now >= next_sample:
                    s = sample(h)
                    app = read32(h, APP)
                    c = read32(h, app + COUNTER_OFFSET) if app else None
                    if s:
                        samples.append((round(now - created, 3),) + s)
                        counters.append((round(now - created, 3), c))
                    next_sample = now + INTERVAL
                    if ready is not None and c is not None and c_ready is not None and c >= c_ready + end + 300:
                        done = True             # the script's final mark has run: its quit ends the run, but the sample is ours
            time.sleep(0.004 if ready is None else 0.005)
        if h:
            k32.TerminateProcess(h, 0)
        s1 = system_times()
        try:
            p.wait(30)
        except subprocess.TimeoutExpired:
            p.terminate()
        log.close()
        # the stretch: the counter at the script's first `mark` (origrun's own line in the log) plus the stretch's bounds
        text = open(os.path.join(d, 'origrun.log'), errors='replace').read()
        marks = [int(m) for m in re.findall(r'mark: the counter reads (\d+)', text)]
        c_mark = marks[0] if marks else None
        # the board's state at the stretch's start and end (STATE_EXPRS' values): a run that did not get into play (the unpatched load
        # leaves the title screen in a state that varies with the counter at the flag) has no sharks (+0x1c4 is 0) and measures nothing
        states = [[int(v) for v in re.findall(r'=(-?\d+)', line.split('peek u=')[1])[1:]] for line in text.split('\n') if 'peek u=' in line]
        played = bool(states) and states[-1][1] != 0
        run = samples[-1][0] if samples else 0
        lo = hi = None
        if c_mark is not None:
            lo = next((t for t, c in counters if c is not None and c >= c_mark + end - 1 - STRETCH), None)
            hi = next((t for t, c in counters if c is not None and c >= c_mark + end - 1), None)
        res = summarise(samples, lo if lo is not None else run - 20, hi)
        if lo is not None and hi is not None and hi > lo:
            c_lo = next(c for t, c in counters if t == lo)
            c_hi = next(c for t, c in counters if t == hi)
            res['updates_in_stretch'] = c_hi - c_lo
            res['updates_per_s_in_stretch'] = round((c_hi - c_lo) / (hi - lo), 1)
        res.update({'exit': p.returncode, 'run_s': round(run, 2), 'machine_busy': round(busy_fraction(s0, s1), 2),
                    'load_s': round(ready, 3) if ready else None, 'counter_at_loaded': c_ready, 'counter_at_mark': c_mark,
                    'last_update_reached': counters[-1][1] if counters else None, 'updates_in_run': end,
                    'game_played': (played if not load_only else None), 'state_at_stretch_start_and_end': states,
                    'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
        if ready_mem:
            res.update({'working_set_mb_at_loaded': round(ready_mem[0] / 1048576, 1),
                        'private_mb_at_loaded': round(ready_mem[1] / 1048576, 1), 'processor_s_at_loaded': round(ready_mem[4], 2)})
        json.dump({'samples': samples, 'counters': counters, 'result': res}, open(os.path.join(d, 'perf.json'), 'w'))
        if h:
            k32.CloseHandle(h)
        results.append(res)
        print(json.dumps(res), flush=True)


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else ''
    scn = sys.argv[3] if len(sys.argv) > 3 and not sys.argv[3].isdigit() else DEFAULT_SCENARIO
    if mode == 'port-load':
        port_load(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 3)
    elif mode == 'port-minute':
        port_minute(sys.argv[2], scn)
    elif mode == 'web-minute':
        web_minute(sys.argv[2], scn)
    elif mode == 'orig':
        orig_run(sys.argv[2], scn, int(sys.argv[4]) if len(sys.argv) > 4 else 1, len(sys.argv) > 5 and sys.argv[5] == 'load')
    else:
        sys.exit(__doc__)
