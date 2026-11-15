"""Load time, memory and processor time of Bonnie's Bookstore, the port and the original, for GAME_NOTES.md "Performance".

From the worktree root:

    build/native/rundesk.exe <python.exe> games/bonnies/tools/perf.py port <out dir> [runs]   (BB_EXE=<exe>: another build,
                                                                                           BB_ARGS=<arguments>: more arguments, e.g. -windowscale=1)
    <python.exe> games/bonnies/tools/perf.py orig <out dir> <placed copy folder> [runs]     (not under rundesk: origrun
                                                                                           puts the game there itself)

Both play the same tour, the port's `tools/tour_test.txt` by its updates and the original's `tools/origrun/tour.txt` by the
clock (logos, the menu, a new player, the difficulty, the story, the tutorial's words and on), without their screenshots,
in real time (the port without -fast, the original on origrun's real clock with its seed pinned as the port's -seed=1).
"Load" is the time from the process's creation to the main menu, the first screen that takes input other than a click
that skips a logo; nobody clicks the logos, so both wait out the same logo times (the game's own, from its data) and the
difference between the two is the loading. The port's screen comes from a `state` command the script gives every 10 updates
(a `STATE ...` line on stdout; the port polls no -commands file, which would add a stat() to every update that a player's
game does not have), the original's from its memory (the application at 0x4752a0, its state at +0x200, 2 for the menu),
read every few milliseconds.

The scripted minute is the 60 s from the first board (the tutorial's first level, its words played by the tour), state 3
in both. The original's tour clicks by the clock, so its script waits 15 s before its first click, past the slowest menu
seen (12 s), and its minute is anchored at the board rather than at a time. Each run samples the game's process every 0.1 s (working set,
private bytes, kernel plus user time, and the cycles its threads ran) and prints one JSON line: the load time, the memory at
the menu, the peaks, the processor time and its cycles over the minute (`minute_processor_s`, `minute_kernel_s`,
`minute_gcycles`), the machine's busy fraction and the game processes of other sessions at the start and end. The cycles are
the figure to compare on a busy machine: it stretches a process's seconds (a shared core, a slower clock), not the cycles it
runs; even so the same exe's minutes differ by up to 30 % when the machine is busy, so compare rounds of runs by their
median and minimum (a busy machine only adds cycles), never one run. `minute_threads` lists the cycles each thread ran in
the minute, named by the module its start address is in (the game's own thread, the graphics driver's, SDL's audio, FMOD's),
so that the sound is counted apart. The run is stopped after the minute.
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
k32.TerminateProcess.argtypes = [W.HANDLE, W.UINT]
k32.QueryProcessCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t,
                                  ctypes.POINTER(ctypes.c_size_t)]
psapi.GetProcessMemoryInfo.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD]

QUERY_LIMITED, QUERY, VM_READ, TERMINATE = 0x1000, 0x400, 0x10, 0x1
EPOCH_DELTA = 11644473600  # seconds from 1601 to 1970
APP, STATE, MENU, BOARD = 0x4752A0, 0x200, 2, 3
MINUTE = 60.0
TOOLS = {'rundesk.exe', 'lsprobe.exe', 'origrun.exe', 'bejeweled_verify.exe', 'python.exe'}


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


class PE32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD),
                ('th32DefaultHeapID', ctypes.c_size_t), ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD),
                ('th32ParentProcessID', W.DWORD), ('pcPriClassBase', ctypes.c_long), ('dwFlags', W.DWORD),
                ('szExeFile', ctypes.c_char * 260)]


def seconds(t):
    return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7


def sample(h):
    """(working set, private bytes, processor seconds, kernel seconds, processor cycles) of the process, or None."""
    m = PMC()
    m.cb = ctypes.sizeof(PMC)
    if not psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb):
        return None
    c, e, k, u = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    if not k32.GetProcessTimes(h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(k), ctypes.byref(u)):
        return None
    cycles = ctypes.c_uint64()   # (the cycles the process's threads ran: a busy machine stretches its seconds, not these)
    k32.QueryProcessCycleTime(h, ctypes.byref(cycles))
    return m.WorkingSetSize, m.PrivateUsage, seconds(k) + seconds(u), seconds(k), cycles.value


class TE32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', ctypes.c_long), ('tpDeltaPri', ctypes.c_long), ('dwFlags', W.DWORD)]


k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.Thread32First.argtypes = [W.HANDLE, ctypes.POINTER(TE32)]
k32.Thread32Next.argtypes = [W.HANDLE, ctypes.POINTER(TE32)]
ntdll = ctypes.WinDLL('ntdll')
ntdll.NtQueryInformationThread.argtypes = [W.HANDLE, ctypes.c_int, ctypes.c_void_p, W.ULONG, ctypes.c_void_p]
psapi.EnumProcessModulesEx.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD, ctypes.POINTER(W.DWORD), W.DWORD]
psapi.GetModuleInformation.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, W.DWORD]
psapi.GetModuleBaseNameW.argtypes = [W.HANDLE, ctypes.c_void_p, W.LPWSTR, W.DWORD]
THREAD_QUERY, THREAD_QUERY_LIMITED = 0x40, 0x800


class MODINFO(ctypes.Structure):
    _fields_ = [('base', ctypes.c_void_p), ('size', W.DWORD), ('entry', ctypes.c_void_p)]


def threads(pid):
    """{thread id: (cycles the thread has run, its Win32 start address)} of the process, by the toolhelp snapshot."""
    snap = k32.CreateToolhelp32Snapshot(4, 0)
    te = TE32()
    te.dwSize = ctypes.sizeof(TE32)
    out = {}
    ok = k32.Thread32First(snap, ctypes.byref(te))
    while ok:
        if te.th32OwnerProcessID == pid:
            ht = k32.OpenThread(THREAD_QUERY | THREAD_QUERY_LIMITED, False, te.th32ThreadID)
            if ht:
                cycles = ctypes.c_uint64()
                k32.QueryThreadCycleTime(ht, ctypes.byref(cycles))
                start = ctypes.c_uint64()
                ntdll.NtQueryInformationThread(ht, 9, ctypes.byref(start), 8, None)   # ThreadQuerySetWin32StartAddress
                out[te.th32ThreadID] = (cycles.value, start.value)
                k32.CloseHandle(ht)
        ok = k32.Thread32Next(snap, ctypes.byref(te))
    k32.CloseHandle(snap)
    return out


def modules(h):
    """[(base, size, name)] of the process's modules (a 32-bit process's, seen from this 64-bit python)."""
    need = W.DWORD(0)
    arr = (ctypes.c_void_p * 1024)()
    if not psapi.EnumProcessModulesEx(h, arr, ctypes.sizeof(arr), ctypes.byref(need), 3):
        return []
    out = []
    for i in range(min(1024, need.value // ctypes.sizeof(ctypes.c_void_p))):
        info = MODINFO()
        buf = ctypes.create_unicode_buffer(260)
        if psapi.GetModuleInformation(h, arr[i], ctypes.byref(info), ctypes.sizeof(info)) and \
                psapi.GetModuleBaseNameW(h, arr[i], buf, 260):
            out.append((info.base or 0, info.size, buf.value))
    return out


def thread_report(h, first, last):
    """The threads that ran between two snapshots, as (module of the start address, thread id, gigacycles), busiest first."""
    mods = modules(h)
    rows = []
    for tid, (c1, start) in last.items():
        c0 = first.get(tid, (0, 0))[0]
        name = next((n for b, s, n in mods if b <= start < b + s), '?')
        rows.append((name, tid, round((c1 - c0) / 1e9, 3), hex(start)))
    return sorted(rows, key=lambda r: -r[2])


def created(h):
    c, e, k, u = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(k), ctypes.byref(u))
    return seconds(c) - EPOCH_DELTA


def system_times():
    i, k, u = W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetSystemTimes(ctypes.byref(i), ctypes.byref(k), ctypes.byref(u))
    return seconds(i), seconds(k) + seconds(u)  # the kernel time includes the idle time


def busy(a, b):
    return round(1.0 - (b[0] - a[0]) / (b[1] - a[1]), 2) if b[1] > a[1] else 0.0


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


def others():
    """The game processes of other sessions: exes in a byo-pop folder or a staged original, but tools and this worktree's."""
    mine = os.getcwd().lower() + '\\'
    n = 0
    for pid, name in processes():
        if name.lower() in TOOLS:
            continue
        path = image_path(pid).lower()
        if ('\\byo-pop\\' in path or '\\byo-og\\' in path) and not path.startswith(mine):
            n += 1
    return n


