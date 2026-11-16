"""memrun.py: where Bookworm Adventures' memory goes, the port's and the original's, read the same way on both sides.

  port:      build/native/rundesk.exe python -I games/bookwormadv/tools/profile/memrun.py port <out> <hold s> [--reg K=V ...]
             [--cmd T:command ...] -- <game args>
  original:  python -I games/bookwormadv/tools/profile/memrun.py orig <out> <hold s> -- <origrun args>

A port run starts build/native/bookwormadv.exe with a copy of the test profile (tests/menu: a player past the first book,
so the game goes from its title to the main menu) in <out>/user, the registry lines given (`--reg Is3D=dword:0`), a live
`-commands` channel and the game's log in <out>/stdout.txt. An original run starts origrun with the arguments given and
waits for the copy's bookwormadventures.exe. Both sample the game's private bytes and working set every 50 ms from its
start. `--cmd 30:lua ...` sends a command at 30 s (port only); `--map 12` reads the memory map at 12 s as well, with every private allocation's committed bytes in regions12.json. At <hold> s the port answers `memreport <out>/memreport.txt`
(its line goes to the log) and tools/memmap.py reads either process from outside into <out>/memmap.json; then the port
is told to quit and the original is left to origrun's timeout. `--cycles 30,90` reads the process's cycle count
(QueryProcessCycleTime) and each thread's at 30 and 90 s from outside; `--nochannel` starts the port with no -commands
channel (no memreport then: a processor-time run) and ends it at <hold>. <out>/summary.json has the peaks and the reading at <hold>.
"""
import ctypes, ctypes.wintypes as W, json, os, shutil, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..', '..'))
k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, ctypes.POINTER(W.DWORD)]
psapi.GetProcessMemoryInfo.argtypes = [W.HANDLE, ctypes.c_void_p, W.DWORD]
psapi.EnumProcesses.argtypes = [ctypes.POINTER(W.DWORD), W.DWORD, ctypes.POINTER(W.DWORD)]
QUERY = 0x1000 | 0x400 | 0x10


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


def counters(h):
    p = PMC()
    p.cb = ctypes.sizeof(p)
    if not psapi.GetProcessMemoryInfo(h, ctypes.byref(p), p.cb):
        return None
    return p.PrivateUsage, p.WorkingSetSize


class MBI(ctypes.Structure):
    _fields_ = [('BaseAddress', ctypes.c_void_p), ('AllocationBase', ctypes.c_void_p), ('AllocationProtect', W.DWORD),
                ('PartitionId', W.WORD), ('RegionSize', ctypes.c_size_t), ('State', W.DWORD), ('Protect', W.DWORD),
                ('Type', W.DWORD)]


