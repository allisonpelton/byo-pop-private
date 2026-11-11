# One measured run of a port, for the comparison of the default drawing path with SDL's software renderer
# (games/alchemy/docs/GAME_NOTES.md, "The software path"). Run it under rundesk, which makes the game invisible and silent:
#
#   build/native/rundesk.exe python -I games/alchemy/tools/profile/softpath.py <out.json> --exe <game exe> --script <file>
#       [--fullscreen 0|1] [--regkey <key>] [--env NAME=VALUE]... [--timeout <seconds>] [-- <game arguments>]
#
# It starts the game on a fresh user folder with -seed=11 and the script, and writes what the run took: the wall time from
# the process's creation to its exit, its processor time (kernel and user) and cycles, its peak working set and private
# bytes, every MEMREPORT line the script asked for (the framework's "memreport" test command) with the update it was
# given at, and the framework's closing line (the update and draw counts). --fullscreen writes the screen mode into the
# user folder's registry.ini before the start (--regkey is the game's registry key, "PopCap\Alchemy" by default), so
# the run starts windowed or in full screen whatever the game's own default is. The environment variables go to the game
# (SDL_RENDER_DRIVER=software selects the software renderer).
import argparse, ctypes, ctypes.wintypes as W, json, os, re, subprocess, sys, threading, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
k32.OpenProcess.restype = W.HANDLE
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD), ('PeakWorkingSetSize', ctypes.c_size_t),
                ('WorkingSetSize', ctypes.c_size_t), ('QuotaPeakPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPagedPoolUsage', ctypes.c_size_t), ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t),
                ('QuotaNonPagedPoolUsage', ctypes.c_size_t), ('PagefileUsage', ctypes.c_size_t),
                ('PeakPagefileUsage', ctypes.c_size_t), ('PrivateUsage', ctypes.c_size_t)]


def ft(v):
    return ((v.dwHighDateTime << 32) | v.dwLowDateTime) / 1e7


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out')
    ap.add_argument('--exe', required=True)
    ap.add_argument('--script', required=True)
    ap.add_argument('--fullscreen', type=int, choices=(0, 1))
    ap.add_argument('--regkey', default='PopCap\\Alchemy')
    ap.add_argument('--env', action='append', default=[])
    ap.add_argument('--timeout', type=float, default=600)
    argv = sys.argv[1:]
    rest = []
    if '--' in argv:        # what follows goes to the game
        rest = argv[argv.index('--') + 1:]
        argv = argv[:argv.index('--')]
    a = ap.parse_args(argv)
    a.rest = rest
    out = os.path.abspath(a.out)
    user = out[:-5] + '-user' if out.endswith('.json') else out + '-user'
    subprocess.run(['cmd', '/c', 'rmdir', '/s', '/q', user], capture_output=True)
    os.makedirs(user)
    if a.fullscreen is not None:
        with open(os.path.join(user, 'registry.ini'), 'w') as f:
            f.write('; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\\%s]\nScreenMode=dword:%d\n' % (a.regkey, a.fullscreen))
    env = dict(os.environ)
    for e in a.env:
        k, _, v = e.partition('=')
        env[k] = v
    order = []      # the memreport lines' updates, in the order the script gives them
    for line in open(a.script):
        p = line.split()
        if len(p) >= 2 and p[1] == 'memreport':
            order.append(int(p[0]))
    args = [os.path.abspath(a.exe), '-userdir=' + user, '-seed=11', '-script=' + os.path.abspath(a.script)] + a.rest
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
    lines = []

    def reader():
        for raw in iter(p.stdout.readline, b''):
            lines.append(raw.decode('utf-8', 'replace').rstrip('\r\n'))
    t = threading.Thread(target=reader, daemon=True)
    t.start()
    h = k32.OpenProcess(0x0400 | 0x0010 | 0x100000, False, p.pid)
    peak_ws = peak_priv = 0
    cycles = ctypes.c_ulonglong(0)
    last_cycles = 0
    started = time.time()
    timed_out = False
    while p.poll() is None:
        m = PMC()
        m.cb = ctypes.sizeof(m)
        if k32.K32GetProcessMemoryInfo(h, ctypes.byref(m), m.cb):
            peak_ws, peak_priv = max(peak_ws, m.PeakWorkingSetSize), max(peak_priv, m.PeakPagefileUsage)
        if k32.QueryProcessCycleTime(h, ctypes.byref(cycles)):
            last_cycles = cycles.value
        if time.time() - started > a.timeout:
            timed_out = True
            p.kill()
            break
        time.sleep(0.05)
    p.wait()
    t.join(5)
    c, e, kt, ut = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(c), ctypes.byref(e), ctypes.byref(kt), ctypes.byref(ut))
    if k32.QueryProcessCycleTime(h, ctypes.byref(cycles)):
        last_cycles = cycles.value
    k32.CloseHandle(h)
    reports = [json.loads(l.split('MEMREPORT ', 1)[1]) for l in lines if 'MEMREPORT ' in l]
    closing = None
    for l in lines:
        mm = re.search(r'Seconds = ([\d.]+), Sleep Count = (\d+), Update Count = (\d+), Draw Count = (\d+)', l)
        if mm:
            closing = {'updates': int(mm.group(3)), 'draws': int(mm.group(4))}
    states = [l.split('STATE ', 1)[1][:400] for l in lines if 'STATE ' in l]      # a script's "state" lines: which screen it was on
    res = {
        'args': a.rest, 'env': a.env, 'fullscreen': a.fullscreen, 'exit': p.returncode, 'timed_out': timed_out,
        'wall_s': round(ft(e) - ft(c), 3), 'cpu_s': round(ft(kt) + ft(ut), 3), 'cycles': last_cycles,
        'peak_working_set': peak_ws, 'peak_private': peak_priv,
        'memreports': [{'update': order[i] if i < len(order) else None, **r} for i, r in enumerate(reports)],
        'closing': closing, 'states': states, 'log_lines': len(lines), 'pid': p.pid,
    }
    json.dump(res, open(out, 'w'), indent=1)
    mb = lambda n: round(n / 1048576.0, 1)
    print('%s: wall %.1f s, cpu %.2f s, %.2f G cycles, peak private %s MB, %s, memreports %d%s' % (
        os.path.basename(out), res['wall_s'], res['cpu_s'], last_cycles / 1e9, mb(peak_priv), closing, len(reports),
        ' TIMED OUT' if timed_out else ''))


if __name__ == '__main__':
    main()