def read32(h, address):
    buf = ctypes.c_uint32(0)
    got = ctypes.c_size_t(0)
    if not k32.ReadProcessMemory(h, address, ctypes.byref(buf), 4, ctypes.byref(got)) or got.value != 4:
        return None
    return buf.value


def mb(x):
    return round(x / 1048576, 1)


def measure(h, t0, at_menu, out):
    """Samples the process from at_menu seconds after its creation t0 (the first board) through the minute after it."""
    samples = []
    pid = k32.GetProcessId(h)
    first = last = None
    while True:
        s = sample(h)
        if s is None:
            break
        t = time.time() - t0
        samples.append((round(t, 3),) + s)
        if t >= at_menu and first is None:
            first = threads(pid)    # the threads' cycles at the first board
        if t >= at_menu + MINUTE:
            last = threads(pid)     # and at the end of the minute
            break
        time.sleep(0.1)
    json.dump(samples, open(os.path.join(out, 'samples.json'), 'w'))
    m = [x for x in samples if x[0] >= at_menu]
    if len(m) < 2:
        return {'minute': None}
    by_thread = thread_report(h, first, last) if first and last else []
    json.dump(by_thread, open(os.path.join(out, 'threads.json'), 'w'))
    return {'minute_threads': [[n, c] for n, tid, c, a in by_thread if c >= 0.05],
            'board_working_set_mb': mb(m[0][1]), 'board_private_mb': mb(m[0][2]),
            'minute_peak_working_set_mb': mb(max(x[1] for x in m)), 'minute_peak_private_mb': mb(max(x[2] for x in m)),
            'minute_processor_s': round(m[-1][3] - m[0][3], 2), 'minute_kernel_s': round(m[-1][4] - m[0][4], 2),
            'minute_gcycles': round((m[-1][5] - m[0][5]) / 1e9, 2),
            'minute_wall_s': round(m[-1][0] - m[0][0], 1)}