def regions(h):
    """The private allocations' committed bytes, by allocation base: {base: [committed, reserved, protect]}."""
    k32.VirtualQueryEx.argtypes = [W.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
    k32.VirtualQueryEx.restype = ctypes.c_size_t
    out, addr, m = {}, 0, MBI()
    while addr < 0xFFFF0000 and k32.VirtualQueryEx(h, ctypes.c_void_p(addr), ctypes.byref(m), ctypes.sizeof(m)):
        if m.Type == 0x20000 and m.State in (0x1000, 0x2000):
            r = out.setdefault('%08x' % (m.AllocationBase or 0), [0, 0, 0])
            r[0 if m.State == 0x1000 else 1] += m.RegionSize
            if m.State == 0x1000:
                r[2] = m.Protect
        addr = (m.BaseAddress or 0) + m.RegionSize
    return out


class THREADENTRY32(ctypes.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('th32ThreadID', W.DWORD), ('th32OwnerProcessID', W.DWORD),
                ('tpBasePri', W.LONG), ('tpDeltaPri', W.LONG), ('dwFlags', W.DWORD)]


def cycles(h, pid):
    """The process's cycle count (every thread, ended ones too) and each live thread's, with its description."""
    k32.QueryProcessCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
    k32.QueryThreadCycleTime.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_uint64)]
    k32.OpenThread.restype = W.HANDLE
    k32.GetThreadDescription.argtypes = [W.HANDLE, ctypes.POINTER(ctypes.c_wchar_p)]
    k32.CreateToolhelp32Snapshot.restype = W.HANDLE
    total = ctypes.c_uint64()
    k32.QueryProcessCycleTime(h, ctypes.byref(total))
    threads = {}
    snap = k32.CreateToolhelp32Snapshot(4, 0)
    te = THREADENTRY32()
    te.dwSize = ctypes.sizeof(te)
    ok = k32.Thread32First(snap, ctypes.byref(te))
    while ok:
        if te.th32OwnerProcessID == pid:
            th = k32.OpenThread(0x800, False, te.th32ThreadID)
            if th:
                c = ctypes.c_uint64()
                k32.QueryThreadCycleTime(th, ctypes.byref(c))
                name = ctypes.c_wchar_p()
                desc = ''
                if k32.GetThreadDescription(th, ctypes.byref(name)) >= 0 and name.value:
                    desc = name.value
                threads[te.th32ThreadID] = [c.value, desc]
                k32.CloseHandle(th)
        ok = k32.Thread32Next(snap, ctypes.byref(te))
    k32.CloseHandle(snap)
    return {'process': total.value, 'threads': threads}


def find(part):
    ids = (W.DWORD * 4096)()
    n = W.DWORD()
    psapi.EnumProcesses(ids, ctypes.sizeof(ids), ctypes.byref(n))
    for pid in ids[:n.value // 4]:
        h = k32.OpenProcess(QUERY, False, pid)
        if not h:
            continue
        buf = ctypes.create_unicode_buffer(1024)
        size = W.DWORD(1024)
        if k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(size)) and part.lower() in buf.value.lower():
            return pid, h
        k32.CloseHandle(h)
    return None, None


