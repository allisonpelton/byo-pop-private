"""Load time, memory and processor time of Mummy Maze, the port and the original, for GAME_NOTES.md "Performance".

From the worktree root:

    build/native/rundesk.exe <python.exe> games/mummymaze/tools/profile/perf.py port-load <out dir> [runs]
    build/native/rundesk.exe <python.exe> games/mummymaze/tools/profile/perf.py port-minute <out dir> <port script>
    <python.exe> games/mummymaze/tools/profile/perf.py orig <out dir> <origrun script> [runs] [load]    (not under rundesk;
                                                                                                  "load" stops at the title)

port-load starts the port without -fast, with the live command channel (as the real-input driver does), and asks `move`
until it answers with the kind "title": the title screen's play link is up, the first screen that takes input. Its time
counts from the creation of the process. port-minute plays a scripted game (the phase-two scenario c6 without its screenshots)
in real time. orig does the same for the original under origrun (real clock, scripted cursor, a fresh copy and a reset play
count each run), reading the same flag (the title screen's "play shown" byte) out of the game's memory every few
milliseconds, as origrun's --watch does, because origrun's own output reaches a pipe late. orig starts origrun directly:
origrun puts the game under rundesk itself.

Both sample the game's process every 0.1 s: working set, private bytes (and the system's own peaks of both), processor time
(kernel + user). Each run prints one JSON line: the first screen's time, and the peaks and the processor time over the whole
run and over the scripted minute (the 6000 updates before the script's last update, the same moves in both: the port marks the
first of them with a screenshot's file time, the original's is found from its update counter at app+0x328), the machine's busy
fraction during the run, and the game processes of other sessions running when it started and ended. The original's run is
stopped from here once its counter passes the script's last update, because its `quit` does not end the game.
The original needs origrun's git-ignored patch list (the registration check answers yes; srand stores the run's seed) and
config (counter=0x49c4c0, update_ms=10) under tools/origrun/.
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


def mine_prefixes():
    return [os.getcwd() + '\\', 'C:\\byo-og\\mmperf']


def strip_script(src, dst, drop):
    with open(src) as f, open(dst, 'w', newline='\n') as g:
        for line in f:
            if not any(d in line for d in drop):
                g.write(line)
    return dst


MINUTE = 6000  # updates of the scripted minute: 60 s at the game's 100 updates a second


def script_span(path):
    """The first and last update numbers of a script (the port's lines start with one, the original's with `at`)."""
    nums = []
    for line in open(path):
        w = line.split()
        if w and w[0] == 'at' and len(w) > 1:
            w = w[1:]
        if w and w[0].isdigit():
            nums.append(int(w[0]))
    return min(nums), max(nums)


def port_exe():
    root = os.getcwd()
    exe = os.environ.get('PORT_EXE') or os.path.join(root, 'build', 'native', 'mummymaze.exe')   # PORT_EXE: another build, such as the frozen baseline's
    return (exe, os.path.join(root, 'assets', 'mummymaze'))


def port_load(out, runs):
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'realplay'))
    import argparse
    import drive
    exe, gamedir = port_exe()
    results = []
    for i in range(runs):
        d = os.path.join(out, 'load%d' % i)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(os.path.join(d, 'user'))
        a = argparse.Namespace(exe=exe, gamedir=gamedir, userdir=os.path.join(d, 'user'), out=d, fast=False, exe_args=[])
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        g = drive.Game(a)
        h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, g.proc.pid)
        created = creation_time(h)
        while True:
            kind, rest = g.cmd('move')
            if kind == 'MOVE' and '"kind":"title"' in rest:
                seen = time.time()
                break
            time.sleep(0.002)
        m = sample(h)
        s1 = system_times()
        others1 = game_processes(mine_prefixes())
        k32.CloseHandle(h)
        g.close()
        r = {'load_s': round(seen - created, 3), 'working_set_mb_at_title': round(m[0] / 1048576, 1),
             'private_mb_at_title': round(m[1] / 1048576, 1), 'processor_s_at_title': round(m[4], 2),
             'machine_busy': round(busy_fraction(s0, s1), 2), 'others_start': len(others0), 'others_end': len(others1)}
        results.append(r)
        print(json.dumps(r), flush=True)
    t = [r['load_s'] for r in results]
    print(json.dumps({'port_load_s': t, 'median_s': round(statistics.median(t), 3)}))


def port_minute(out, script):
    exe, gamedir = port_exe()
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    script = strip_script(script, os.path.join(out, 'script.txt'), ['screenshot'])
    first, last = script_span(script)
    mark = os.path.join(out, 'mark.png')
    with open(script, 'a', newline='\n') as f:     # one picture at the minute's first update marks the time it began
        f.write('%d screenshot %s\n' % (last - MINUTE, mark.replace('\\', '/')))
    others0 = game_processes(mine_prefixes())
    s0 = system_times()
    log = open(os.path.join(out, 'game.log'), 'wb')
    p = subprocess.Popen([exe, '-gamedir=' + gamedir, '-userdir=' + os.path.join(out, 'user'), '-seed=21', '-script=' + script],
                         cwd=out, stdout=log, stderr=subprocess.STDOUT)
    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
    created = creation_time(h)
    samples = []
    while p.poll() is None:
        s = sample(h)
        if s:
            samples.append((round(time.time() - created, 3),) + s[:2] + s[2:])
        time.sleep(0.1)
    end = samples[-1][0] if samples else 0
    s1 = system_times()
    begin = os.path.getmtime(mark) - created if os.path.exists(mark) else None
    res = summarise(samples, begin if begin is not None else end - 60)
    res.update({'exit': p.returncode, 'run_s': round(end, 1), 'machine_busy': round(busy_fraction(s0, s1), 2),
                'minute_start_marked': begin is not None, 'updates_in_minute': MINUTE,
                'updates_per_s_in_minute': round(MINUTE / (end - begin), 1) if begin is not None else None,
                'others_start': len(others0), 'others_end': len(game_processes(mine_prefixes()))})
    json.dump({'samples': samples, 'result': res}, open(os.path.join(out, 'perf.json'), 'w'))
    k32.CloseHandle(h)
    print(json.dumps(res))


# The original's title screen: the app (static at 0x49c198) holds the title screen at +0x10398, and its flag for the play
# link being shown is at +0x68 of that (what the identical-play passes watch); its update counter is at +0x328
TITLE_PTR, PLAY_SHOWN, UPDATE_COUNT = 0x4AC530, 0x68, 0x49C4C0
TERMINATE = 0x1


def orig_run(out, script, runs, load_only=False):
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    run = 'mmperf'
    results = []
    for i in range(runs):
        d = os.path.join(out, 'orig%d' % i)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        sc = strip_script(script, os.path.join(d, 'script.txt'), [' shot '])
        first, last = script_span(sc)
        shutil.rmtree('C:/byo-og/' + run, ignore_errors=True)
        shutil.rmtree(os.path.join(root, 'build', 'origrun', run), ignore_errors=True)
        # the play count: the original shows its register nag once it reaches ten, and each quit adds one
        subprocess.run(['reg', 'add', 'HKLM\\Software\\WOW6432Node\\PopCap\\MummyMaze', '/v', 'TimesPlayed', '/t', 'REG_DWORD',
                        '/d', '0', '/f'], capture_output=True)
        others0 = game_processes(mine_prefixes())
        s0 = system_times()
        log = open(os.path.join(d, 'origrun.log'), 'wb')
        t_start = time.time()
        p = subprocess.Popen([origrun, '--game', 'mummymaze', '--run', run, '--stage-root', 'C:/byo-og', '--script', sc,
                              '--clock', 'real', '--cursor', 'scripted', '--seed', '21', '--timeout', '400'],
                             stdout=log, stderr=subprocess.STDOUT)
        h = None
        created = ready = spawned = None
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
                pid = find_process('WinMM.exe', '\\byo-og\\' + run + '\\')
                if pid:
                    h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED | TERMINATE, False, pid)
                    created = creation_time(h)
                    spawned = created - t_start
            if h:
                now = time.time()
                if ready is None:
                    ptr = read32(h, TITLE_PTR)
                    if ptr:
                        flag = read32(h, ptr + PLAY_SHOWN)
                        if flag is not None and (flag & 0xFF):
                            ready = now - created
                            ready_mem = sample(h)
                if now >= next_sample:
                    s = sample(h)
                    c = read32(h, UPDATE_COUNT)
                    if s:
                        samples.append((round(now - created, 3),) + s)
                        counters.append((round(now - created, 3), c))
                    next_sample = now + 0.1
                    if c is not None and c >= last + 30:   # the script's quit has run: the original keeps running, so stop it here
                        done = True
            time.sleep(0.004 if ready is None else 0.02)
        if h:
            k32.TerminateProcess(h, 0)
        s1 = system_times()
        try:
            p.wait(30)
        except subprocess.TimeoutExpired:
            p.terminate()
        # the minute: from the sample where the counter reached the minute's first update to the one at the last update
        lo = next((t for t, c in counters if c is not None and c >= last - MINUTE), None)
        hi = next((t for t, c in counters if c is not None and c >= last), None)
        end = samples[-1][0] if samples else 0
        res = summarise(samples, lo if lo is not None else end - 60, hi)
        if lo is not None and hi is not None:
            c_lo = next(c for t, c in counters if t == lo)
            c_hi = next(c for t, c in counters if t == hi)
            res['updates_in_minute'] = c_hi - c_lo
            res['updates_per_s_in_minute'] = round((c_hi - c_lo) / (hi - lo), 1)
        res.update({'exit': p.returncode, 'run_s': round(end, 1), 'machine_busy': round(busy_fraction(s0, s1), 2),
                    'load_s': round(ready, 3) if ready else None, 'origrun_to_process_s': round(spawned, 1) if h else None,
                    'last_update_reached': counters[-1][1] if counters else None,
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
        port_minute(sys.argv[2], sys.argv[3])
    elif mode == 'orig':
        orig_run(sys.argv[2], sys.argv[3], int(sys.argv[4]) if len(sys.argv) > 4 else 1, len(sys.argv) > 5 and sys.argv[5] == 'load')
    else:
        sys.exit(__doc__)