def strip(src, dst, drop, replace=()):
    with open(src) as f, open(dst, 'w', newline='\n') as g:
        for line in f:
            for a, b in replace:
                line = line.replace(a, b)
            if not any(w in drop for w in line.split()[:2]):   # "<update> screenshot ..." or "shot <name>"
                g.write(line)
    return dst


def state_script(src, dst, shots_dir):
    """The port's tour without its screenshots and with a `state` command every 10 updates up to the first board, whose line
    on stdout tells perf.py when the menu and the board came. A player's game polls no file (-commands would stat one every
    update), so the port is measured without it, as the original is (read from outside through its memory)."""
    lines = []
    for line in open(src):
        words = line.split()
        if not words or words[0].startswith('#'):
            lines.append((-1, line.replace('@DIR@', shots_dir)))
        elif 'screenshot' in words[:2]:
            continue
        else:
            lines.append((int(words[0]), line.replace('@DIR@', shots_dir)))
    for n in range(300, 4500, 10):
        lines.append((n, '%d state\n' % n))
    lines.sort(key=lambda x: x[0])      # stable: a script's own lines at the same update keep their order
    with open(dst, 'w', newline='\n') as g:
        for n, line in lines:
            g.write(line)
    return dst


def port(out, runs):
    root = os.getcwd()
    exe = os.environ.get('BB_EXE') or os.path.join(root, 'build', 'native', 'bonnies.exe')   # BB_EXE: a variant to compare
    for i in range(runs):
        d = os.path.join(out, 'port%d' % i)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(os.path.join(d, 'user'))
        script = state_script(os.path.join(root, 'games', 'bonnies', 'tools', 'tour_test.txt'), os.path.join(d, 'script.txt'),
                              d.replace('\\', '/'))
        o0, s0 = others(), system_times()
        log = os.path.join(d, 'game.log')
        with open(log, 'wb') as lf:
            p = subprocess.Popen([exe, '-gamedir=assets/bonnies', '-userdir=' + os.path.join(d, 'user'), '-seed=1',
                                  '-script=' + script] + os.environ.get('BB_ARGS', '').split(), stdout=lf,
                                 stderr=subprocess.STDOUT)
        h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED | TERMINATE, False, p.pid)
        t0 = created(h)
        menu, seen = None, 0
        while menu is None and p.poll() is None and time.time() - t0 < 120:
            with open(log, encoding='latin-1') as f:
                f.seek(seen)
                text = f.read()
            cut = text.rfind('\n') + 1
            seen += len(text[:cut].encode('latin-1'))
            for line in text[:cut].splitlines():
                if line.startswith('STATE ') and '"screen":"menu"' in line:
                    menu = time.time() - t0
                    break
            time.sleep(0.005)
        board = None
        while menu is not None and board is None and p.poll() is None and time.time() - t0 < 240:
            with open(log, encoding='latin-1') as f:
                f.seek(seen)
                text = f.read()
            cut = text.rfind('\n') + 1
            seen += len(text[:cut].encode('latin-1'))
            if any(l.startswith('STATE ') and '"screen":"game"' in l for l in text[:cut].splitlines()):
                board = time.time() - t0
            time.sleep(0.05)
        r = {'side': 'port', 'load_s': round(menu, 2) if menu else None, 'board_s': round(board, 2) if board else None}
        if board:
            r.update(measure(h, t0, board, d))
        k32.TerminateProcess(h, 0)
        p.wait(30)
        k32.CloseHandle(h)
        loading = [l for l in open(log, encoding='latin-1') if 'Resource Loading Time' in l]
        r.update({'loading_line': loading[0].strip() if loading else None, 'machine_busy': busy(s0, system_times()),
                  'others_start': o0, 'others_end': others()})
        print(json.dumps(r), flush=True)


