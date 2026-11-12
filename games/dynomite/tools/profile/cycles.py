"""Processor time like for like (docs/COORDINATION.md, "The target"): the port and the original over the same stretch of
play, in real time, as cycle counts. pairs.sh runs them in turn; from the worktree root:

    build/native/rundesk.exe <python.exe> -I games/dynomite/tools/profile/cycles.py port <out dir> <script> <seed> <from> <updates>
    <python.exe> -I games/dynomite/tools/profile/cycles.py orig <out dir> <origrun script> <seed> <from> <updates>
                                                    (not under rundesk: origrun puts the game there itself)

The stretch is the updates <from> to <from>+<updates> of the same input (a port -script, and the same turned into
origrun's by identical/toorig.py). Neither side runs test machinery in it: the port reads its input from the -script at
start and gets no -commands; it is marked by two `state` lines of that script, at <from> and at <from>+<updates>, whose
replies this script sees in the port's log (polled every 10 ms). The original's update counter (the app's +0x468,
origrun's --counter) is read out of its memory every 10 ms. At each end the process's cycles are read
(QueryProcessCycleTime: every thread's, those that ended included) and each thread's own (QueryThreadCycleTime, with the
name a thread was given), so the port's sound thread can be counted apart: origrun's stand-in takes the original's
DirectSound, so the original's figure holds no DirectSound mixing. Cycle counts, never GetProcessTimes or
GetThreadTimes, which Windows charges by sampling at the 15.6 ms clock tick.

Each run prints one JSON line: the stretch's cycles (G), its length in seconds and updates a second, the threads'
cycles in it, and for the original its draws in it, its windowed and vsync flags (the app's +0x2e2 and +0x559) and its
refresh period (its DDInterface's +0x4c), which show how it paces itself. Its main loop (SexyApp vf04, 0x4834b0) times
its 100 updates a second with timeGetTime and, when no update is due, draws and sleeps out the rest of the 10 ms.
Windowed, its redraw (0x469f50) blits with DDBLT_WAIT and never waits for a vertical blank, so it draws after every
update, as on a player's windowed screen; full screen, its Flip waits for the vertical blank. With CYC_VSYNC=1 the copy
runs windowed with WaitForVSync on (through origrun's --reg-map popcap), so its draw step (SexyApp vf06, 0x488920)
sleeps out the refresh period: at most one draw a refresh, as in full screen on a player's 60 Hz screen (a player's copy
starts in full screen: partner.xml's DefaultWindowed is False). The port's draws are counted too (its app's mDrawCount
and mUpdateCount, read from outside at each end); its present waits for the vertical blank outside -fast. The port's
window is 2x the game's size where rundesk's desktop has room (a player's default window); with CYC_WINDOWSCALE=1 it is
the game's own 640x480, as the original's window (-windowscale=1). Its result names the window and renderer it got, from
its log's display lines (docs/TEST_COMMANDS.md, "The window line").

With CYC_SPLIT=1 a run also splits its cycles into update, draw and present, to find where they go (docs/TEST_COMMANDS.md,
"The cycle split"): the port with -cyclesplit (split.jsonl, snapshots `a` and `b` at the stretch's ends), the original
with origrun's --count over count.txt's functions (counts.jsonl, the same labels). tools/cyclesplit.py reads the two
files. The split's readings cost cycles of their own, so a split run is never one of the measured ones.
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
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.argtypes = [W.DWORD, W.DWORD]
k32.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, ctypes.POINTER(W.DWORD)]
k32.QueryProcessCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_ulonglong)]
k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_ulonglong)]
k32.GetThreadDescription.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_wchar_p)]
k32.LocalFree.argtypes = [ctypes.c_void_p]
k32.TerminateProcess.argtypes = [W.HANDLE, W.UINT]
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]

QUERY_LIMITED, VM_READ, TERMINATE, SYNCHRONIZE = 0x1000, 0x10, 0x1, 0x100000
THREAD_QUERY_LIMITED = 0x800


class PE32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD), ('th32ParentProcessID', W.DWORD),
                ('pcPriClassBase', ctypes.c_long), ('dwFlags', W.DWORD), ('szExeFile', ctypes.c_char * 260)]


class TE32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', ctypes.c_long), ('tpDeltaPri', ctypes.c_long), ('dwFlags', W.DWORD)]


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
    for pid, name in processes():
        if name.lower() in ('rundesk.exe', 'origrun.exe', 'python.exe', 'bash.exe'):
            continue
        p = image_path(pid).lower()
        if ('\\byo-pop\\' in p or '\\byo-og\\' in p) and '\\byo-og\\dyncyc\\' not in p and os.getcwd().lower() not in p:
            n += 1
    return n


def with_marks(src, dst, first, last, drop, more=()):
    """The script without `drop` lines, with a `state` line at `first` and at `last` and the (update, line)s of `more`
    (in update order)."""
    lines = []
    for line in open(src):
        w = line.split()
        if not w or any(d in line for d in drop):
            continue
        lines.append((int(w[0]), line.rstrip('\n')))
    lines += [(first, '%d state' % first), (last, '%d state' % last)] + list(more)
    lines.sort(key=lambda kv: kv[0])
    with open(dst, 'w', newline='\n') as f:
        f.write(''.join(l + '\n' for _, l in lines))
    return dst


def new_user(d):
    """A fresh user folder: the tests' settings (windowed, no vsync) and mkhs.py's low tables, as perf.py's."""
    u = os.path.join(d, 'user')
    os.makedirs(u)
    with open(os.path.join(u, 'registry.ini'), 'w', newline='\n') as f:
        f.write('[SOFTWARE\\PopCap\\Dynomite]\n' + ''.join('%s=dword:%s\n' % kv for kv in (
            ('CustomCursors', '0'), ('InProgress', '0'), ('MusicVolume', '50'), ('Muted', '0'), ('PreferredX', '-1'),
            ('PreferredY', '-1'), ('ScreenMode', '0'), ('SfxVolume', '85'), ('TimesExecuted', '5'), ('TimesPlayed', '0'),
            ('WaitForVSync', '0'))))
    subprocess.run([sys.executable, '-I', 'games/dynomite/tools/identical/mkhs.py', u], check=True)
    return u


