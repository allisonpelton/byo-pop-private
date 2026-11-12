"""Rocket Mania Deluxe's processor time like for like, the port against the original, counted as cycles (docs/COORDINATION.md,
"The target"; GAME_NOTES.md, "Performance"). From BookWorm's games/bookworm/tools/profile/cycles.py.

Run from the worktree root, each under pincore (a performance core of its own) and, for the port, rundesk (pairs.sh does it):

    python -I tools/pincore.py -- build/native/rundesk.exe <python.exe> games/rocketmania/tools/profile/cycles.py port <out dir> <scenario>
    python -I tools/pincore.py -- <python.exe> games/rocketmania/tools/profile/cycles.py orig <out dir> <scenario>
    <python.exe> games/rocketmania/tools/profile/cycles.py summary <out dir>        (the medians of pairs.sh's runs)

What it counts. QueryProcessCycleTime (every thread of the process, the ones that ended too) and QueryThreadCycleTime for each
thread, read at the scenario's marker updates and never in between: nothing samples, suspends or polls the game, and Windows' 15.6 ms
tick (GetProcessTimes) charges nothing. The scenario is a port script (scenario.txt: the port's bot's input for three games, one of
each mode). The port plays it in real time with only the script's own lines: a `pointer` line at each marker update (one log line,
which this script receives as OutputDebugString output the moment it is logged: the game's stdout is a file its C runtime keeps until
exit; no state log, no -commands channel, no -fast), the original through origrun on its real clock (tools/identical/toorig.py), its
markers the update counter (0x4f6850) read out of its memory. Both are windowed at the game's own size (the port with
-windowscale=1, or the default window's size with PERF_PORT_SCALE=default; its log's display line is kept in the result). The original keeps to its own pace (a Sleep to the next update); the
number of updates in each segment is reported for both.

The settings. Both sides start from a registry of this run's own: the port's emulated one (user dir's registry.ini), the original's
HKCU\\Software\\PopCap\\RocketMania (origrun --reg-map popcap; deleted and written again before the run, restored by nothing: the
user has allowed changes to PopCap games' registry values). PERF_PORT_REG (default "ScreenMode=0,Is3D=1") and PERF_ORIG_REG (default
"ScreenMode=0,Is3D=0": on rundesk's private desktop the original has only its software screen, docs/ORIGRUN.md, "Hardware
acceleration") name the values; PERF_PORT_ARGS adds port arguments, PERF_ORIG_ARGS origrun arguments (a check run's --watch values), PERF_EXE names another exe in build/native (a frozen build).

Sound. origrun's stand-in takes the original's DirectSound and mixes nothing, so the port's audio thread (SDL's "SDLAudio..." thread,
under rundesk SDL's silent driver, which still calls the mixer) is listed apart and the port's total is given with and without it.
"""
import ctypes
import ctypes.wintypes as W
import glob
import json
import os
import re
import shutil
import statistics
import subprocess
import sys
import time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
ntdll = ctypes.WinDLL('ntdll')
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CloseHandle.argtypes = [W.HANDLE]
k32.TerminateProcess.argtypes = [W.HANDLE, W.UINT]
k32.QueryProcessCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.GetThreadDescription.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_wchar_p)]
k32.LocalFree.argtypes = [ctypes.c_void_p]
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
k32.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, ctypes.c_wchar_p, ctypes.POINTER(W.DWORD)]
ntdll.NtQueryInformationThread.argtypes = [W.HANDLE, ctypes.c_int, ctypes.c_void_p, W.ULONG, ctypes.c_void_p]
THREAD_QUERY = 0x40 | 0x800   # THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION
QUERY, VM_READ, QUERY_LIMITED, TERMINATE = 0x400, 0x10, 0x1000, 0x1