def main():
    side, out, hold = sys.argv[1], os.path.abspath(sys.argv[2]), float(sys.argv[3])
    rest = sys.argv[4:]
    extra = rest[rest.index('--') + 1:] if '--' in rest else []
    opts = rest[:rest.index('--')] if '--' in rest else rest
    regs = [opts[i + 1] for i in range(len(opts)) if opts[i] == '--reg']
    maps = sorted(float(opts[i + 1]) for i in range(len(opts)) if opts[i] == '--map')
    cmds = sorted((float(c.split(':', 1)[0]), c.split(':', 1)[1]) for c in
                  [opts[i + 1] for i in range(len(opts)) if opts[i] == '--cmd'])
    cyc = [float(v) for o in [opts[i + 1] for i in range(len(opts)) if opts[i] == '--cycles'] for v in o.split(',')]
    nochannel = '--nochannel' in opts
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    if side == 'port':
        shutil.copytree(os.path.join(ROOT, 'games', 'bookwormadv', 'tests', 'menu', 'user'), os.path.join(out, 'user'))
        if regs:
            with open(os.path.join(out, 'user', 'registry.ini'), 'a') as f:
                f.write(''.join(r + '\n' for r in regs))
        chan = os.path.join(out, 'cmd.txt')
        open(chan, 'w').close()
        log = open(os.path.join(out, 'stdout.txt'), 'w')
        proc = subprocess.Popen([os.path.join(ROOT, 'build', 'native', 'bookwormadv.exe'),
                                 '-userdir=' + os.path.join(out, 'user')] + ([] if nochannel else ['-commands=' + chan]) + extra,
                                stdout=log, stderr=subprocess.STDOUT)
        pid = proc.pid
        h = k32.OpenProcess(QUERY, False, pid)
    else:
        log = open(os.path.join(out, 'origrun.txt'), 'w')
        proc = subprocess.Popen([os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe')] + extra, stdout=log,
                                stderr=subprocess.STDOUT)
        pid, h = None, None
        while pid is None and proc.poll() is None:
            pid, h = find('\\game\\bookwormadventures.exe')
            time.sleep(0.05)
        if pid is None:
            print('memrun: no original started')
            return 1
    t0 = time.time()
    peak_priv = peak_ws = 0
    at_hold = None
    samples = []
    cycle_reads = []

    def send(c):
        with open(os.path.join(out, 'cmd.txt'), 'a') as f:
            f.write(c + '\n')

    while True:
        c = counters(h)
        t = time.time() - t0
        if c is None or not c[0] or proc.poll() is not None:
            break
        samples.append((round(t, 2), c[0], c[1]))
        peak_priv, peak_ws = max(peak_priv, c[0]), max(peak_ws, c[1])
        while side == 'port' and cmds and t >= cmds[0][0]:
            send(cmds.pop(0)[1])
        while cyc and t >= cyc[0]:
            cyc.pop(0)
            cycle_reads.append(dict(cycles(h, pid), t=round(time.time() - t0, 3)))
        while maps and t >= maps[0]:
            json.dump(regions(h), open(os.path.join(out, 'regions%g.json' % maps[0]), 'w'))
            subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'memmap.py'), str(pid), '--json',
                            os.path.join(out, 'memmap%g.json' % maps[0])],
                           stdout=open(os.path.join(out, 'memmap%g.txt' % maps.pop(0)), 'w'), stderr=subprocess.STDOUT)
        if at_hold is None and t >= hold:
            at_hold = {'t': round(t, 2), 'private': c[0], 'workingSet': c[1], 'peakPrivateSoFar': peak_priv}
            if side == 'port' and not nochannel:
                send('memreport ' + os.path.join(out, 'memreport.txt'))
                time.sleep(1.0)
            subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'memmap.py'), str(pid), '--json',
                            os.path.join(out, 'memmap.json')], stdout=open(os.path.join(out, 'memmap.txt'), 'w'),
                           stderr=subprocess.STDOUT)
            if side == 'port':
                send('quit')
        if side == 'port' and at_hold and (nochannel or t > at_hold['t'] + 30):
            proc.kill()
            break
        time.sleep(0.05)
    proc.wait()
    summary = {'side': side, 'pid': pid, 'peakPrivate': peak_priv, 'peakWorkingSet': peak_ws, 'atHold': at_hold,
               'seconds': samples[-1][0] if samples else 0}
    if len(cycle_reads) >= 2:
        a, b = cycle_reads[0], cycle_reads[-1]
        per = {}
        for tid, (c, desc) in b['threads'].items():
            per[tid] = [c - a['threads'].get(tid, [0])[0], desc]
        audio = sum(c for c, desc in per.values() if desc.lower().startswith('sdlaudio'))
        summary['cycles'] = {'from': a['t'], 'to': b['t'], 'process': b['process'] - a['process'], 'audio': audio,
                             'threads': sorted(per.values(), reverse=True)[:8]}
    json.dump(summary, open(os.path.join(out, 'summary.json'), 'w'), indent=1)
    json.dump(samples, open(os.path.join(out, 'samples.json'), 'w'))
    mb = lambda v: '%.1f' % (v / 1048576.0) if v else '-'
    print('memrun %s: peak private %s MB, at %ss private %s MB, working set peak %s MB' % (
        side, mb(peak_priv), at_hold and at_hold['t'], mb(at_hold and at_hold['private']), mb(peak_ws)))
    if 'cycles' in summary:
        cy = summary['cycles']
        print('memrun %s: %.3f G cycles from %.1f to %.1f s (%.3f G without the SDL audio thread)' % (
            side, cy['process'] / 1e9, cy['from'], cy['to'], (cy['process'] - cy['audio']) / 1e9))
    return 0


if __name__ == '__main__':
    sys.exit(main())