class ME32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('th32ModuleID', W.DWORD), ('th32ProcessID', W.DWORD), ('GlblcntUsage', W.DWORD),
                ('ProccntUsage', W.DWORD), ('modBaseAddr', ctypes.c_void_p), ('modBaseSize', W.DWORD),
                ('hModule', ctypes.c_void_p), ('szModule', ctypes.c_char * 256), ('szExePath', ctypes.c_char * 260)]


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


def module_base(pid, name):
    snap = k32.CreateToolhelp32Snapshot(0x18, pid)   # the process's modules, the 32-bit ones too
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


# the framework's SexyAppBase::mDrawCount and mUpdateCount (clang's offsetof, 2026-11-11); a stretch whose update counter
# does not advance by about its length reports no draws, so a moved field shows
DRAW_COUNT, UPDATE_COUNT = 944, 948


def port_counts(h, pid, app_rva):
    """The port's (draws, updates) so far, read from its app object (gSexyAppBase), or None."""
    base = module_base(pid, 'dynomite.exe') if app_rva else None
    app = read32(h, base + app_rva) if base else None
    if not app:
        return None
    d, u = read32(h, app + DRAW_COUNT), read32(h, app + UPDATE_COUNT)
    return None if d is None or u is None else (d, u)


def port(out, script, seed, first, count):
    root = os.getcwd()
    exe, gamedir = os.path.join(root, 'build', 'native', 'dynomite.exe'), os.path.join(root, 'assets', 'dynomite')
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    user = new_user(out)
    split = os.environ.get('CYC_SPLIT') == '1'
    sc = with_marks(script, os.path.join(out, 'script.txt'), first, first + count, ['screenshot', ' state'],
                    [(first, '%d cyclesplit a' % first), (first + count, '%d cyclesplit b' % (first + count))] if split else [])
    n0 = others()
    logpath = os.path.join(out, 'game.log')
    log = open(logpath, 'wb')
    args = [exe, '-gamedir=' + gamedir, '-userdir=' + user, '-seed=' + seed, '-script=' + sc]
    if os.environ.get('CYC_WINDOWSCALE'):
        args.append('-windowscale=' + os.environ['CYC_WINDOWSCALE'])
    if split:
        args.append('-cyclesplit=' + os.path.join(out, 'split.jsonl'))
    p = subprocess.Popen(args, cwd=out, stdout=log, stderr=subprocess.STDOUT)
    h = k32.OpenProcess(QUERY_LIMITED | VM_READ | SYNCHRONIZE | TERMINATE, False, p.pid)
    threads = Threads(p.pid)
    marks, pos, seen = [], 0, 0
    app_rva, counts = symbol_rva(exe, '__ZN4Sexy12gSexyAppBaseE'), []
    while p.poll() is None and len(marks) < 2:
        time.sleep(0.01)
        with open(logpath, 'rb') as f:
            f.seek(pos)
            data = f.read()
        end = data.rfind(b'\n') + 1
        seen += data[:end].count(b'STATE ')
        pos += end
        while len(marks) < seen and len(marks) < 2:
            marks.append(reading(h, threads))
            counts.append(port_counts(h, p.pid, app_rva))
    p.wait(120)
    whole = process_cycles(h)
    log.close()
    res = {'side': 'port', 'exit': p.returncode, 'others_start': n0, 'others_end': others(),
           'whole_run_gcycles': round(whole / 1e9, 3) if whole else None, 'log_bytes': os.path.getsize(logpath)}
    with open(logpath, 'rb') as f:
        res['display'] = [l[l.find('display: ') + 9:].strip() for l in f.read().decode('latin-1').splitlines()
                          if 'display: ' in l]
    if len(marks) == 2:
        res.update(stretch(marks[0], marks[1], threads.name))
        res['updates'] = count
        res['updates_per_s'] = round(count / res['seconds'], 1)
        # the counters are read a few ms after each mark's reply, so the update counter's span may be off by an update
        # or two; one far from the stretch's length means the offsets moved
        if None not in counts and abs(counts[1][1] - counts[0][1] - count) <= 10:
            res['draws'] = counts[1][0] - counts[0][0]
            res['updates_counted'] = counts[1][1] - counts[0][1]
    threads.close()
    k32.CloseHandle(h)
    print(json.dumps(res), flush=True)