ROOT = os.getcwd()
STAGE = os.environ.get('OG_STAGE', 'C:/byo-og/rm')
COUNTER = 0x4f6850                      # the original's update counter (the app's static object at 0x4f6460, +0x3f0)
MODE = 0x4f6990                         # the game's mode (0 arcade, 1 classic, 2 strategy) and the board's pointer: what the original is playing
BOARD = 0x4f69f8
DEFAULT_MARKS = '900,3200,3500,5900,6200,8600'   # arcade play, the dialogs between games, classic play, the dialogs, strategy play


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


class PE32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD), ('th32ParentProcessID', W.DWORD),
                ('pcPriClassBase', ctypes.c_long), ('dwFlags', W.DWORD), ('szExeFile', ctypes.c_char * 260)]


class MODINFO(ctypes.Structure):
    _fields_ = [('base', ctypes.c_void_p), ('size', W.DWORD), ('entry', ctypes.c_void_p)]


class DebugOutput:
    """OutputDebugString's listener (what DebugView does): SDL_Log reaches it the moment a line is logged."""

    def __init__(self):
        k32.CreateEventW.restype = W.HANDLE
        k32.CreateFileMappingW.restype = W.HANDLE
        k32.MapViewOfFile.restype = ctypes.c_void_p
        k32.WaitForSingleObject.argtypes = [W.HANDLE, W.DWORD]
        k32.SetEvent.argtypes = [W.HANDLE]
        self.ready = k32.CreateEventW(None, False, False, 'DBWIN_BUFFER_READY')
        self.data = k32.CreateEventW(None, False, False, 'DBWIN_DATA_READY')
        self.mapping = k32.CreateFileMappingW(ctypes.c_void_p(-1), None, 0x04, 0, 4096, 'DBWIN_BUFFER')
        self.view = k32.MapViewOfFile(self.mapping, 0x04, 0, 0, 0)
        if not (self.ready and self.data and self.mapping and self.view):
            raise OSError('cannot listen for OutputDebugString: %d' % ctypes.get_last_error())
        k32.SetEvent(self.ready)

    def wait(self, ms):
        """(process id, text, time) of the next line, or None after ms."""
        if k32.WaitForSingleObject(self.data, ms) != 0:
            return None
        t = time.perf_counter()
        pid = ctypes.c_uint32.from_address(self.view).value
        text = ctypes.string_at(self.view + 4).decode('mbcs', 'replace')
        k32.SetEvent(self.ready)
        return pid, text, t


def marks():
    """The port's update numbers of the markers."""
    return [int(x) for x in os.environ.get('PERF_MARKS', DEFAULT_MARKS).split(',')]


def tsc_hz():
    """The rate the cycle counters tick at: a thread's cycles over the wall time of a quarter second's spin."""
    h = k32.OpenThread(THREAD_QUERY, False, k32.GetCurrentThreadId())
    c0, c1 = ctypes.c_uint64(), ctypes.c_uint64()
    k32.QueryThreadCycleTime(h, ctypes.byref(c0))
    t0 = time.perf_counter()
    while time.perf_counter() - t0 < 0.25:
        pass
    k32.QueryThreadCycleTime(h, ctypes.byref(c1))
    t1 = time.perf_counter()
    k32.CloseHandle(h)
    return (c1.value - c0.value) / (t1 - t0)


def system_times():
    i, k, u = W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetSystemTimes(ctypes.byref(i), ctypes.byref(k), ctypes.byref(u))
    f = lambda t: ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7
    return f(i), f(k) + f(u)      # kernel time includes the idle time


def busy_fraction(a, b):
    return 1.0 - (b[0] - a[0]) / (b[1] - a[1]) if b[1] > a[1] else 0.0


def others():
    """The number of game processes on the machine (tools/gamecount.py); a run counts it before its own game starts and after it ended."""
    out = subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'gamecount.py')], capture_output=True, text=True).stdout
    m = re.search(r'game processes: (\d+)', out)
    return int(m.group(1)) if m else -1


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


