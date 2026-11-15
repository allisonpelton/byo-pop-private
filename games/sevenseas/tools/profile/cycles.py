"""Processor time like for like (docs/COORDINATION.md, "The target"): the port and the original over the same stretch of
play, in real time, as cycle counts. pairs.sh runs them in turn; from the worktree root:

    build/native/rundesk.exe <python.exe> -I games/sevenseas/tools/profile/cycles.py port <out dir> perf_port.txt [seed]
    <python.exe> -I games/sevenseas/tools/profile/cycles.py orig <out dir> perf_orig.txt [seed]
                                                    (not under rundesk: origrun puts the game there itself)
    <python.exe> -I games/sevenseas/tools/profile/cycles.py summary <runs.txt>

The scripts are perf_port.txt and perf_orig.txt: the title, the menu, the Easy demo, then a broadside every 1.5 s. The
stretch is updates 3000 to 8500 of the update counter (55 s at the game's 100 updates a second). Neither side runs test
machinery in it: the port reads its input from a -script at start and gets no -commands and no state log, and the original
gets origrun's `at` input. Both are timed to the update counter (the original's is [0x4e21bc]+0x378, origrun's own counter
for this game), so the input lands on the same update on both sides. The counters are read from outside every 5 ms. At the
stretch's two ends the process's cycles are read (QueryProcessCycleTime: every thread's, those that ended included) and
each thread's own (QueryThreadCycleTime, with the name a thread was given), so the port's SDL audio thread can be counted
apart: origrun's stand-in takes the original's DirectSound, so the original's figure holds no mixing. Cycle counts, never
GetProcessTimes or GetThreadTimes, which Windows charges by sampling at the 15.6 ms clock tick. This process runs at idle
priority, below the game's (rundesk's below normal), so that its polling never keeps the game from its core.

Each run prints one JSON line: the stretch's cycles (G), its length in seconds, the updates, draws and sleeps in it and the
threads' cycles in it. The port's counters are the framework's mSleepCount, mDrawCount and mUpdateCount (+940, +944 and +948
of its app object, found through gSexyAppBase).
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

MINUTE = 5500   # the stretch: updates 3000 to 8500, 55 s at 100 updates a second, in the broadsides after the Easy demo
FIRST = 3000


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
    mine = (os.getcwd().lower(),)
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
    exe = os.environ.get('PORT_EXE') or os.path.join(root, 'build', 'native', 'sevenseas.exe')
    gamedir = os.path.join(root, 'assets', 'sevenseas')
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    # CYC_WINDOW=1x or 2x: windowed (registry ScreenMode=0 in the userdir's registry.ini), the window at the original's size (1x,
    # -windowscale=1) or the framework's default 2x; unset: a fresh userdir's own start (full screen). The display line of the
    # port's log says which it got.
    mode = os.environ.get('CYC_WINDOW', '')
    extra = []
    if mode in ('1x', '2x'):
        with open(os.path.join(out, 'user', 'registry.ini'), 'w', newline='\n') as f:
            f.write('[SOFTWARE\\PopCap\\SevenSeas]\nScreenMode=dword:0\n')
        if mode == '1x':
            extra = ['-windowscale=1']
    last = FIRST + MINUTE
    # no screenshots (they draw the screen again) and no more than the stretch's own updates, then the quit
    sc = strip_script(script, os.path.join(out, 'script.txt'), ['screenshot'], last + 50)
    n0 = others()
    log = open(os.path.join(out, 'game.log'), 'wb')
    # CYC_VISIBLE=1 (between 22:00 and 08:00 only, CLAUDE.md "Overnight"): the port on the user's own desktop, not under rundesk,
    # where its present meets a real vertical blank; SDL's dummy audio driver, so that it is as silent as under rundesk, and
    # normal priority, which the idle priority of this process would otherwise pass on
    visible = os.environ.get('CYC_VISIBLE') == '1'
    env = dict(os.environ, SDL_AUDIODRIVER='dummy', ORACLE_VISIBLE='1') if visible else None
    p = subprocess.Popen([exe, '-gamedir=' + gamedir, '-userdir=' + os.path.join(out, 'user'), '-seed=' + seed, '-script=' + sc] + extra,
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
        if c[2] >= FIRST + MINUTE * len(marks):
            marks.append(reading(h, threads))
            counts.append(c)
    try:
        p.wait(60)
    except subprocess.TimeoutExpired:
        k32.TerminateProcess(h, 0)
    log.close()
    res = {'side': 'port-visible' if visible else 'port', 'exit': p.returncode, 'others_start': n0, 'others_end': others()}
    res['window'] = mode or 'fresh'
    try:
        res['display'] = next(l.strip().partition('display: ')[2] for l in open(os.path.join(out, 'game.log'), errors='replace') if 'display: ' in l)
    except StopIteration:
        pass
    if len(marks) == 2:
        res.update(stretch(marks[0], marks[1], threads.name))
        res['updates'] = counts[1][2] - counts[0][2]
        res['draws'] = counts[1][1] - counts[0][1]
        res['sleeps'] = counts[1][0] - counts[0][0]
        res['updates_per_s'] = round(res['updates'] / res['seconds'], 1)
    threads.close()
    k32.CloseHandle(h)
    print(json.dumps(res), flush=True)


# The original's app object is the pointer at 0x4e21bc (origrun's --counter [0x4e21bc]+0x378): its update counter is +0x378;
# its sleep and draw counters are assumed to be the two words before it, as in Mummy Maze's framework (+0x370 sleeps, +0x374
# draws), and a run's updates, draws and sleeps a second show whether they are (a draw count above the update count, or a sleep
# count far from it, would mean they are not)
APP_PTR = 0x4E21BC
SLEEPS, DRAWS, UPDATES = 0x370, 0x374, 0x378
RUN = 'sscyc'
SCRIPT_FILES = {'port': 'perf_port.txt', 'orig': 'perf_orig.txt'}


LOADED = 0x3C4   # the dword holding the app's mLoaded byte (+0x3c6): the title shows its play link once it is set
BOARD, DEMO, MENU = 0x434, 0x430, 0x428   # the app's pointers to the board, the demo and the difficulty menu
BOARD_AT = 2400  # the port script's update of the demo's start click: the board exists from there
SKIP = 600       # the stretch starts this many updates after the board appears (the port's update 3000)


def orig_script(src, dst):
    """origrun's script, each step waiting for its screen: the original's load (a few seconds to a minute of wall time), the
    menu's and the demo's arrival are not on the update counter, so a click timed by it is often lost and the run sits on the
    title, the menu or the demo (the first five pairs held two runs of 700 draws, on the menu or demo, and three of 3700, in
    the demo, none on the board). So: wait for the app to be loaded, click the title's link, wait for the menu, click the
    difficulty (the port script's update 1000), wait for the demo, click its start button (update 2400), wait for the board,
    and then the script's lines from update 3000 on, as `at +<N - 2400>` from the board's appearance. The final quit is
    dropped (the stretch's end is read from outside, which stops the run afterwards)."""
    out = ['waitfor [[0x4e21bc]+0x424]+0x64 == 1072693248 240000', 'mark', 'at +100 move 200 385'] + ['at +%d click 200 385 1' % n for n in range(300, 1600, 100)] + [
           
           'waitfor [0x4e21bc]+0x%x != 0 120000' % MENU, 'mark', 'at +900 click 507 397 1',
           'waitfor [0x4e21bc]+0x%x != 0 120000' % DEMO, 'mark', 'at +400 move 390 440', 'at +420 click 390 440 1',
           'waitfor [0x4e21bc]+0x%x == 0 120000' % DEMO, 'mark']
    for line in open(src):
        w = line.split(None, 2)
        if len(w) < 3 or w[0] != 'at' or w[2].startswith(('shot', 'quit')) or int(w[1]) < 3000:
            continue
        out.append('at +%d %s' % (int(w[1]) - BOARD_AT, w[2].strip()))
    with open(dst, 'w', newline='\n') as f:
        f.write('\n'.join(out) + '\n')
    return dst



def orig(out, script, seed):
    idle()
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    sc = orig_script(script, os.path.join(out, 'script.txt'))
    shutil.rmtree(os.path.join(root, 'build', 'origrun', RUN), ignore_errors=True)
    n0 = others()
    log = open(os.path.join(out, 'origrun.log'), 'wb')
    # the sevenseas.cfg's counter is the update counter and its patches pin the seed; input is timed to that counter
    p = subprocess.Popen([origrun, '--game', 'sevenseas', '--run', RUN, '--script', sc, '--clock', 'real', '--seed', seed,
                          '--timeout', '400'], stdout=log, stderr=subprocess.STDOUT, creationflags=0x40)  # idle priority
    h = threads = None
    app = base = None
    seen_demo = False
    marks, counts, screens = [], [], []
    deadline = time.time() + 420
    last = SKIP + MINUTE
    while p.poll() is None and time.time() < deadline:
        time.sleep(0.005)
        if h is None:
            pid = find_process('winss.exe', '\\origrun\\' + RUN + '\\')
            if pid:
                h = k32.OpenProcess(QUERY_LIMITED | VM_READ | SYNCHRONIZE | TERMINATE, False, pid)
                threads = Threads(pid)
            continue
        if not app:
            app = read32(h, APP_PTR)
            continue
        u = read32(h, app + UPDATES)
        if u is None:
            continue
        if base is None:
            if read32(h, app + DEMO):
                seen_demo = True
            if seen_demo and not read32(h, app + DEMO):
                base = u   # the update the board was seen at (the last mark's, within a few updates)
            else:
                continue
        if len(marks) < 2 and u >= base + SKIP + MINUTE * len(marks):
            marks.append(reading(h, threads))
            counts.append((read32(h, app + SLEEPS), read32(h, app + DRAWS), u))
            # which screens exist: title, menu, scores, demo, board, map (the app's +0x424 .. +0x438)
            screens.append([int(bool(read32(h, app + o))) for o in (0x424, 0x428, 0x42c, 0x430, 0x434, 0x438)])
        if u >= base + last + 30:
            break
    if h:
        k32.TerminateProcess(h, 0)
    try:
        p.wait(30)
    except subprocess.TimeoutExpired:
        p.terminate()
    log.close()
    res = {'side': 'orig', 'exit': p.returncode, 'others_start': n0, 'others_end': others(), 'screens': screens}
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