# The original's app is the static object at 0x5304f8: its update counter is +0x468 (0x530960, origrun's --counter),
# its draw counter +0x464, its windowed flag +0x2e2 (ScreenMode 0) and its vsync flag +0x559 (WaitForVSync)
APP = 0x5304F8
COUNTER, DRAWS = APP + 0x468, APP + 0x464
STAGE = 'C:/byo-og/dyn/game'          # identical/setup.sh's staged original
COPY = 'C:/byo-og/dyncyc/game'        # a fresh copy of it each run


def find_process(name, folder):
    for pid, n in processes():
        if n.lower() == name.lower() and folder.lower() in image_path(pid).lower():
            return pid
    return None


def orig(out, script, seed, first, count):
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    sc = os.path.join(out, 'script.txt')
    with open(script) as f, open(sc, 'w', newline='\n') as g:
        g.write(''.join(l for l in f if ' shot ' not in l and ' screenshot ' not in l))
    last_input = max(int(l.split()[1]) for l in open(sc) if l.startswith('at '))
    split = os.environ.get('CYC_SPLIT') == '1'
    if split:   # origrun's --count snapshots at the stretch's ends, labelled as the port's
        lines = open(sc).read().splitlines()
        at = [l for l in lines if l.startswith('at ')] + ['at %d count a' % first, 'at %d count b' % (first + count)]
        at.sort(key=lambda l: int(l.split()[1]))
        with open(sc, 'w', newline='\n') as g:
            g.write(''.join(l + '\n' for l in at + [l for l in lines if not l.startswith('at ')]))
    shutil.rmtree(os.path.dirname(COPY), ignore_errors=True)
    shutil.copytree(STAGE, COPY, ignore=shutil.ignore_patterns('*.HS', '*.hs', 'SLOT*.SAV', 'STOMPED.TMP'))
    subprocess.run([sys.executable, '-I', 'games/dynomite/tools/identical/mkhs.py', COPY], check=True)
    # The original reads its settings from HKLM\SOFTWARE\PopCap\Dynomite (with no --reg-map, as every Dynomite run of
    # origrun): the user's own values there, windowed (ScreenMode 0) and WaitForVSync off. A run on the host cannot write
    # HKLM, so CYC_VSYNC=1 runs the copy with origrun's --reg-map popcap, which opens those keys under HKCU instead, and
    # sets the run's values there (the identical-play runs' settings.reg, windowed, WaitForVSync on), putting
    # settings.reg's back after. With WaitForVSync on, its draw step sleeps out the screen's refresh period: at most one
    # draw a refresh, as its full screen's Flip, which waits for the vertical blank, draws on a player's 60 Hz screen.
    vsync = os.environ.get('CYC_VSYNC') == '1'
    settings = os.path.join(root, 'games', 'dynomite', 'tools', 'identical', 'settings.reg')
    if vsync:
        subprocess.run(['reg', 'import', settings], capture_output=True)
        for name in ('ScreenMode', 'WaitForVSync'):
            subprocess.run(['reg', 'add', HKCU_KEY, '/v', name, '/t', 'REG_DWORD', '/d', '1' if name == 'WaitForVSync' else '0',
                            '/f'], capture_output=True)
    try:
        res = orig_play(out, sc, seed, first, count, last_input, origrun, (['--reg-map', 'popcap'] if vsync else []) +
                        (['--count', os.path.join(root, 'games', 'dynomite', 'tools', 'profile', 'count.txt')] if split else []))
        if split and os.path.exists(os.path.join(root, 'build', 'origrun', 'dyncyc', 'counts.jsonl')):
            shutil.copy(os.path.join(root, 'build', 'origrun', 'dyncyc', 'counts.jsonl'), os.path.join(out, 'counts.jsonl'))
    finally:
        if vsync:
            subprocess.run(['reg', 'import', settings], capture_output=True)
    print(json.dumps(res), flush=True)


