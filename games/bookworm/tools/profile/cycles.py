"""BookWorm Deluxe's processor time like for like, the port against the original, counted as cycles (docs/COORDINATION.md,
"The target"; GAME_NOTES.md, "Processor time like for like").

Run from the worktree root, each under pincore (a performance core of its own) and, for the port, rundesk:

    python -I tools/pincore.py -- build/native/rundesk.exe <python.exe> games/bookworm/tools/profile/cycles.py port <out dir> <scenario>
    python -I tools/pincore.py -- <python.exe> games/bookworm/tools/profile/cycles.py orig <out dir> <scenario>
    <python.exe> games/bookworm/tools/profile/cycles.py summary <out dir>        (the medians of pairs2.sh's runs)

What it counts. QueryProcessCycleTime (every thread of the process, the ones that ended too) and QueryThreadCycleTime for each
thread, read at the scenario's marker updates and never in between: nothing samples, suspends or polls the game, and Windows'
15.6 ms tick (GetProcessTimes) charges nothing. The scenario is a port script (a bot game's input, scenario.txt). The port plays it
in real time with only the script's own lines: `pointer` lines at the marker updates (a log line each, which this script receives as
OutputDebugString output the moment it is logged: the game's stdout is a file its C runtime keeps until exit; no state log, no
-commands channel, no -fast), the original through origrun on its real clock (tools/identical/toorig.py), its markers the update counter read
out of its memory. The original paces itself with Sleep (SexyApp::Process, 0x480700: draws when it is ahead and sleeps the rest of the
10 ms), so it is not patched; the number of updates in each segment is reported for both. A segment's figures: wall time, updates,
cycles, cycles an update, and a share of one core = cycles / (the cycle counter's rate x wall time); the rate is measured here, by a
thread that spins for a quarter second at the start of the run.

Sound. origrun's stand-in takes the original's DirectSound and mixes nothing (BASS decodes its music in a thread of the game's own,
which is counted), so the port's audio thread (SDL's "SDLAudio..." thread, under rundesk SDL's silent driver, which still calls the
mixer) is listed apart and the port's total is given with and without it.
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

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import perf  # noqa: E402  (the existing tools' helpers: scenario files, the original's staging and memory reads)

k32 = perf.k32
psapi = perf.psapi
ntdll = ctypes.WinDLL('ntdll')
k32.QueryProcessCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.GetThreadDescription.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_wchar_p)]
k32.LocalFree.argtypes = [ctypes.c_void_p]
ntdll.NtQueryInformationThread.argtypes = [W.HANDLE, ctypes.c_int, ctypes.c_void_p, W.ULONG, ctypes.c_void_p]
THREAD_QUERY = 0x40 | 0x800   # THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


class DebugOutput:
    """OutputDebugString's listener (what DebugView does): SDL_Log reaches it the moment a line is logged, while the game's stdout is
    a file its C runtime keeps until exit and `-commands`, which flushes it, costs 340 k cycles an update. The game's own cost is a call
    it makes anyway for each log line."""

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


class MODINFO(ctypes.Structure):
    _fields_ = [('base', ctypes.c_void_p), ('size', W.DWORD), ('entry', ctypes.c_void_p)]


# the marker updates of the port's script: the first segment is play across three levels (words, level-ups), the second is the 2,000
# updates before the scenario's last, the stretch of GAME_NOTES.md's first performance pass (the end of a game, the Hall of Fame, the selector)
def marks(end):
    """The port's update numbers of the markers; `end` is the last update a marker can run at (the script's own quit comes at the next)."""
    return [int(x) for x in os.environ.get('PERF_MARKS', '1500,3500,%d,%d' % (end - 1999, end)).split(',')]


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
        # the game loop's thread, where counting adds its readings: the busiest thread that is not the mixer's or a graphics driver's
        main = next((d for d, _, nm in threads if not re.search(r'nvwgf2um|nvldumd|d3d|dxgi|sdlaudio|bass|dsound|audio', nm, re.I)), 0)
        seg = {'from_update': ua, 'to_update': ub, 'updates': n, 'wall_s': round(wall, 3),
               'updates_per_s': round(n / wall, 1), 'cycles_g': round(cyc / 1e9, 4),
               'cycles_per_update_m': round(cyc / n / 1e6, 4), 'main_cycles_per_update_m': round(main / n / 1e6, 5), 'share_pct': round(100 * cyc / (hz * wall), 2),
               'audio_thread_share_pct': round(100 * audio / (hz * wall), 2),
               'share_without_audio_pct': round(100 * (cyc - audio) / (hz * wall), 2),
               'threads': [{'share_pct': round(100 * d / (hz * wall), 2), 'tid': tid, 'thread': nm} for d, tid, nm in threads[:6]]}
        out.append(seg)
    return out


def port_run(out, scenario_file):
    exe, gamedir = perf.port_exe()
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    os.makedirs(os.path.join(out, 'mods'))
    script = perf.port_script(scenario_file, os.path.join(out, 'script.txt'))
    first, last = perf.script_span(script)
    ms = marks(last - 2)      # the scenario's own quit is at `last` - 1 and runs before a marker of the same update
    states = toggle_states(len(ms) - 1)             # PERF_TOGGLE: block i, from marker i to marker i + 1, counts or does not
    split = os.environ.get('PERF_SPLIT') == '1' or states is not None   # the run with the framework's -cyclesplit counting (tools/cyclesplit.py reads its file)
    with open(script, 'a', newline='\n') as f:      # a log line at each marker update: the only test line the player's run lacks
        for i, m in enumerate(ms):
            f.write('%d pointer\n' % m)
            if states is not None and i < len(states):
                f.write('%d cyclesplit %s\n' % (m, 'resume' if states[i] else 'pause'))
            elif split:
                f.write('%d cyclesplit m%d\n' % (m, m))
    hz = tsc_hz()
    dbg = DebugOutput()
    others0 = perf.game_processes(perf.mine_prefixes())
    s0 = perf.system_times()
    logpath = os.path.join(out, 'game.log')
    log = open(logpath, 'wb')
    p = subprocess.Popen([exe, '-gamedir=' + gamedir, '-userdir=' + os.path.join(out, 'user'), '-modsdir=' + os.path.join(out, 'mods'),
                          '-seed=' + os.environ.get('PERF_SEED', '1'), '-script=' + script] +
                         (['-cyclesplit=' + os.path.join(out, 'split.jsonl')] if split else []) +
                         os.environ.get('PERF_PORT_ARGS', '').split(), cwd=out, stdout=log,   # PERF_PORT_ARGS='-windowscale=1': the game's own window size
                         stderr=subprocess.STDOUT)
    hp = k32.OpenProcess(perf.QUERY | perf.VM_READ | perf.QUERY_LIMITED, False, p.pid)
    created = perf.creation_time(hp)
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
    s1 = perf.system_times()
    if p.poll() is None:
        p.wait(60)
    log.close()
    res = {'side': 'port', 'exit': p.returncode, 'markers_seen': len(snaps), 'tsc_hz_m': round(hz / 1e6, 1),
           'machine_busy': round(perf.busy_fraction(s0, s1), 2), 'others_start': len(others0),
           'others_end': len(perf.game_processes(perf.mine_prefixes()))}
    if len(snaps) == len(ms):
        res['segments'] = segments(snaps, ms, hz, watch)
    watch.close()
    k32.CloseHandle(hp)
    json.dump(res, open(os.path.join(out, 'cycles.json'), 'w'))
    print(json.dumps(res), flush=True)


def toggle_states(n, mode=None):
    """PERF_TOGGLE=1 or 2: whether counting is on in each of n blocks of updates, in the order on, paused, paused, on (and the reverse
    for 2), so that a slow drift of the machine and a periodic animation fall alike on both. Unset: None."""
    mode = mode or os.environ.get('PERF_TOGGLE')
    if not mode:
        return None
    pattern = [True, False, False, True]
    return [pattern[i % 4] == (mode == '1') for i in range(n)]


def add_count_lines(script, markers):
    """Put `at <counter> count <label>` lines into an origrun script, in order among its `at` lines and before its quit."""
    lines = open(script).read().splitlines()
    for counter, label in markers:
        at = len(lines)
        for i, line in enumerate(lines):
            w = line.split()
            if w[:1] == ['quit'] or (w[:1] == ['at'] and len(w) > 1 and w[1].isdigit() and int(w[1]) > counter):
                at = i
                break
        lines.insert(at, 'at %d count %s' % (counter, label))
    open(script, 'w', newline='\n').write('\n'.join(lines) + '\n')


def orig_run(out, scenario_file):
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    run = os.path.basename(perf.STAGE)
    master = perf.STAGE + '/master'
    game = perf.STAGE + '/game'
    d = os.path.abspath(out)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    cut = perf.port_script(scenario_file, os.path.join(d, 'port-script.txt'))
    sc = perf.orig_script(cut, os.path.join(d, 'script.txt'))
    first, last = perf.script_span(sc)         # the original's own update numbers (a port line at K is `at K-1`)
    ms = [m - 1 for m in marks(last - 1)]   # a port update K is the original's counter K-1; the last port marker is the update before its quit
    states = toggle_states(len(ms) - 1)
    split = os.environ.get('PERF_SPLIT') == '1' or states is not None
    if states is not None:
        add_count_lines(sc, [(m, 'resume' if on else 'pause') for m, on in zip(ms, states)])
    elif split:
        # origrun's --count: a snapshot at each marker, named as the port's cyclesplit lines name them (m<port update>)
        add_count_lines(sc, list(zip(ms, ['m%d' % x for x in marks(last)])))
    shutil.rmtree(game, ignore_errors=True)
    shutil.copytree(master, game)
    shutil.rmtree(os.path.join(root, 'build', 'origrun', run), ignore_errors=True)
    subprocess.run(['reg', 'delete', r'HKCU\Software\PopCap\BookWorm', '/f'], capture_output=True)
    hz = tsc_hz()
    others0 = perf.game_processes(perf.mine_prefixes())
    s0 = perf.system_times()
    log = open(os.path.join(d, 'origrun.log'), 'wb')
    t_start = time.time()
    p = subprocess.Popen([origrun, '--exe', os.path.join(game, 'BookwormGame.exe').replace('/', '\\'), '--run', run, '--script', sc,
                          '--clock', 'real', '--cursor', 'scripted', '--counter', '[0x%X]+0x%X' % (perf.APP_PTR, perf.COUNTER),
                          '--update-ms', '10', '--seed', os.environ.get('PERF_SEED', '1'), '--patches',
                          os.path.join(root, 'tools', 'origrun', 'patches', 'bookworm.txt'), '--timeout', '400']
                         + (['--count', os.path.join(root, 'games', 'bookworm', 'tools', 'profile', 'count.txt')] if split else []),
                         stdout=log, stderr=subprocess.STDOUT)
    h = None
    watch = None
    pid = None
    snaps, counts = [], []
    next_refresh = 0
    deadline = time.time() + 420
    while p.poll() is None and len(snaps) < len(ms) and time.time() < deadline:
        if h is None:
            pid = perf.find_process('BookwormGame.exe', '\\byo-og\\' + run + '\\')
            if pid:
                h = k32.OpenProcess(perf.QUERY | perf.VM_READ | perf.QUERY_LIMITED | perf.TERMINATE, False, pid)
                watch = Watch(h)
        else:
            app = perf.read32(h, perf.APP_PTR)
            c = perf.read32(h, app + perf.COUNTER) if app else None
            if c is not None and c >= ms[len(snaps)]:
                snaps.append(watch.snapshot())
                c2 = perf.read32(h, app + perf.COUNTER)
                counts.append((c + (c2 if c2 is not None else c)) / 2.0)
                continue
            if time.time() >= next_refresh:
                watch.refresh(pid)
                next_refresh = time.time() + 1
        time.sleep(0.002)
    s1 = perf.system_times()
    if h and not split:
        k32.TerminateProcess(h, 0)         # a counting run goes on to the script's own quit, so that its last snapshot (the exit one) is taken
    try:
        p.wait(60)
    except subprocess.TimeoutExpired:
        p.terminate()
    counts_file = os.path.join(root, 'build', 'origrun', run, 'counts.jsonl')
    if split and os.path.exists(counts_file):
        shutil.copy(counts_file, os.path.join(d, 'split.jsonl'))
    res = {'side': 'orig', 'exit': p.returncode, 'markers_seen': len(snaps), 'tsc_hz_m': round(hz / 1e6, 1),
           'machine_busy': round(perf.busy_fraction(s0, s1), 2), 'others_start': len(others0),
           'others_end': len(perf.game_processes(perf.mine_prefixes()))}
    if len(snaps) == len(ms):
        res['segments'] = segments(snaps, counts, hz, watch, audio_words=())
        for seg, m in zip(res['segments'], zip(ms, ms[1:])):
            seg['marker_updates'] = list(m)
    if watch:
        watch.close()
    if h:
        k32.CloseHandle(h)
    json.dump(res, open(os.path.join(d, 'cycles.json'), 'w'))
    print(json.dumps(res), flush=True)


def summary(out):
    sides = {}
    for side in ('port', 'orig'):
        rows = []
        for name in sorted(glob.glob(os.path.join(out, 'p*-%s.json' % side))):
            for line in open(name):
                if line.startswith('{'):
                    r = json.loads(line)
                    if 'segments' in r:
                        rows.append(r)
        sides[side] = rows
    print('pairs: port %d, orig %d' % (len(sides['port']), len(sides['orig'])))
    nseg = min((len(r['segments']) for rows in sides.values() for r in rows), default=0)
    for i in range(nseg):
        a = sides['port'][0]['segments'][i]
        print('== updates %d to %d' % (a['from_update'], a['to_update']))
        for side in ('port', 'orig'):
            rows = sides[side]
            if not rows:
                continue

            def stat(key):
                v = [r['segments'][i][key] for r in rows]
                return '%.2f (%.2f to %.2f)' % (statistics.median(v), min(v), max(v))
            print('  %-5s share of one core %%: %s; cycles an update M: %s; updates a second: %s' % (
                side, stat('share_pct'), stat('cycles_per_update_m'), stat('updates_per_s')))
            if side == 'port':
                print('        without the audio thread %%: %s; the audio thread %%: %s' % (stat('share_without_audio_pct'), stat('audio_thread_share_pct')))
        if sides['port'] and sides['orig']:
            pm = statistics.median(r['segments'][i]['cycles_per_update_m'] for r in sides['port'])
            om = statistics.median(r['segments'][i]['cycles_per_update_m'] for r in sides['orig'])
            print('  port / original, cycles an update: %.2f' % (pm / om))


def compare(out):
    """splitpairs.sh's runs: the port with -cyclesplit counting off and on, by segment: medians and the pairs' ratios."""
    sides = {}
    for k in ('off', 'on'):
        rows = []
        for name in sorted(glob.glob(os.path.join(out, 'p*-%s.json' % k))):
            for line in open(name):
                if line.startswith('{'):
                    r = json.loads(line)
                    if 'segments' in r:
                        rows.append((os.path.basename(name).split('-')[0], r))
        sides[k] = dict(rows)
    pairs = sorted(set(sides['off']) & set(sides['on']))
    print('pairs: %d' % len(pairs))
    for i in range(3):
        off = [sides['off'][p]['segments'][i]['cycles_per_update_m'] for p in pairs]
        on = [sides['on'][p]['segments'][i]['cycles_per_update_m'] for p in pairs]
        ratio = [b / a for a, b in zip(off, on)]
        seg = sides['off'][pairs[0]]['segments'][i]
        print('updates %d to %d: cycles an update M, off %.3f (%.3f to %.3f), on %.3f (%.3f to %.3f); on / off by pair: median %.3f, %s' % (
            seg['from_update'], seg['to_update'], statistics.median(off), min(off), max(off), statistics.median(on), min(on), max(on),
            statistics.median(ratio), ' '.join('%.3f' % x for x in ratio)))


def toggle(out):
    """toggleruns.sh's runs: counting on and paused in alternating blocks of one run. Per run the mean cycles an update of the blocks that
    counted against the blocks that did not, and each counting block against its paused neighbours (the machine's drift is slower than a block)."""
    runs = []
    for name in sorted(glob.glob(os.path.join(out, 't*-m*.json'))):
        for line in open(name):
            if line.startswith('{'):
                r = json.loads(line)
                if 'segments' in r:
                    runs.append((os.path.basename(name), r))
    allratio, neighbours, added = [], [], []
    for name, r in runs:
        segs = [s.get('main_cycles_per_update_m', s['cycles_per_update_m']) for s in r['segments']]
        on_state = toggle_states(len(segs), name.split('-m')[1][0])
        on = [c for c, o in zip(segs, on_state) if o]
        off = [c for c, o in zip(segs, on_state) if not o]
        ratio = statistics.mean(on) / statistics.mean(off)
        allratio.append(ratio)
        added.append(statistics.mean(on) - statistics.mean(off))
        for i, (c, o) in enumerate(zip(segs, on_state)):
            near = [segs[j] for j in (i - 1, i + 1) if 0 <= j < len(segs) and on_state[j] != o]
            if o and near:
                neighbours.append(c / statistics.mean(near))
        print('%s: %d blocks of %d updates, counting %.3f M cycles an update (%s), paused %.3f (%s): %.4f' % (
            name, len(segs), r['segments'][0]['updates'], statistics.mean(on), ' '.join('%.2f' % x for x in on),
            statistics.mean(off), ' '.join('%.2f' % x for x in off), ratio))
    if allratio:
        print('counting / paused: mean of the runs %.4f (%s), median of the %d blocks against their neighbours %.4f' % (
            statistics.mean(allratio), ' '.join('%.3f' % x for x in allratio), len(neighbours), statistics.median(neighbours)))
        print('what counting adds a update: mean of the runs %.0f cycles (%s)' % (
            1e6 * statistics.mean(added), ' '.join('%.0f' % (1e6 * x) for x in added)))


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else ''
    if mode == 'port':
        port_run(sys.argv[2], sys.argv[3])
    elif mode == 'orig':
        orig_run(sys.argv[2], sys.argv[3])
    elif mode == 'summary':
        summary(sys.argv[2])
    elif mode == 'compare':
        compare(sys.argv[2])
    elif mode == 'toggle':
        toggle(sys.argv[2])
    else:
        sys.exit(__doc__)
