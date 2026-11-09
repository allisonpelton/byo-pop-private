# Performance of the original and the port, measured the same way (games/alchemy/docs/GAME_NOTES.md, "Performance").
#
#   python perf.py port load <out dir> [seed]        the port's load: launch to the title's play link, over -commands (state's "ready")
#   python perf.py port play <out dir> <script>      the port plays a -script file in real time (no -fast); the script ends with "quit"
#   python perf.py orig load <out dir>               the original under origrun (real clock): launch to the title's play link
#   python perf.py orig play <out dir> <origrun script>
#
# Run a port under rundesk (build/native/rundesk.exe python perf.py ...); start the original's run directly, since origrun
# puts the game under rundesk itself (docs/ORIGRUN.md). The sampler reads the game process only: its working set, private
# bytes and processor time every 100 ms, the peaks Windows keeps (PeakWorkingSetSize, PeakPagefileUsage = peak private
# bytes), and how many game processes run on the machine (any session's: a process under a \build\ folder or C:\byo-og
# that is not a tool). The original's "launch" is its process's creation time; its first input is the title's play
# link, read from the game's memory as origrun's --watch reads it (the app object at 0x5160b0, mLoadingThreadCompleted at +0x52e,
# the title screen at +0xe4c with its "link shown" byte at +0x70). The port's is the first "ready":true of its state command.
import ctypes, ctypes.wintypes as W, json, os, re, subprocess, sys, threading, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
ROOT = os.getcwd()
EXE = os.path.join(ROOT, 'build', 'native', 'alchemy.exe')
ORIGRUN = os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe')
OG_GAME = 'C:/byo-og/alch/game/AlchemyGame.exe'
TOOLS = {'rundesk.exe', 'python.exe', 'bash.exe', 'cmake.exe', 'ninja.exe', 'node.exe', 'sh.exe', 'conhost.exe', 'pythonw.exe'}


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


class PE(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ProcessID', W.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                ('th32ModuleID', W.DWORD), ('cntThreads', W.DWORD), ('th32ParentProcessID', W.DWORD),
                ('pcPriClassBase', ctypes.c_long), ('dwFlags', W.DWORD), ('szExeFile', ctypes.c_wchar * 260)]


k32.OpenProcess.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.ReadProcessMemory.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
PQI, VMR = 0x400, 0x10
MB = float(1 << 20)


def processes():
    snap = k32.CreateToolhelp32Snapshot(2, 0)
    out = []
    e = PE(); e.dwSize = ctypes.sizeof(PE)
    ok = k32.Process32FirstW(snap, ctypes.byref(e))
    while ok:
        out.append((e.th32ProcessID, e.th32ParentProcessID, e.szExeFile))
        ok = k32.Process32NextW(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


def descendants(pid, name):
    procs = processes()
    kids, found, todo = {}, [], [pid]
    for p, pp, n in procs:
        kids.setdefault(pp, []).append((p, n))
    seen = set()
    while todo:
        c = todo.pop()
        for p, n in kids.get(c, []):
            if p in seen:
                continue
            seen.add(p)
            if n.lower() == name.lower():
                found.append(p)
            todo.append(p)
    return found


def game_processes():
    n = 0
    for p, pp, name in processes():
        if name.lower() in TOOLS:
            continue
        h = k32.OpenProcess(0x1000, False, p)            # PROCESS_QUERY_LIMITED_INFORMATION
        if not h:
            continue
        buf = ctypes.create_unicode_buffer(1024)
        sz = W.DWORD(1024)
        if k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(sz)):
            low = buf.value.lower().replace('/', '\\')
            if '\\build\\' in low or low.startswith('c:\\byo-og'):
                n += 1
        k32.CloseHandle(h)
    return n


def ft(t):
    return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7


def times(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    return ft(a), ft(c) + ft(d)                           # creation (FILETIME seconds), kernel + user


def now_ft():
    t = W.FILETIME()
    k32.GetSystemTimeAsFileTime(ctypes.byref(t))
    return ft(t)


def mem(h):
    m = PMC(); m.cb = ctypes.sizeof(PMC)
    psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb)
    return m


def rd(h, addr, n):
    buf = ctypes.create_string_buffer(n)
    got = ctypes.c_size_t(0)
    if not k32.ReadProcessMemory(h, ctypes.c_void_p(addr), buf, n, ctypes.byref(got)) or got.value != n:
        return None
    return buf.raw


def u32(h, addr):
    b = rd(h, addr, 4)
    return int.from_bytes(b, 'little') if b else None


def u8(h, addr):
    b = rd(h, addr, 1)
    return b[0] if b else None


class Reader(threading.Thread):
    """Reads the port's stdout and stderr as lines with the wall time they arrived."""
    def __init__(self, stream):
        super().__init__(daemon=True)
        self.stream, self.lines = stream, []
        self.start()

    def run(self):
        for raw in iter(self.stream.readline, b''):
            self.lines.append((time.time(), raw.decode('utf-8', 'replace').rstrip('\r\n')))