def module_list(hp):
    arr = (ctypes.c_void_p * 1024)()
    need = W.DWORD()
    psapi.EnumProcessModulesEx(hp, arr, ctypes.sizeof(arr), ctypes.byref(need), 3)
    out = []
    for i in range(need.value // ctypes.sizeof(ctypes.c_void_p)):
        name = ctypes.create_unicode_buffer(520)
        psapi.GetModuleFileNameExW(hp, ctypes.c_void_p(arr[i]), name, 520)
        mi = MODINFO()
        psapi.GetModuleInformation(hp, ctypes.c_void_p(arr[i]), ctypes.byref(mi), ctypes.sizeof(mi))
        out.append((mi.base or 0, mi.size, os.path.basename(name.value).lower()))
    return sorted(out)


class Watch:
    """Cycle counts of one process: the process's and each thread's, with the threads' names (SetThreadDescription) and the module
    their start address is in."""

    def __init__(self, hp):
        self.hp = hp
        self.handles = {}
        self.names = {}
        self.mods = []

    def thread_info(self, tid, h):
        d = ctypes.c_wchar_p()
        name = ''
        if k32.GetThreadDescription(h, ctypes.byref(d)) >= 0 and d.value:
            name = d.value
            k32.LocalFree(d)
        start = ctypes.c_size_t(0)
        where = '?'
        if ntdll.NtQueryInformationThread(h, 9, ctypes.byref(start), ctypes.sizeof(start), None) == 0:
            for base, size, mod in self.mods:
                if base <= start.value < base + size:
                    where = '%s+0x%x' % (mod, start.value - base)
                    break
        return (name + ' ' if name else '') + where

    def refresh(self, pid):
        """Open the threads that have started since the last call (named and placed now, so that a snapshot only reads counters)."""
        new = [tid for tid in thread_ids(pid) if tid not in self.handles]
        if not new:
            return
        self.mods = module_list(self.hp)
        for tid in new:
            h = k32.OpenThread(THREAD_QUERY, False, tid)
            if h:
                self.handles[tid] = h
                self.names[tid] = self.thread_info(tid, h)

    def snapshot(self):
        t = time.perf_counter()
        total = ctypes.c_uint64()
        k32.QueryProcessCycleTime(self.hp, ctypes.byref(total))
        per = {}
        for tid, h in self.handles.items():
            c = ctypes.c_uint64()
            if k32.QueryThreadCycleTime(h, ctypes.byref(c)):
                per[tid] = c.value
        return {'t': t, 'process': total.value, 'threads': per}

    def close(self):
        for h in self.handles.values():
            k32.CloseHandle(h)


def segments(snaps, updates, hz, watch, audio_words=('SDLAudio',)):
    """The figures of each pair of neighbouring snapshots (the scenario's marker updates)."""
    out = []
    for a, b, ua, ub in zip(snaps, snaps[1:], updates, updates[1:]):
        wall = b['t'] - a['t']
        n = ub - ua
        cyc = b['process'] - a['process']
        threads = []
        for tid, c in b['threads'].items():
            d = c - a['threads'].get(tid, 0)
            if d > 0:
                threads.append((d, tid, watch.names.get(tid, '?')))
        threads.sort(reverse=True)
        audio = sum(d for d, _, nm in threads if any(w in nm for w in audio_words))
        seg = {'from_update': ua, 'to_update': ub, 'updates': n, 'wall_s': round(wall, 3),
               'updates_per_s': round(n / wall, 1), 'cycles_g': round(cyc / 1e9, 4),
               'cycles_per_update_m': round(cyc / n / 1e6, 4), 'share_pct': round(100 * cyc / (hz * wall), 2),
               'audio_thread_share_pct': round(100 * audio / (hz * wall), 2),
               'share_without_audio_pct': round(100 * (cyc - audio) / (hz * wall), 2),
               'threads': [{'share_pct': round(100 * d / (hz * wall), 2), 'tid': tid, 'thread': nm} for d, tid, nm in threads[:6]]}
        out.append(seg)
    return out


def write_registry_ini(path, values):
    with open(path, 'w', newline='\n') as f:
        f.write('; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\\PopCap\\RocketMania]\n')
        for kv in values.split(','):
            if kv:
                f.write('%s=dword:%s\n' % tuple(kv.split('=')))


def result_line(side, p, snaps, ms, hz, watch, s0, s1, others0, audio_words, extra):
    res = {'side': side, 'exit': p.returncode, 'markers_seen': len(snaps), 'tsc_hz_m': round(hz / 1e6, 1),
           'machine_busy': round(busy_fraction(s0, s1), 2), 'others_start': others0, 'others_end': others()}
    res.update(extra)
    if len(snaps) == len(ms):
        res['segments'] = segments(snaps, ms if side == 'port' else extra['counts'], hz, watch, audio_words)
    return res


def port_run(out, scenario):
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    exe = os.path.join(ROOT, 'build', 'native', os.environ.get('PERF_EXE', 'rocketmania.exe'))
    ms = marks()
    split = os.environ.get('PERF_SPLIT') == '1'     # the framework's -cyclesplit: the loop thread's cycles in update, draw and present (tools/cyclesplit.py reads split.jsonl)
    script = os.path.join(out, 'script.txt')
    shutil.copy(scenario, script)
    with open(script, 'a', newline='\n') as f:      # a log line at each marker update: the only test line the player's run lacks
        f.write('\n')
        for m in ms:
            f.write('%d pointer\n' % m)
            if split:
                f.write('%d cyclesplit m%d\n' % (m, m))
    write_registry_ini(os.path.join(out, 'user', 'registry.ini'), os.environ.get('PERF_PORT_REG', 'ScreenMode=0,Is3D=1'))
    hz = tsc_hz()
    dbg = DebugOutput()
    others0 = others()
    s0 = system_times()
    log = open(os.path.join(out, 'game.log'), 'wb')
    p = subprocess.Popen([exe, '-userdir=' + os.path.join(out, 'user'), '-seed=' + os.environ.get('PERF_SEED', '1'), '-script=' + script]
                         + (['-cyclesplit=' + os.path.join(out, 'split.jsonl')] if split else [])
                         + (['-windowscale=' + os.environ.get('PERF_PORT_SCALE', '1')] if os.environ.get('PERF_PORT_SCALE', '1') != 'default' else []) +
                         os.environ.get('PERF_PORT_ARGS', '').split(), cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    hp = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED, False, p.pid)
    watch = Watch(hp)
    snaps = []
    next_refresh = 0
    deadline = time.time() + 400
    while p.poll() is None and len(snaps) < len(ms) and time.time() < deadline:
        line = dbg.wait(10)
        if line and line[0] == p.pid and 'POINTER' in line[1]:
            snaps.append(watch.snapshot())
        if time.time() >= next_refresh:
            watch.refresh(p.pid)
            next_refresh = time.time() + 1
    s1 = system_times()
    if p.poll() is None:
        try:
            p.wait(60)
        except subprocess.TimeoutExpired:
            p.kill()
    log.close()
    text = open(os.path.join(out, 'game.log'), 'rb').read().decode('latin-1')
    display = re.findall(r'display: [^\r\n]*', text)
    res = result_line('port', p, snaps, ms, hz, watch, s0, s1, others0, ('SDLAudio',),
                      {'display': display[-1] if display else None, 'port_reg': os.environ.get('PERF_PORT_REG', 'ScreenMode=0,Is3D=1')})
    watch.close()
    k32.CloseHandle(hp)
    json.dump(res, open(os.path.join(out, 'cycles.json'), 'w'))
    print(json.dumps(res), flush=True)


def orig_run(out, scenario):
    origrun = os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe')
    run = os.path.basename(STAGE)
    game = STAGE + '/game'
    d = os.path.abspath(out)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    ms = [m - 1 for m in marks()]           # a port update K is the original's counter K-1
    sc = os.path.join(d, 'script.txt')
    with open(sc, 'w', newline='\n') as f:
        f.write(subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'games', 'rocketmania', 'tools', 'identical', 'toorig.py'), scenario, '-1'],
                               capture_output=True, text=True).stdout)
    if os.environ.get('PERF_ORIG_SHOTS'):         # a check run: `at <update> shot s<update>` lines, in order among the script's (pictures in build/origrun/<run>/shots)
        lines = open(sc).read().splitlines()
        body = [l for l in lines if l.strip() and l.split()[0] == 'at']
        body += ['at %s shot s%s' % (u, u) for u in os.environ['PERF_ORIG_SHOTS'].split(',')]
        body.sort(key=lambda l: int(l.split()[1]))
        open(sc, 'w', newline='\n').write('\n'.join(body + ['quit']) + '\n')
    for f in glob.glob(os.path.join(game, 'Players', 'hiscores.dat')) + glob.glob(os.path.join(game, 'Players', 'save?.dat')):
        os.remove(f)                      # a first run's saves, as orig.sh does it (savea.dat, saveb.dat, savec.dat: one a mode; a saved game
                                          # turns the Continue dialog up and the scenario's clicks miss)
    reg = os.environ.get('PERF_ORIG_REG', 'ScreenMode=0,Is3D=0')
    subprocess.run(['reg', 'delete', r'HKCU\Software\PopCap\RocketMania', '/f'], capture_output=True)
    for kv in reg.split(','):
        n, v = kv.split('=')
        subprocess.run(['reg', 'add', r'HKCU\Software\PopCap\RocketMania', '/v', n, '/t', 'REG_DWORD', '/d', v, '/f'], capture_output=True)
    hz = tsc_hz()
    others0 = others()
    s0 = system_times()
    log = open(os.path.join(d, 'origrun.log'), 'wb')
    p = subprocess.Popen([origrun, '--exe', os.path.join(game, 'RocketManiaGame.exe').replace('/', '\\'), '--run', run, '--stage-root',
                          os.path.dirname(STAGE).replace('/', '\\'), '--script', sc, '--clock', 'real', '--cursor', 'scripted',
                          '--counter', '0x%X' % COUNTER, '--update-ms', '10', '--seed', os.environ.get('PERF_SEED', '1'), '--patches',
                          os.path.join(ROOT, 'tools', 'origrun', 'patches', 'rocketmania.txt'), '--reg-map', 'popcap', '--timeout', '400']
                         + os.environ.get('PERF_ORIG_ARGS', '').split(),
                         stdout=log, stderr=subprocess.STDOUT)
    h = None
    watch = None
    pid = None
    snaps, counts, states = [], [], []
    next_refresh = 0
    deadline = time.time() + 420
    while p.poll() is None and len(snaps) < len(ms) and time.time() < deadline:
        if h is None:
            pid = find_process('RocketManiaGame.exe', '\\byo-og\\' + run + '\\')
            if pid:
                h = k32.OpenProcess(QUERY | VM_READ | QUERY_LIMITED | TERMINATE, False, pid)
                watch = Watch(h)
        else:
            c = read32(h, COUNTER)
            if c is not None and c >= ms[len(snaps)]:
                snaps.append(watch.snapshot())
                states.append((read32(h, MODE), read32(h, BOARD)))
                c2 = read32(h, COUNTER)
                counts.append((c + (c2 if c2 is not None else c)) / 2.0)
                continue
            if time.time() >= next_refresh:
                watch.refresh(pid)
                next_refresh = time.time() + 1
        time.sleep(0.002)
    s1 = system_times()
    if h:
        k32.TerminateProcess(h, 0)
    try:
        p.wait(60)
    except subprocess.TimeoutExpired:
        p.terminate()
    res = result_line('orig', p, snaps, ms, hz, watch, s0, s1, others0, (), {'counts': counts, 'orig_reg': reg})
    res['states'] = states                  # (mode, board pointer) at each marker: the stretches' games must be mode 0, 1 and 2 with a board
    if len(snaps) == len(ms):
        for seg, m in zip(res['segments'], zip(ms, ms[1:])):
            seg['marker_updates'] = list(m)
    if watch:
        watch.close()
    if h:
        k32.CloseHandle(h)
    json.dump(res, open(os.path.join(d, 'cycles.json'), 'w'))
    print(json.dumps(res), flush=True)


