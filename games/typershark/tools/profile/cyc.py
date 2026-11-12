"""The processor time of Typer Shark Deluxe, the port and the original, as cycle counts thread by thread (docs/COORDINATION.md,
"The target": like for like; GAME_NOTES.md section 11).

From the worktree root, each under pincore so that a run has a performance core of its own (pairs2.sh does it):

    python -I tools/pincore.py -- build/native/rundesk.exe <python.exe> -I games/typershark/tools/profile/cyc.py port <out dir> [scenario]
    python -I tools/pincore.py -- <python.exe> -I games/typershark/tools/profile/cyc.py orig <out dir> [scenario]   (origrun starts the game
                                                                                                           under rundesk itself)
    python -I games/typershark/tools/profile/cyc.py summary <runs.txt>

What it counts: every 0.2 s it reads QueryThreadCycleTime of each thread of the game process and QueryProcessCycleTime of the process
(which includes the threads that ended), and the game's update counter out of its memory (the original's `[0x565380]+0x4b8`; the
port's SexyAppBase::mUpdateCount, found through the exe's symbol gSexyAppBase). Nothing runs inside the game for it: no -commands
channel, no state log, no screenshot (the port's script holds only the scenario's inputs and a quit). The window is "the stretch": the
scenario's last STRETCH updates (PERF_STRETCH, 2000 unless set: 20 s at the game's 100 updates a second) and the cycles at its two
ends are interpolated between the samples that bracket the counter's value. Cycle counts, not GetProcessTimes: Windows charges that at
the 15.6 ms clock tick.

The scenario is perf.py's (build/ts-p2/rec/rt2.in.txt, cut at update PERF_END, 7000 unless set; a new player named Tester, the
Adventure button, Easy, then the port's own typist playing the first levels), played in real time by both: the port without -fast at
the game's own window size (-renderscale=1, as a player's windowed 640x480 has it; under rundesk a window of that size gets 2), the
original on origrun's real clock with the loading patch of phase 2 (build/ts-p2/patches.txt, as pairs.sh does). Each run prints one
JSON line: the stretch's cycles of the process and of each thread, with the thread's name (SDL's threads have one) and the module and
offset of its start address, the updates a second, the game processes of other sessions and the renderer named. The original's
renderer here is DirectDraw (2D): a hidden desktop gives it no Direct3D 9 (GAME_NOTES section 16).
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

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import perf  # the scenario, the original's script and the helpers of the performance pass

k32, psapi = perf.k32, perf.psapi
ntdll = ctypes.WinDLL('ntdll')
k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_ulonglong)]
k32.QueryProcessCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_ulonglong)]
k32.GetThreadDescription.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_void_p)]
k32.LocalFree.argtypes = [ctypes.c_void_p]
k32.Thread32First.argtypes = [W.HANDLE, ctypes.c_void_p]
k32.Thread32Next.argtypes = [W.HANDLE, ctypes.c_void_p]
ntdll.NtQueryInformationThread.argtypes = [W.HANDLE, ctypes.c_ulong, ctypes.c_void_p, ctypes.c_ulong, ctypes.c_void_p]
psapi.EnumProcessModulesEx.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD, ctypes.POINTER(W.DWORD), W.DWORD]
psapi.GetModuleBaseNameW.argtypes = [W.HANDLE, ctypes.c_void_p, W.LPWSTR, W.DWORD]
psapi.GetModuleInformation.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, W.DWORD]

THREAD_QUERY = 0x40 | 0x800          # THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION
SAMPLE_EVERY = 0.2
NM = os.environ.get('LLVM_NM', 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-nm.exe')
APP_COUNTER_OFFSET = 948          # offsetof(SexyAppBase, mUpdateCount) of the port's 32-bit build (the framework's header: see the notes)


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


class MODINFO(ctypes.Structure):
    _fields_ = [('base', ctypes.c_void_p), ('size', W.DWORD), ('entry', ctypes.c_void_p)]


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


def thread_cycles(h):
    c = ctypes.c_ulonglong()
    return c.value if k32.QueryThreadCycleTime(h, ctypes.byref(c)) else None


def process_cycles(h):
    c = ctypes.c_ulonglong()
    return c.value if k32.QueryProcessCycleTime(h, ctypes.byref(c)) else None


def modules(hp):
    """[(base, size, name)] of the process's modules, 32-bit and 64-bit; the exe is the first of a 32-bit process's own."""
    arr = (ctypes.c_void_p * 1024)()
    need = W.DWORD()
    if not psapi.EnumProcessModulesEx(hp, arr, ctypes.sizeof(arr), ctypes.byref(need), 3):
        return []
    out = []
    for m in arr[:need.value // ctypes.sizeof(ctypes.c_void_p)]:
        name = ctypes.create_unicode_buffer(260)
        psapi.GetModuleBaseNameW(hp, ctypes.c_void_p(m), name, 260)
        mi = MODINFO()
        psapi.GetModuleInformation(hp, ctypes.c_void_p(m), ctypes.byref(mi), ctypes.sizeof(mi))
        out.append((mi.base or 0, mi.size, name.value))
    return out


def thread_label(h, mods):
    """(name set by the thread, 'module+offset' of its start address)."""
    name = ''
    pw = ctypes.c_void_p()
    if k32.GetThreadDescription(h, ctypes.byref(pw)) >= 0 and pw.value:
        name = ctypes.wstring_at(pw.value)
        k32.LocalFree(pw)
    addr = ctypes.c_uint64()
    start = ''
    if ntdll.NtQueryInformationThread(h, 9, ctypes.byref(addr), 8, None) == 0 and addr.value:
        start = '0x%x' % addr.value
        for base, size, mod in mods:
            if base <= addr.value < base + size:
                start = '%s+0x%x' % (mod, addr.value - base)
                break
    return name, start


class Meter:
    """Reads a game process every SAMPLE_EVERY seconds: the counter, the process's cycles and each thread's."""

    def __init__(self, h, pid, created, counter_fn):
        self.h, self.pid, self.created, self.counter_fn = h, pid, created, counter_fn
        self.threads = {}                      # tid -> thread handle
        self.labels = {}
        self.samples = []                      # (age, counter, process cycles, {tid: cycles})
        self.next_enum = 0

    def take(self):
        now = time.time()
        if now >= self.next_enum:
            for tid in thread_ids(self.pid):
                if tid not in self.threads:
                    th = k32.OpenThread(THREAD_QUERY, False, tid)
                    if th:
                        self.threads[tid] = th
            self.next_enum = now + 1.0
        cyc = {}
        for tid, th in self.threads.items():
            c = thread_cycles(th)
            if c is not None:
                cyc[tid] = c
        pc = process_cycles(self.h)
        self.samples.append((now - self.created, self.counter_fn(), pc, cyc))

    def label_threads(self):
        mods = modules(self.h)
        for tid, th in self.threads.items():
            self.labels[tid] = thread_label(th, mods)

    def at_counter(self, target):
        """(age, process cycles, {tid: cycles}) when the counter reached `target`, interpolated between the two samples around it."""
        prev = None
        for s in self.samples:
            if s[1] is None:
                continue
            if s[1] >= target:
                if prev is None or s[1] == prev[1]:
                    return s[0], s[2], dict(s[3])
                f = min(1.0, max(0.0, (target - prev[1]) / (s[1] - prev[1])))
                age = prev[0] + f * (s[0] - prev[0])
                pcy = prev[2] + f * (s[2] - prev[2])
                cyc = {}
                for tid, c in s[3].items():
                    c0 = prev[3].get(tid, 0)
                    cyc[tid] = c0 + f * (c - c0)
                return age, pcy, cyc
            prev = s
        return None

    def report(self, lo, hi):
        a, b = self.at_counter(lo), self.at_counter(hi)
        if a is None or b is None:
            return None
        threads = []
        for tid, c in b[2].items():
            d = c - a[2].get(tid, 0)
            name, start = self.labels.get(tid, ('', ''))
            threads.append({'tid': tid, 'name': name, 'start': start, 'mcycles': round(d / 1e6, 1)})
        threads.sort(key=lambda t: -t['mcycles'])
        total = b[1] - a[1]
        return {'window_s': round(b[0] - a[0], 2), 'updates': hi - lo, 'process_mcycles': round(total / 1e6, 1),
                'threads_mcycles': round(sum(t['mcycles'] for t in threads), 1), 'threads': threads[:8]}


def exe_symbol(exe, symbol):
    """(address of `symbol` in the exe's symbol table, the exe's preferred image base)."""
    out = subprocess.run([NM, exe], capture_output=True, text=True).stdout
    addr = next((int(l.split()[0], 16) for l in out.splitlines() if l.rstrip().endswith(' ' + symbol)), None)
    with open(exe, 'rb') as f:
        data = f.read(4096)
    pe = int.from_bytes(data[0x3c:0x40], 'little')
    base = int.from_bytes(data[pe + 24 + 28:pe + 24 + 32], 'little')
    return addr, base


def port_run(out, src, extra=()):
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    lines, end = perf.scenario(src)
    sc = os.path.join(out, 'script.txt')
    with open(sc, 'w', newline='\n') as f:      # the scenario's inputs, and a quit a little after the stretch's end; nothing else
        for n, c, a in lines:
            f.write('%d %s %s\n' % (n, c, ' '.join(a)))
        f.write('%d quit\n' % (end + 150))
    exe, gamedir = perf.port_exe()
    ud = os.path.join(out, 'ud')
    perf.fresh_users(ud)
    sym, imagebase = exe_symbol(exe, '__ZN4Sexy12gSexyAppBaseE')
    others0 = perf.game_processes(perf.mine_prefixes())
    s0 = perf.system_times()
    log = open(os.path.join(out, 'game.log'), 'wb')
    p = subprocess.Popen([exe, '-gamedir=' + gamedir, '-userdir=' + ud, '-seed=1', '-script=' + sc, '-renderscale=1'] + list(extra), cwd=out,
                         stdout=log, stderr=subprocess.STDOUT)
    h = k32.OpenProcess(perf.QUERY | perf.VM_READ | perf.QUERY_LIMITED, False, p.pid)
    created = perf.creation_time(h)
    state = {'addr': None}

    def counter():
        if state['addr'] is None:
            mods = modules(h)
            exe_mod = next((m for m in mods if m[2].lower() == os.path.basename(exe).lower()), None)
            if exe_mod is None or sym is None:
                return None
            state['addr'] = exe_mod[0] + (sym - imagebase if sym >= imagebase else sym)
        app = perf.read32(h, state['addr'])
        return perf.read32(h, app + APP_COUNTER_OFFSET) if app else None

    meter = Meter(h, p.pid, created, counter)
    while p.poll() is None:
        meter.take()
        time.sleep(SAMPLE_EVERY)
        if len(meter.samples) % 50 == 0:
            meter.label_threads()
    s1 = perf.system_times()
    meter.label_threads()
    rep = meter.report(end - perf.STRETCH, end)
    renderer = ''
    text = open(os.path.join(out, 'game.log'), errors='replace').read()
    m = re.search(r'[Rr]enderer[^\n]*', text)
    if m:
        renderer = m.group(0)[:80]
    res = {'side': 'port', 'renderer': renderer or 'SDL, Direct3D 11 (3D on, the default)', 'exit': p.returncode,
           'machine_busy': round(perf.busy_fraction(s0, s1), 2), 'others_start': len(others0),
           'others_end': len(perf.game_processes(perf.mine_prefixes()))}
    if rep:
        res.update(rep)
        res['updates_per_s'] = round(rep['updates'] / rep['window_s'], 1)
    json.dump({'samples': [(a, c, pc, cy) for a, c, pc, cy in meter.samples], 'labels': meter.labels, 'result': res},
              open(os.path.join(out, 'cyc.json'), 'w'))
    k32.CloseHandle(h)
    print(json.dumps(res), flush=True)


def orig_run(out, src):
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    root = os.getcwd()
    origrun = os.path.join(root, 'build', 'origrun-bin', 'origrun.exe')
    exe = os.path.join(root, perf.GAME, 'TyperSharkGame.exe')
    patches = os.path.join(root, os.environ.get('PERF_PATCHES', os.path.join('build', 'ts-p2', 'patches.txt')))
    sc, end = perf.orig_script(src, os.path.join(out, 'script.txt'))
    users = os.path.join(root, perf.GAME, 'users')
    shutil.rmtree(users, ignore_errors=True)
    shutil.copytree(os.path.join(root, 'build', 'ts-p1', 'fixtures', 'users-clean'), users)
    subprocess.run(['reg', 'delete', r'HKCU\Software\PopCap\TyperShark', '/f'], capture_output=True)
    others0 = perf.game_processes(perf.mine_prefixes())
    s0 = perf.system_times()
    log = open(os.path.join(out, 'origrun.log'), 'wb')
    p = subprocess.Popen([origrun, '--exe', exe, '--run', 'typershark', '--script', sc, '--clock', 'real', '--cursor', 'scripted',
                          '--reg-map', 'popcap', '--seed', '1', '--counter', perf.COUNTER_EXPR, '--patches', patches, '--timeout', '900'],
                         stdout=log, stderr=subprocess.STDOUT)
    h = None
    meter = None
    c_ready = None
    done = False
    deadline = time.time() + 1200
    next_take = 0
    while p.poll() is None and not done and time.time() < deadline:
        if h is None:
            pid = perf.find_process('TyperSharkGame.exe', '\\' + perf.GAME.replace('/', '\\') + '\\')
            if pid:
                h = k32.OpenProcess(perf.QUERY | perf.VM_READ | perf.QUERY_LIMITED | perf.TERMINATE, False, pid)
                created = perf.creation_time(h)

                def counter(h=h):
                    app = perf.read32(h, perf.APP)
                    return perf.read32(h, app + perf.COUNTER_OFFSET) if app else None
                meter = Meter(h, pid, created, counter)
        if meter is not None and time.time() >= next_take:
            meter.take()
            next_take = time.time() + SAMPLE_EVERY
            app = perf.read32(h, perf.APP)
            if c_ready is None and app:
                flag = perf.read32(h, app + perf.LOADED_OFFSET)
                if flag is not None and (flag & 0xff):
                    c_ready = perf.read32(h, app + perf.COUNTER_OFFSET)
            c = meter.samples[-1][1]
            if c_ready is not None and c is not None and c >= c_ready + end + 100:
                done = True
            if len(meter.samples) % 50 == 0:
                meter.label_threads()
        time.sleep(0.01)
    s1 = perf.system_times()
    if meter is not None:
        meter.label_threads()
    if h:
        k32.TerminateProcess(h, 0)
    try:
        p.wait(30)
    except subprocess.TimeoutExpired:
        p.terminate()
    log.close()
    text = open(os.path.join(out, 'origrun.log'), errors='replace').read()
    marks = [int(m) for m in re.findall(r'mark: the counter reads (\d+)', text)]
    states = [[int(v) for v in re.findall(r'=(-?\d+)', line.split('peek u=')[1])[1:]] for line in text.split('\n') if 'peek u=' in line]
    played = bool(states) and states[-1][1] != 0
    res = {'side': 'orig', 'renderer': 'DirectDraw (2D; a hidden desktop gives it no Direct3D 9)', 'exit': p.returncode,
           'machine_busy': round(perf.busy_fraction(s0, s1), 2), 'others_start': len(others0),
           'others_end': len(perf.game_processes(perf.mine_prefixes())), 'game_played': played, 'counter_at_mark': marks[0] if marks else None}
    if meter is not None and marks:
        rep = meter.report(marks[0] + end - 1 - perf.STRETCH, marks[0] + end - 1)
        if rep:
            res.update(rep)
            res['updates_per_s'] = round(rep['updates'] / rep['window_s'], 1)
    if meter is not None:
        json.dump({'samples': [(a, c, pc, cy) for a, c, pc, cy in meter.samples], 'labels': meter.labels, 'result': res},
                  open(os.path.join(out, 'cyc.json'), 'w'))
    print(json.dumps(res), flush=True)


def thread_class(side, t):
    """The role of a thread: the port's main thread starts at the exe's entry (a page boundary), its audio thread is SDL's, and the
    other busy thread is the Direct3D driver's; the original's main thread starts at its CRT entry, the next is the cursor thread
    (SexyApp::CursorThreadProc) and WINMM's is the timer's."""
    start, name = t['start'], t['name']
    if side.startswith('port'):
        if name.startswith('SDLAudio'):
            return 'audio'
        return 'main' if start.endswith('1000') and 'dll' not in start else 'driver'
    if '10a644' in start or '50a644' in start:
        return 'main'
    if '10b957' in start or '50b957' in start:
        return 'cursor thread'
    return 'other'


def summary(*paths):
    """The runs.txt of pairs2.sh (several may be given): a line a run, then the medians of each side and of each kind of thread."""
    sides, classes = {}, {}
    for path in paths:
        for line in open(path):
            if '{' not in line:
                continue
            kind = line.split('{')[0].strip()
            r = json.loads(line[line.index('{'):])
            if 'process_mcycles' not in r:
                print(kind, 'NO RESULT', r.get('error') or r.get('exit'))
                continue
            top = ', '.join('%s %.0f' % (t['name'] or t['start'] or t['tid'], t['mcycles']) for t in r['threads'][:4])
            print('%-12s %8.0f Mcycles in %.1f s (%.1f updates/s; threads %.0f; others %s-%s; busy %.2f): %s' % (
                kind, r['process_mcycles'], r['window_s'], r['updates_per_s'], r['threads_mcycles'], r['others_start'], r['others_end'],
                r['machine_busy'], top))
            if r.get('game_played') is False:
                print('   NOT PLAYED (left out of the medians)')
                continue
            sides.setdefault(kind, []).append(r['process_mcycles'])
            per = {}
            for t in r['threads']:
                c = thread_class(kind, t)
                per[c] = per.get(c, 0) + t['mcycles']
            for c, v in per.items():
                classes.setdefault(kind, {}).setdefault(c, []).append(v)
    for kind, v in sides.items():
        print('median %-12s %8.0f Mcycles (%d runs: %s)' % (kind, statistics.median(v), len(v), ' '.join('%.0f' % x for x in v)))
        for c, vals in sorted(classes.get(kind, {}).items(), key=lambda kv: -statistics.median(kv[1])):
            print('    %-14s median %7.0f Mcycles (%s)' % (c, statistics.median(vals), ' '.join('%.0f' % x for x in vals)))


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else ''
    scn = sys.argv[3] if len(sys.argv) > 3 and not sys.argv[3].startswith('-') else perf.DEFAULT_SCENARIO
    if mode == 'port':
        port_run(sys.argv[2], scn, [a for a in sys.argv[3:] if a.startswith('-')])
    elif mode == 'orig':
        orig_run(sys.argv[2], scn)
    elif mode == 'summary':
        summary(*sys.argv[2:])
    else:
        sys.exit(__doc__)