HKCU_KEY = r'HKCU\Software\PopCap\Dynomite'


def orig_play(out, sc, seed, first, count, last_input, origrun, extra):
    n0 = others()
    log = open(os.path.join(out, 'origrun.log'), 'wb')
    p = subprocess.Popen([origrun, '--exe', COPY + '/DynomiteGame.exe', '--run', 'dyncyc', '--script', sc,
                          '--clock', 'real', '--cursor', 'scripted', '--counter', '0x530960', '--seed', seed,
                          '--patches', 'tools/origrun/patches/dynomite.txt', '--timeout', '400'] + extra,
                         stdout=log, stderr=subprocess.STDOUT)
    h = threads = None
    marks, flags, draws = [], None, []
    deadline = time.time() + 420
    while p.poll() is None and time.time() < deadline:
        time.sleep(0.01)
        if h is None:
            pid = find_process('DynomiteGame.exe', '\\byo-og\\dyncyc\\')
            if pid:
                h = k32.OpenProcess(QUERY_LIMITED | VM_READ | SYNCHRONIZE | TERMINATE, False, pid)
                threads = Threads(pid)
            continue
        c = read32(h, COUNTER)
        if c is None:
            continue
        if len(marks) < 2 and c >= first + count * len(marks):
            marks.append(reading(h, threads))
            marks[-1]['counter'] = c
            draws.append(read32(h, DRAWS))
            if flags is None:
                ddi = read32(h, APP + 0x30c)
                flags = {'windowed': (read32(h, APP + 0x2e0) >> 16) & 0xff, 'vsync': (read32(h, APP + 0x558) >> 8) & 0xff,
                         'refresh_ms': read32(h, ddi + 0x4c) if ddi else None}
        if c >= last_input + 30:   # past the script's last input: stop it here
            break
    whole = process_cycles(h) if h else None
    if h:
        k32.TerminateProcess(h, 0)
    try:
        p.wait(30)
    except subprocess.TimeoutExpired:
        p.terminate()
    log.close()
    res = {'side': 'orig', 'exit': p.returncode, 'others_start': n0, 'others_end': others(),
           'whole_run_gcycles': round(whole / 1e9, 3) if whole else None, 'flags': flags}
    if len(marks) == 2:
        res.update(stretch(marks[0], marks[1], threads.name))
        res['updates'] = marks[1]['counter'] - marks[0]['counter']
        res['updates_per_s'] = round(res['updates'] / res['seconds'], 1)
        if None not in draws:
            res['draws'] = draws[1] - draws[0]
    if threads:
        threads.close()
    if h:
        k32.CloseHandle(h)
    return res


if __name__ == '__main__':
    if len(sys.argv) != 7 or sys.argv[1] not in ('port', 'orig'):
        sys.exit(__doc__)
    (port if sys.argv[1] == 'port' else orig)(sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]), int(sys.argv[6]))
