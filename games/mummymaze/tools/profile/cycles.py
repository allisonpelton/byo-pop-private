"""Processor time like for like (docs/COORDINATION.md, "The target"): the port and the original over the same stretch of
play, in real time, as cycle counts. pairs.sh runs them in turn; from the worktree root:

    build/native/rundesk.exe <python.exe> -I games/mummymaze/tools/profile/cycles.py port <out dir> <port script> [seed]
    <python.exe> -I games/mummymaze/tools/profile/cycles.py orig <out dir> <origrun script> [seed]
                                                    (not under rundesk: origrun puts the game there itself)
    <python.exe> -I games/mummymaze/tools/profile/cycles.py summary <runs.txt>

The scripts are phase two's scenario c6 made by identical/p2.py (pairs.sh does it). The stretch is the 6000 updates that
follow the classic game's start click by 20: the port's updates 1020 to 7020, the original's 3321 to 9321 on its update
counter. Neither side runs test machinery in it: the port reads its input from a -script at start and gets no -commands
and no state log, and the original gets origrun's `at` input, which holds its window thread for an instant at each line.
The original's input is timed to its own update counter (by_updates): origrun's default counter for this game is its
sleep counter, which lags the updates by 2 to 7 % in a real-time run (an original behind by 5 ms makes up the update in
the same pass, without a sleep), so a key timed by it lands late in game time. The counters are read out of each game's
memory from outside, every 5 ms. At the stretch's two ends the process's cycles are read (QueryProcessCycleTime: every
thread's, those that ended included) and each thread's own (QueryThreadCycleTime, with the name a thread was given), so
that the sound can be counted apart: the original's FMOD runs two threads of its own (fmod.dll; its mixer is the second
thread of the original's cycles, the other the third), which mix into origrun's stand-in DirectSound without playing, as
the port's SDL audio thread (SDLAudioP2) does under rundesk's silent driver. Cycle counts, never GetProcessTimes or
GetThreadTimes, which Windows charges by sampling at the 15.6 ms clock tick. This process runs at idle priority, below
the game's (rundesk's below normal), so that its polling never keeps the game from its core.

Each run prints one JSON line: the stretch's cycles (G), its length in seconds, the updates, draws and sleeps in it, the
threads' cycles in it and, for the original, its windowed flag and the board's state at the two ends (identical/
identical.py's expressions: [board state, explorer x, y, score, chamber], to see that runs of the script played the same
game). The original's counters are the app's +0x330 (updates), +0x32c (draws) and +0x328 (sleeps); the port's are the
framework's mUpdateCount, mDrawCount and mSleepCount, +948, +944 and +940 of its app object (clang's
-fdump-record-layouts on MummyApp.cpp, 2026-11-12), found through gSexyAppBase.
How the original paces itself: its main loop (SexyApp vf03, 0x438310) does an update when 5 ms or more of the frame have
passed (up to ten in a row without a draw), else draws (vf05, 0x4375a0: only when a widget is dirty, or a second has
passed) and sleeps out the rest of the 10 ms frame. So it keeps 100 updates a second with one sleep an update when the
machine has the time, and when it wakes 5 ms late it makes the update up in the same pass.
CYC_VISIBLE=1 runs the port on the user's own desktop instead of under rundesk (ORACLE_VISIBLE, SDL's dummy audio
driver, normal priority); only between 22:00 and 08:00 (CLAUDE.md, "Overnight").
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
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.argtypes = [W.DWORD, W.DWORD]
for _f in ('Process32First', 'Process32Next', 'Thread32First', 'Thread32Next', 'Module32First', 'Module32Next'):
    getattr(k32, _f).argtypes = [W.HANDLE, ctypes.c_void_p]
k32.SetPriorityClass.argtypes = [W.HANDLE, W.DWORD]
k32.GetCurrentProcess.restype = W.HANDLE
k32.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, ctypes.POINTER(W.DWORD)]
k32.QueryProcessCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_ulonglong)]
k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_ulonglong)]
k32.GetThreadDescription.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_wchar_p)]
k32.LocalFree.argtypes = [ctypes.c_void_p]
k32.TerminateProcess.argtypes = [W.HANDLE, W.UINT]
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]

QUERY_LIMITED, VM_READ, TERMINATE, SYNCHRONIZE = 0x1000, 0x10, 0x1, 0x100000
THREAD_QUERY_LIMITED = 0x800

MINUTE = 6000       # the stretch's updates: 60 s at the game's 100 updates a second
PORT_FIRST = 1020   # c6's classic game start click is the port's update 1000; the stretch starts 20 updates later
ORIG_FIRST = PORT_FIRST + 2301   # the original's update counter for the same moment (identical/p2.py's OFFSET, 3321)


class PE32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD), ('th32ParentProcessID', W.DWORD),
                ('pcPriClassBase', ctypes.c_long), ('dwFlags', W.DWORD), ('szExeFile', ctypes.c_char * 260)]


class TE32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', ctypes.c_long), ('tpDeltaPri', ctypes.c_long), ('dwFlags', W.DWORD)]


class ME32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('th32ModuleID', W.DWORD), ('th32ProcessID', W.DWORD), ('GlblcntUsage', W.DWORD),
                ('ProccntUsage', W.DWORD), ('modBaseAddr', ctypes.c_void_p), ('modBaseSize', W.DWORD),
                ('hModule', ctypes.c_void_p), ('szModule', ctypes.c_char * 256), ('szExePath', ctypes.c_char * 260)]


def process_cycles(h):
    c = ctypes.c_ulonglong()
    return c.value if k32.QueryProcessCycleTime(h, ctypes.byref(c)) else None


class Threads:
    """Each thread of a process seen so far, with its handle held (a thread that ends can still be read) and its name."""

    def __init__(self, pid):
        self.pid, self.h, self.name = pid, {}, {}

    def look(self):
        snap = k32.CreateToolhelp32Snapshot(4, 0)
        e = TE32()
        e.dwSize = ctypes.sizeof(e)
        ok = k32.Thread32First(snap, ctypes.byref(e))
        while ok:
            tid = e.th32ThreadID
            if e.th32OwnerProcessID == self.pid and tid not in self.h:
                h = k32.OpenThread(THREAD_QUERY_LIMITED, False, tid)
                if h:
                    self.h[tid] = h
                    p = ctypes.c_wchar_p()
                    self.name[tid] = ''
                    if k32.GetThreadDescription(h, ctypes.byref(p)) >= 0 and p.value:
                        self.name[tid] = p.value
                        k32.LocalFree(p)
            ok = k32.Thread32Next(snap, ctypes.byref(e))
        k32.CloseHandle(snap)

    def cycles(self):
        self.look()
        out = {}
        for tid, h in self.h.items():
            c = ctypes.c_ulonglong()
            if k32.QueryThreadCycleTime(h, ctypes.byref(c)):
                out[tid] = c.value
        return out

    def close(self):
        for h in self.h.values():
            k32.CloseHandle(h)


def reading(h, threads):
    return {'t': time.perf_counter(), 'process': process_cycles(h), 'threads': threads.cycles()}


def stretch(a, b, names):
    """The cycles between two readings, the process's and each thread's (the busiest first, by name where it has one)."""
    rows = []
    for tid, c in b['threads'].items():
        d = c - a['threads'].get(tid, 0)
        if d > 0:
            rows.append((d, tid))
    rows.sort(reverse=True)
    total = b['process'] - a['process']
    return {'gcycles': round(total / 1e9, 3), 'seconds': round(b['t'] - a['t'], 2),
            'threads': [[names.get(tid) or str(tid), round(d / 1e9, 3)] for d, tid in rows if d >= 1e7],
            'other_threads_gcycles': round((total - sum(d for d, _ in rows)) / 1e9, 3)}


def read32(h, address):
    buf = ctypes.c_uint32(0)
    got = ctypes.c_size_t(0)
    if not k32.ReadProcessMemory(h, address, ctypes.byref(buf), 4, ctypes.byref(got)) or got.value != 4:
        return None
    return buf.value


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
    """Game processes running on the machine that are not this run's (tools/gamecount.py's count, roughly)."""
    n = 0
    mine = (os.getcwd().lower(), 'c:\\byo-og\\mmcyc\\')
    for pid, name in processes():
        if name.lower() in ('rundesk.exe', 'origrun.exe', 'python.exe', 'bash.exe', 'lsprobe.exe', 'bejeweled_verify.exe'):
            continue
        p = image_path(pid).lower()
        if ('\\byo-pop\\' in p or '\\byo-og\\' in p) and not any(m in p for m in mine):
            n += 1
    return n


def find_process(name, folder):
    for pid, n in processes():
        if n.lower() == name.lower() and folder.lower() in image_path(pid).lower():
            return pid
    return None


def module_base(pid, name):
    snap = k32.CreateToolhelp32Snapshot(0x18, pid)   # the process's modules, the 32-bit ones too
    if snap in (None, -1, 0xFFFFFFFFFFFFFFFF):   # a process that is still starting has none yet
        return None
    m = ME32()
    m.dwSize = ctypes.sizeof(m)
    ok = k32.Module32First(snap, ctypes.byref(m))
    base = None
    while ok:
        if m.szModule.decode('mbcs', 'replace').lower() == name:
            base = m.modBaseAddr
            break
        ok = k32.Module32Next(snap, ctypes.byref(m))
    k32.CloseHandle(snap)
    return base


def symbol_rva(exe, name):
    """A global's offset from the exe's image base, by llvm-nm (LLVM_NM, else llvm-mingw's) and the PE header."""
    nm = os.environ.get('LLVM_NM', 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-nm.exe')
    try:
        lines = subprocess.run([nm, exe], capture_output=True, text=True).stdout.splitlines()
    except OSError:
        return None
    addr = next((int(l.split()[0], 16) for l in lines if l.endswith(' ' + name)), None)
    data = open(exe, 'rb').read(4096)
    pe = int.from_bytes(data[0x3c:0x40], 'little')
    return addr - int.from_bytes(data[pe + 0x34:pe + 0x38], 'little') if addr else None


def strip_script(src, dst, drop, quit_at=None):
    """The script without the lines that hold any of `drop`; a port script's quit moves to update quit_at."""
    with open(src) as f, open(dst, 'w', newline='\n') as g:
        for line in f:
            w = line.split()
            if any(d in line for d in drop):
                continue
            if quit_at is not None and len(w) == 2 and w[1] == 'quit':
                line = '%d quit\n' % quit_at
            g.write(line)
    return dst


# the framework's SexyAppBase::mSleepCount, mDrawCount and mUpdateCount (clang's -fdump-record-layouts, 2026-11-12)
PORT_SLEEP, PORT_DRAW, PORT_UPDATE = 940, 944, 948


def port_counts(h, pid, app_rva, exe_name):
    """The port's (sleeps, draws, updates) so far, read from its app object (gSexyAppBase), or None."""
    base = module_base(pid, exe_name)
    app = read32(h, base + app_rva) if base else None
    if not app:
        return None
    c = tuple(read32(h, app + o) for o in (PORT_SLEEP, PORT_DRAW, PORT_UPDATE))
    return None if None in c else c


def idle():
    """This process at idle priority: the game (rundesk's below normal) is then never kept from the core by its poller."""
    k32.SetPriorityClass(k32.GetCurrentProcess(), 0x40)


def port(out, script, seed):
    idle()
    root = os.getcwd()
    exe = os.environ.get('PORT_EXE') or os.path.join(root, 'build', 'native', 'mummymaze.exe')
    gamedir = os.path.join(root, 'assets', 'mummymaze')
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    last = PORT_FIRST + MINUTE
    # no screenshots (they draw the screen again) and no more than the stretch's own updates, then the quit
    sc = strip_script(script, os.path.join(out, 'script.txt'), ['screenshot'], last + 50)
    n0 = others()
    log = open(os.path.join(out, 'game.log'), 'wb')
    # CYC_VISIBLE=1 (between 22:00 and 08:00 only, CLAUDE.md "Overnight"): the port on the user's own desktop, not under rundesk,
    # where its present meets a real vertical blank; SDL's dummy audio driver, so that it is as silent as under rundesk, and
    # normal priority, which the idle priority of this process would otherwise pass on
    visible = os.environ.get('CYC_VISIBLE') == '1'
    env = dict(os.environ, SDL_AUDIODRIVER='dummy', ORACLE_VISIBLE='1') if visible else None
    p = subprocess.Popen([exe, '-gamedir=' + gamedir, '-userdir=' + os.path.join(out, 'user'), '-seed=' + seed, '-script=' + sc],
                         cwd=out, stdout=log, stderr=subprocess.STDOUT, env=env, creationflags=0x20 if visible else 0)
    h = k32.OpenProcess(QUERY_LIMITED | VM_READ | SYNCHRONIZE | TERMINATE, False, p.pid)
    threads = Threads(p.pid)
    app_rva = symbol_rva(exe, '__ZN4Sexy12gSexyAppBaseE')
    exe_name = os.path.basename(exe).lower()
    marks, counts = [], []
    deadline = time.time() + 400
    while p.poll() is None and len(marks) < 2 and time.time() < deadline:
        time.sleep(0.005)
        try:
            c = port_counts(h, p.pid, app_rva, exe_name) if app_rva else None
        except (ctypes.ArgumentError, OverflowError):
            c = None
        if c is None:
            continue
        if c[2] >= PORT_FIRST + MINUTE * len(marks):
            marks.append(reading(h, threads))
            counts.append(c)
    try:
        p.wait(60)
    except subprocess.TimeoutExpired:
        k32.TerminateProcess(h, 0)
    log.close()
    res = {'side': 'port-visible' if visible else 'port', 'exit': p.returncode, 'others_start': n0, 'others_end': others()}
    if len(marks) == 2:
        res.update(stretch(marks[0], marks[1], threads.name))
        res['updates'] = counts[1][2] - counts[0][2]
        res['draws'] = counts[1][1] - counts[0][1]
        res['sleeps'] = counts[1][0] - counts[0][0]
        res['updates_per_s'] = round(res['updates'] / res['seconds'], 1)
    threads.close()
    k32.CloseHandle(h)
    print(json.dumps(res), flush=True)


# The original's app is the static object at 0x49c198: its sleep counter is +0x328 (0x49c4c0, origrun's counter), its draw
# counter +0x32c and its update counter +0x330 (vf03 0x438310 counts the sleeps, vf05 0x4375a0 the draws, vf04 0x437420
# the updates), and its windowed flag +0x230
APP = 0x49C198
SLEEPS, DRAWS, UPDATES, WINDOWED = APP + 0x328, APP + 0x32C, APP + 0x330, APP + 0x230
RUN = 'mmcyc'


CLICK = 3301   # the original's counter at the classic game's start click (the port's 1000 plus identical/p2.py's OFFSET)


def by_updates(src, dst):
    """origrun's script for the original with its input timed to the game's own update counter (the app's +0x330), from the
    sleep-timed script of identical/p2.py: the lines up to the start click keep their numbers, a `mark` follows the click,
    and every later line is `at +<updates after the click>`. The sleep counter lags the updates by 3 to 11 % in a real-time
    run (an original behind by 5 ms makes up the update in the same pass, without a sleep), so a key's `at N` by sleeps
    lands late in game time; by updates it lands where the port's does. No shots and no quit: the original's quit would
    close it at the script's last update, and the stretch's end is read from here, which stops it afterwards."""
    out, marked = [], False
    for line in open(src):
        w = line.split(None, 2)
        if len(w) < 3 or w[0] != 'at' or w[2].startswith(('shot', 'quit')):
            continue
        n = int(w[1])
        if marked:
            out.append('at +%d %s' % (n - CLICK, w[2].strip()))
        else:
            out.append(line.strip())
            if n == CLICK and w[2].startswith('click'):
                out.append('mark')
                marked = True
    with open(dst, 'w', newline='\n') as f:
        f.write('\n'.join(out) + '\n')
    return dst


def play_state(h):
    """What the original's board says now: [board state, explorer x, y, score, chamber] (identical/identical.py's expressions),
    so that runs of the same script can be seen to have played the same game; None where a read fails."""
    board = read32(h, 0x4AC54C)
    ex = read32(h, board + 0x60) if board else None
    vals = [read32(h, board + 0x5C) if board else None, read32(h, ex + 0x50) if ex else None,
            read32(h, ex + 0x54) if ex else None, read32(h, 0x4AC59C), read32(h, 0x4AC5BC)]
    return vals


def orig(out, script, seed):
    idle()
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    sc = by_updates(script, os.path.join(out, 'script.txt'))
    last = ORIG_FIRST + MINUTE
    shutil.rmtree('C:/byo-og/' + RUN, ignore_errors=True)
    shutil.rmtree(os.path.join(root, 'build', 'origrun', RUN), ignore_errors=True)
    # the play count: the original shows its register nag once it reaches ten, and each quit adds one
    subprocess.run(['reg', 'add', 'HKLM\\Software\\WOW6432Node\\PopCap\\MummyMaze', '/v', 'TimesPlayed', '/t', 'REG_DWORD',
                    '/d', '0', '/f'], capture_output=True)
    n0 = others()
    log = open(os.path.join(out, 'origrun.log'), 'wb')
    p = subprocess.Popen([origrun, '--game', 'mummymaze', '--run', RUN, '--stage-root', 'C:/byo-og', '--script', sc,
                          '--clock', 'real', '--cursor', 'scripted', '--counter', hex(UPDATES), '--seed', seed, '--timeout', '400'],
                         stdout=log, stderr=subprocess.STDOUT, creationflags=0x40)   # idle priority, below the game's
    h = threads = None
    marks, counts, windowed, plays = [], [], None, []
    deadline = time.time() + 420
    while p.poll() is None and time.time() < deadline:
        time.sleep(0.005)
        if h is None:
            pid = find_process('WinMM.exe', '\\byo-og\\' + RUN + '\\')
            if pid:
                h = k32.OpenProcess(QUERY_LIMITED | VM_READ | SYNCHRONIZE | TERMINATE, False, pid)
                threads = Threads(pid)
            continue
        u = read32(h, UPDATES)
        if u is None:
            continue
        if len(marks) < 2 and u >= ORIG_FIRST + MINUTE * len(marks):
            marks.append(reading(h, threads))
            counts.append((read32(h, SLEEPS), read32(h, DRAWS), u))
            plays.append(play_state(h))
            if windowed is None:
                windowed = (read32(h, WINDOWED) or 0) & 0xff
        if u >= last + 30:   # past the stretch's end: the original keeps running after the script's quit, so stop it here
            break
    if h:
        k32.TerminateProcess(h, 0)
    try:
        p.wait(30)
    except subprocess.TimeoutExpired:
        p.terminate()
    log.close()
    res = {'side': 'orig', 'exit': p.returncode, 'others_start': n0, 'others_end': others(), 'windowed': windowed, 'play': plays}
    if len(marks) == 2:
        res.update(stretch(marks[0], marks[1], threads.name))
        if None not in counts[0] + counts[1]:
            res['sleeps'] = counts[1][0] - counts[0][0]
            res['draws'] = counts[1][1] - counts[0][1]
            res['updates'] = counts[1][2] - counts[0][2]
            res['updates_per_s'] = round(res['updates'] / res['seconds'], 1)
    if threads:
        threads.close()
    if h:
        k32.CloseHandle(h)
    print(json.dumps(res), flush=True)


def summary(path):
    """The medians of runs.txt's pairs: each side's stretch cycles, updates a second, draws and the busiest threads."""
    rows = {'orig': [], 'port': []}
    for line in open(path):
        side, _, js = line.partition(' ')
        if side in rows and js.startswith('{'):
            rows[side].append(json.loads(js))
    for side, rs in rows.items():
        rs = [r for r in rs if 'gcycles' in r]
        if not rs:
            continue
        g = [r['gcycles'] for r in rs]
        print('%s: %d runs, G cycles %s, median %.3f; updates/s %s; draws %s; sleeps %s' % (
            side, len(rs), g, statistics.median(g), [r.get('updates_per_s') for r in rs], [r.get('draws') for r in rs],
            [r.get('sleeps') for r in rs]))
    if rows['orig'] and rows['port']:
        mo = statistics.median(r['gcycles'] for r in rows['orig'] if 'gcycles' in r)
        mp = statistics.median(r['gcycles'] for r in rows['port'] if 'gcycles' in r)
        print('port / original = %.2f (medians %.3f and %.3f G cycles)' % (mp / mo, mp, mo))


if __name__ == '__main__':
    if len(sys.argv) >= 3 and sys.argv[1] == 'summary':
        summary(sys.argv[2])
    elif len(sys.argv) >= 4 and sys.argv[1] in ('port', 'orig'):
        (port if sys.argv[1] == 'port' else orig)(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else '21')
    else:
        sys.exit(__doc__)
