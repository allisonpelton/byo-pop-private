# memstart.py <game> <pairs> <out dir>: the port in a 800x600 window, in surface mode and with the renderer (SURFACE_DISPLAY=0) in turn:
# the time from the process's start to the screenshot of its first update, and its private bytes and working set at the end
# of a run of 12 s. Run under rundesk: build/native/rundesk.exe python -I games/mahjongjapan/tools/display/memstart.py mahjongjapan 5 build/memstart
import ctypes, ctypes.wintypes as W, os, shutil, statistics, subprocess, sys, time

game, pairs, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
root = os.getcwd()
section = {'mahjongjapan': 'Mahjong Escape Ancient Japan', 'mahjongchina': 'Mahjong Escape'}[game]


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD), ('PeakWorkingSetSize', ctypes.c_size_t), ('WorkingSetSize', ctypes.c_size_t),
                ('QuotaPeakPagedPoolUsage', ctypes.c_size_t), ('QuotaPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t), ('QuotaNonPagedPoolUsage', ctypes.c_size_t),
                ('PagefileUsage', ctypes.c_size_t), ('PeakPagefileUsage', ctypes.c_size_t), ('PrivateUsage', ctypes.c_size_t)]


k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.restype = ctypes.c_void_p
k32.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CloseHandle.argtypes = [ctypes.c_void_p]
psapi.GetProcessMemoryInfo.argtypes = [ctypes.c_void_p, ctypes.POINTER(PMC), W.DWORD]


def mem(pid):
    h = k32.OpenProcess(0x1000 | 0x0400, False, pid)  # PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_QUERY_INFORMATION
    if not h:
        return None
    c = PMC(); c.cb = ctypes.sizeof(PMC)
    ok = psapi.GetProcessMemoryInfo(h, ctypes.byref(c), c.cb)
    k32.CloseHandle(h)
    return (c.PrivateUsage / 1e6, c.WorkingSetSize / 1e6) if ok else None


rows = {'surface': [], 'renderer': []}
for i in range(pairs):
    for mode in ('surface', 'renderer'):
        d = os.path.join(root, out, '%s%d' % (mode, i)); shutil.rmtree(d, ignore_errors=True); os.makedirs(d + '/user')
        open(d + '/user/registry.ini', 'w').write('; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE' + chr(92) + 'JollyBear' + chr(92) + '%s]\nFullScreen=dword:0\n' % section)
        shot = d + '/first.png'
        open(d + '/script.txt', 'w').write('400 screenshot %s\n2400 quit\n' % shot.replace(chr(92), '/'))
        env = dict(os.environ)
        env.pop('SURFACE_DISPLAY', None)
        if mode == 'renderer':
            env['SURFACE_DISPLAY'] = '0'
        t0 = time.time()
        p = subprocess.Popen([os.path.join(root, 'build/native', game + '.exe'), '-gamedir=' + os.path.join(root, 'assets', game),
                              '-userdir=' + d + '/user', '-script=' + d + '/script.txt', '-windowscale=1'], env=env,
                             stdout=open(d + '/out.txt', 'w'), stderr=subprocess.STDOUT)
        first = None
        last = None
        while p.poll() is None:
            if first is None and os.path.exists(shot):
                first = time.time() - t0
            m = mem(p.pid)
            if m:
                last = m
            time.sleep(0.5)
        if first is None and os.path.exists(shot):
            first = os.path.getmtime(shot) - t0
        rows[mode].append((first if first is not None else -1, last[0] if last else 0, last[1] if last else 0))
        print(mode, i, 'first update at %.2f s, private %.1f MB, working set %.1f MB' % rows[mode][-1], flush=True)
for mode in rows:
    r = rows[mode]
    print('%-8s median: first update %.2f s, private %.1f MB, working set %.1f MB (n=%d)' % (
        mode, statistics.median(x[0] for x in r), statistics.median(x[1] for x in r), statistics.median(x[2] for x in r), len(r)))