def played(r):
    """Whether an original's run played the scenario's three games: a board at every marker, in the modes 0, 0, 1, 1, 2, 2."""
    return all(b for m, b in r['states']) and [m for m, b in r['states']] == [0, 0, 1, 1, 2, 2][:len(r['states'])] and len(r['states']) == 6


def summary(out):
    names = sorted({os.path.basename(f).split('-', 1)[1][:-5] for f in glob.glob(os.path.join(out, 'p*-*.json'))})
    order = [n for n in names if n != 'orig'] + (['orig'] if 'orig' in names else [])
    sides = {}
    for side in order:
        rows = []
        for name in sorted(glob.glob(os.path.join(out, 'p*-%s.json' % side))):
            for line in open(name):
                if line.startswith('{'):
                    r = json.loads(line)
                    if 'segments' in r and (side != 'orig' or played(r)):
                        rows.append(r)
        sides[side] = rows
    print('pairs: ' + ', '.join('%s %d' % (k, len(v)) for k, v in sides.items()) + ' (an original whose games did not all start is left out)')
    for side, rows in sides.items():
        if rows:
            print('%s: other game processes at the start: %s; machine busy %s; display: %s' % (
                side, sorted(r['others_start'] for r in rows), sorted(r['machine_busy'] for r in rows), rows[0].get('display')))
    nseg = min((len(r['segments']) for rows in sides.values() if rows for r in rows), default=0)
    first = next(rows for rows in sides.values() if rows)
    for i in range(nseg):
        a = first[0]['segments'][i]
        print('== updates %d to %d' % (a['from_update'], a['to_update']))
        for side, rows in sides.items():
            if not rows:
                continue

            def stat(key):
                v = [r['segments'][i][key] for r in rows]
                return '%.2f (%.2f to %.2f)' % (statistics.median(v), min(v), max(v))
            print('  %-8s share of one core %%: %s; cycles an update M: %s; updates a second: %s' % (
                side, stat('share_pct'), stat('cycles_per_update_m'), stat('updates_per_s')))
            if side != 'orig':
                print('           without the audio thread %%: %s; the audio thread %%: %s' % (stat('share_without_audio_pct'), stat('audio_thread_share_pct')))
        if sides.get('orig'):
            om = statistics.median(r['segments'][i]['cycles_per_update_m'] for r in sides['orig'])
            for side, rows in sides.items():
                if side != 'orig' and rows:
                    pm = statistics.median(r['segments'][i]['cycles_per_update_m'] for r in rows)
                    pair = []   # the same round's port run over its original, for the rounds whose original played all three games
                    for name in sorted(glob.glob(os.path.join(out, 'p*-orig.json'))):
                        other = name[:-len('orig.json')] + side + '.json'
                        if os.path.exists(other):
                            ro, rp = [json.loads(next(l for l in open(f) if l.startswith('{'))) for f in (name, other)]
                            if played(ro) and 'segments' in rp:
                                pair.append(rp['segments'][i]['cycles_per_update_m'] / ro['segments'][i]['cycles_per_update_m'])
                    print('  %s / original, cycles an update: %.2f (medians), %.2f over %d pairs of one round (%.2f to %.2f)' % (
                        side, pm / om, statistics.median(pair), len(pair), min(pair), max(pair)))


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else ''
    if mode == 'port':
        port_run(sys.argv[2], sys.argv[3])
    elif mode == 'orig':
        orig_run(sys.argv[2], sys.argv[3])
    elif mode == 'summary':
        summary(sys.argv[2])
    else:
        sys.exit(__doc__)