def main():
    kind, what, out = sys.argv[1], sys.argv[2], os.path.abspath(sys.argv[3])
    extra = sys.argv[4] if len(sys.argv) > 4 else None
    os.makedirs(out, exist_ok=True)
    user = os.path.join(out, 'user')
    subprocess.run(['cmd', '/c', 'rmdir', '/s', '/q', user], capture_output=True)
    os.makedirs(user)
    cf = os.path.join(out, 'commands.txt')
    reader = None
    if kind == 'port':
        args = [EXE, '-userdir=' + user, '-seed=11'] + os.environ.get('PERF_PORT_ARGS', '').split()
        if what == 'load':
            open(cf, 'w').close()
            args += ['-commands=' + cf]
        else:
            args += ['-script=' + os.path.abspath(extra)]
        p = subprocess.Popen(args, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        reader = Reader(p.stdout)
        game = p.pid
    else:
        for f in ('hiscores.dat', 'savegame.dat'):
            try:
                os.remove('C:/byo-og/alch/game/' + f)
            except OSError:
                pass
        script = os.path.join(out, 'orig.txt')
        if what == 'load':
            open(script, 'w').write('wait 14000\nquit\n')
        else:
            open(script, 'w').write(open(os.path.abspath(extra)).read())
        args = [ORIGRUN, '--exe', OG_GAME, '--run', 'alch', '--script', script, '--clock', 'real', '--counter', '0x516570',
                '--update-ms', '10', '--seed', '11', '--timeout', '200'] + (['--patches', os.environ['PERF_ORIG_PATCHES']] if os.environ.get('PERF_ORIG_PATCHES') else [])
        p = subprocess.Popen(args, cwd=ROOT, stdout=open(os.path.join(out, 'origrun.log'), 'w'), stderr=subprocess.STDOUT)
        game = None
    t_wall0 = time.time()
    h = None
    created = None
    samples, counts = [], []
    dump, dumps = None, []
    first_input = None
    loaded_at = None
    peak_ws = peak_priv = 0
    last_sample = last_count = last_state = 0.0
    state_pos = 0
    cmd_lines = 0
    cpu_last = 0.0
    sent_close = False
    hung = False
    while True:
        t = time.time()
        if h is None:
            if kind == 'orig':
                kids = descendants(p.pid, 'AlchemyGame.exe')
                game = kids[0] if kids else None
            if game:
                h = k32.OpenProcess(PQI | VMR | 0x100000, False, game)   # + SYNCHRONIZE
                if h:
                    created, _ = times(h)
            if h is None:
                if p.poll() is not None:
                    break
                time.sleep(0.01)
                continue
        alive = k32.WaitForSingleObject(h, 0) == 0x102
        if alive:
            m = mem(h)
            peak_ws, peak_priv = m.PeakWorkingSetSize, m.PeakPagefileUsage
            if t - last_sample >= 0.1:
                last_sample = t
                _, cpu = times(h)
                cpu_last = cpu
                samples.append((round(now_ft() - created, 3), m.WorkingSetSize, m.PagefileUsage, round(cpu, 3)))
            if t - last_count >= 5.0:
                last_count = t
                counts.append(game_processes())
                if kind == 'orig':    # the app object's dwords from +0x480, to find the framework's counters (the draw count among them)
                    dump = rd(h, 0x5160b0 + 0x480, 0x100) or dump
                    dumps.append((round(now_ft() - created, 1), dump.hex() if dump else None))
            if first_input is None:
                if kind == 'orig':
                    app = 0x5160b0                       # the app is a global object there, not a pointer
                    if True:
                        if loaded_at is None and u8(h, app + 0x52e):
                            loaded_at = now_ft() - created
                        title = u32(h, app + 0xe4c)
                        if loaded_at is not None and title and u8(h, title + 0x70):
                            first_input = now_ft() - created
                elif what == 'load' and t - last_state >= 0.05:
                    last_state = t
                    with open(cf, 'a', newline='') as f:
                        f.write('state\n')
                    for lt, ln in reader.lines[state_pos:]:
                        mo = re.search(r'STATE (\{.*\})', ln)
                        if mo and first_input is None:
                            try:
                                if json.loads(mo.group(1)).get('ready'):
                                    # the line's arrival (Unix seconds) against the process's creation (FILETIME seconds from 1601)
                                    first_input = lt - (created - 11644473600.0)
                            except ValueError:
                                pass
                    state_pos = len(reader.lines)
            if kind == 'port' and what == 'load' and first_input is not None and not sent_close and t - t_wall0 > 0:
                time.sleep(1.0)
                with open(cf, 'a', newline='') as f:
                    f.write('quit\n')
                sent_close = True
        else:
            break
        if t - t_wall0 > (60 if what == 'load' else 150):          # a run that never ends is stopped here, and says so
            hung = True
            if h:
                k32.TerminateProcess(h, 1)
            p.kill()
            break
        time.sleep(0.01)
    end = now_ft() - created if created else None
    try:
        p.wait(30)
    except Exception:
        pass
    res = dict(kind=kind, what=what, stopped_by_the_cap=hung, launch_to_first_input_s=first_input, loaded_flag_s=loaded_at, ran_s=end,
               peak_working_set_mb=peak_ws / MB, peak_private_mb=peak_priv / MB, cpu_total_s=cpu_last,
               game_processes=dict(samples=counts, min=min(counts) if counts else None, max=max(counts) if counts else None,
                                   mean=round(sum(counts) / len(counts), 1) if counts else None),
               samples=samples, app_dumps=dumps)
    # the processor time of the last 60 s of the run (the scripted minute), and of the part before it
    if samples:
        tend = samples[-1][0]
        tmin = max(0.0, tend - 60.0)
        before = [s for s in samples if s[0] <= tmin]
        c0 = before[-1][3] if before else 0.0
        res['cpu_before_last_minute_s'] = c0
        res['cpu_last_minute_s'] = round(samples[-1][3] - c0, 3)
        res['mean_working_set_last_minute_mb'] = round(sum(s[1] for s in samples if s[0] > tmin) / max(1, len([s for s in samples if s[0] > tmin])) / MB, 1)
    json.dump(res, open(os.path.join(out, 'perf.json'), 'w'))
    short = {k: v for k, v in res.items() if k not in ('samples', 'app_dumps')}
    print(json.dumps(short))


main()
