# perf.py <port|orig> <out dir> <script> [seconds]: loading time, memory and processor time of one run of Rocket Mania in real time.
# Run from the worktree root. The port runs under rundesk with -commands (to see the play link come up) and the script as
# -script; the original under origrun (real clock, scripted cursor; origrun puts it on a private desktop itself). The game
# process is found by its command line and sampled every 0.25 s: working set, private bytes and processor time (kernel + user),
# until it exits. Loading time: seconds from the process's creation to the first screen that takes input, the play link: for the
# port the first `state` that says "link":true, for the original the update at which the loaded flag (app+0x45a) turned on, read
# from origrun's watch of the dword at 0x4f68b8, with its log time. Prints one RESULT line and writes samples.csv.
import ctypes, ctypes.wintypes as W, json, os, re, subprocess, sys, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
psapi = ctypes.WinDLL('psapi', use_last_error=True)


class PMC(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD)] + [(n, ctypes.c_size_t) for n in (
        'PeakWorkingSetSize', 'WorkingSetSize', 'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage',
        'QuotaPeakNonPagedPoolUsage', 'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage', 'PrivateUsage')]


k32.OpenProcess.restype = W.HANDLE
side, out, script = sys.argv[1], os.path.abspath(sys.argv[2]), os.path.abspath(sys.argv[3])
os.makedirs(out, exist_ok=True)
exe_name = 'rocketmania.exe' if side == 'port' else 'RocketManiaGame.exe'
log = open(os.path.join(out, 'run.log'), 'wb')
cmdfile = os.path.join(out, 'commands.txt')
if side == 'port':
    open(cmdfile, 'w').close(); os.remove(cmdfile)
    user = os.path.join(out, 'user')
    if os.environ.get('FRESH'):
        subprocess.run(['cmd', '/c', 'rmdir', '/s', '/q', user], capture_output=True)
    cmd = [os.path.abspath('build/native/rundesk.exe'), os.path.abspath('build/native/rocketmania.exe'), '-script=' + script,
           '-commands=' + cmdfile, '-userdir=' + user]
else:
    stage = os.environ.get('OG_STAGE', 'C:/byo-og/rm')
    cmd = [os.path.abspath('build/origrun-bin/origrun.exe'), '--exe', os.path.abspath(stage + '/game/RocketManiaGame.exe').replace('/', '\\'),
           '--run', os.path.basename(stage), '--stage-root', os.path.dirname(stage).replace('/', '\\'), '--script', script, '--clock', 'real',
           '--cursor', 'scripted', '--counter', '0x4f6850', '--update-ms', '10', '--seed', '1', '--patches',
           os.path.abspath('tools/origrun/patches/rocketmania.txt'), '--watch', '0x4f68b8', '--timeout', '300']
t0 = time.time()
p = subprocess.Popen(cmd, stdout=log, stderr=subprocess.STDOUT)


def find_pid():
    r = subprocess.run(['powershell', '-NoProfile', '-Command',
                        "Get-CimInstance Win32_Process -Filter \"Name='%s'\" | ForEach-Object { \"$($_.ProcessId)|$($_.CommandLine)\" }" % exe_name],
                       capture_output=True, text=True).stdout.strip().splitlines()
    for l in r:
        pid, _, c = l.partition('|')
        if side == 'port' and os.path.basename(out) in c.replace('/', '\\').replace('\\', '/'):
            return int(pid)
        if side == 'orig' and 'RocketManiaGame' in c:
            return int(pid)
    return None


def cpu(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    f = lambda t: ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7
    return f(c) + f(d), a


pid = None
while pid is None and time.time() - t0 < 120:
    pid = find_pid(); time.sleep(0.2)
if pid is None:
    print('RESULT FAILED: no game process'); sys.exit(1)
h = k32.OpenProcess(0x100000 | 0x1000 | 0x0400, False, pid)   # query limited + query information
create = W.FILETIME(); a, b, c = W.FILETIME(), W.FILETIME(), W.FILETIME()
k32.GetProcessTimes(h, ctypes.byref(create), ctypes.byref(a), ctypes.byref(b), ctypes.byref(c))
ct = ((create.dwHighDateTime << 32) | create.dwLowDateTime) / 1e7 - 11644473600
rows, link_t, peak_ws, peak_priv, last = [], None, 0, 0, None
n = 0
while True:
    if k32.WaitForSingleObject(h, 0) == 0:
        break
    m = PMC(); m.cb = ctypes.sizeof(PMC)
    psapi.GetProcessMemoryInfo(h, ctypes.byref(m), m.cb)
    cp, _ = cpu(h)
    now = time.time()
    rows.append((now - ct, m.WorkingSetSize, m.PrivateUsage, cp))
    peak_ws = max(peak_ws, m.WorkingSetSize); peak_priv = max(peak_priv, m.PrivateUsage)
    n += 1
    if side == 'port' and link_t is None and n % 2 == 0:
        with open(cmdfile, 'a') as f:
            f.write('state\n')
        time.sleep(0.05)
        txt = open(os.path.join(out, 'run.log'), 'rb').read().decode('latin-1')
        if '"link":true' in txt:
            link_t = time.time() - ct
    time.sleep(0.25)
end_cpu = rows[-1][3] if rows else 0
if side == 'orig':
    txt = open(os.path.join(out, 'run.log'), 'rb').read().decode('latin-1')
    mm = re.search(r'\[(\d\d):(\d\d):(\d\d)\][^\n]*watch 1: 1 -> 65793 at update (\d+)', txt) or re.search(r'\[(\d\d):(\d\d):(\d\d)\][^\n]*watch 1: \d+ -> 65793', txt)
    if mm:
        lt = time.mktime(time.localtime(ct)); lt = time.strftime('%Y-%m-%d', time.localtime(ct))
        t = time.mktime(time.strptime(lt + ' %s:%s:%s' % mm.groups()[:3], '%Y-%m-%d %H:%M:%S'))
        link_t = t - ct
with open(os.path.join(out, 'samples.csv'), 'w') as f:
    f.write('t,ws,private,cpu\n')
    for r in rows:
        f.write('%.2f,%d,%d,%.2f\n' % r)
life = rows[-1][0] if rows else 0
print('RESULT %s: load-to-link %s s, peak working set %.1f MB, peak private %.1f MB, cpu %.1f s over %.1f s of life (%.0f%%)' % (
    side, ('%.2f' % link_t) if link_t is not None else 'not measured', peak_ws / 1048576.0, peak_priv / 1048576.0, end_cpu, life,
    100.0 * end_cpu / life if life else 0))