def orig(out, template, runs):
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    run = 'bonnies-perf'
    folder = os.path.join(root, 'build', 'origrun', run)
    for i in range(runs):
        d = os.path.join(out, 'orig%d' % i)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        # a fresh copy each run, so every run starts from the same saves and a first read of the files
        shutil.rmtree(folder, ignore_errors=True)
        shutil.copytree(template, os.path.join(folder, 'game'))
        script = strip(os.path.join(root, 'games', 'bonnies', 'tools', 'origrun', 'tour.txt'),
                       os.path.join(d, 'script.txt'), ['shot'], [('waitshot logo 20000', 'wait 9000')])
        o0, s0 = others(), system_times()
        with open(os.path.join(d, 'origrun.log'), 'wb') as lf:
            p = subprocess.Popen([origrun, '--exe', os.path.join(folder, 'game', 'BonniesGame.exe'), '--run', run,
                                  '--script', script, '--clock', 'real',
                                  '--patches', os.path.join(root, 'games', 'bonnies', 'tools', 'origrun', 'patches.txt'),
                                  '--seed', '1', '--timeout', '300'], stdout=lf, stderr=subprocess.STDOUT)
        h, menu, end = None, None, time.time() + 180
        while menu is None and p.poll() is None and time.time() < end:
            if h is None:
                for pid, name in processes():
                    if name.lower() == 'bonniesgame.exe' and folder.lower() in image_path(pid).lower():
                        h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED | TERMINATE, False, pid)
                        t0 = created(h)
                        break
            if h:
                app = read32(h, APP)
                if app and read32(h, app + STATE) == MENU:
                    menu = time.time() - t0
            time.sleep(0.004)
        board = None
        while menu is not None and board is None and p.poll() is None and time.time() < end + 120:
            app = read32(h, APP)
            if app and read32(h, app + STATE) == BOARD:
                board = time.time() - t0
            time.sleep(0.02)
        r = {'side': 'original', 'load_s': round(menu, 2) if menu else None, 'board_s': round(board, 2) if board else None}
        if board:
            r.update(measure(h, t0, board, d))
        if h:
            k32.TerminateProcess(h, 0)
            k32.CloseHandle(h)
        try:
            p.wait(30)
        except subprocess.TimeoutExpired:
            p.terminate()
        r.update({'machine_busy': busy(s0, system_times()), 'others_start': o0, 'others_end': others()})
        print(json.dumps(r), flush=True)


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else ''
    if mode == 'port':
        port(os.path.abspath(sys.argv[2]), int(sys.argv[3]) if len(sys.argv) > 3 else 1)
    elif mode == 'orig':
        orig(os.path.abspath(sys.argv[2]), os.path.abspath(sys.argv[3]), int(sys.argv[4]) if len(sys.argv) > 4 else 1)
    else:
        sys.exit(__doc__)
