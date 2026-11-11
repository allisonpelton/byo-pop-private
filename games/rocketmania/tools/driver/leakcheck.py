# leakcheck.py [--cycles N] [--updates U] [--exe PATH]: looks for memory the port keeps after a game: starts a game of each mode
# in turn (the live channel's `goto`), lets it run U updates, leaves it (`goto menu`) and reads the process's private bytes after
# every cycle. A line that grows by the same amount every cycle is a leak; the RESULT line gives the growth per cycle of the
# last two thirds of the run (the first cycles load the pictures). The port runs under rundesk with -fast and -hold.
import argparse, os, re, subprocess, sys, time, ctypes
from ctypes import wintypes

ap = argparse.ArgumentParser()
ap.add_argument('--cycles', type=int, default=30)
ap.add_argument('--updates', type=int, default=300)
ap.add_argument('--exe', default='build/native/rocketmania.exe')
ap.add_argument('--out', default='build/ph2/leak')
args = ap.parse_args()

out = os.path.abspath(args.out)
os.makedirs(out, exist_ok=True)
log_path = os.path.join(out, 'log.txt')
cmd_path = os.path.join(out, 'commands.txt')
user = os.path.join(out, 'user')
subprocess.run(['cmd', '/c', 'rmdir', '/s', '/q', user], capture_output=True)
open(cmd_path, 'w').close()
os.remove(cmd_path)
empty = os.path.join(out, 'empty.txt')
open(empty, 'w').close()
open(log_path, 'w').close()
logf = open(log_path, 'wb')
rundesk = os.path.abspath('build/native/rundesk.exe')
proc = subprocess.Popen([rundesk, os.path.abspath(args.exe), '-commands=' + cmd_path, '-realinput=' + empty, '-fast', '-seed=1', '-userdir=' + user],
                        stdout=logf, stderr=subprocess.STDOUT)


class PMC(ctypes.Structure):
    _fields_ = [('cb', wintypes.DWORD), ('PageFaultCount', wintypes.DWORD), ('PeakWorkingSetSize', ctypes.c_size_t),
                ('WorkingSetSize', ctypes.c_size_t), ('QuotaPeakPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPagedPoolUsage', ctypes.c_size_t), ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t),
                ('QuotaNonPagedPoolUsage', ctypes.c_size_t), ('PagefileUsage', ctypes.c_size_t),
                ('PeakPagefileUsage', ctypes.c_size_t), ('PrivateUsage', ctypes.c_size_t)]


def game_pid():
    r = subprocess.run(['powershell', '-NoProfile', '-Command',
                        "(Get-CimInstance Win32_Process | Where-Object { $_.Name -eq '%s' -and $_.CommandLine -like '*%s*' }).ProcessId"
                        % (os.path.basename(args.exe), os.path.basename(out))], capture_output=True, text=True)
    ids = [int(x) for x in r.stdout.split()]
    return ids[0] if ids else None


def private_bytes(pid):
    h = ctypes.windll.kernel32.OpenProcess(0x1000 | 0x0400, False, pid)   # QUERY_LIMITED_INFORMATION, QUERY_INFORMATION
    pmc = PMC()
    pmc.cb = ctypes.sizeof(pmc)
    ok = ctypes.windll.psapi.GetProcessMemoryInfo(h, ctypes.byref(pmc), pmc.cb)
    ctypes.windll.kernel32.CloseHandle(h)
    return pmc.PrivateUsage if ok else None


def log_text():
    with open(log_path, 'rb') as f:
        return f.read().decode('latin-1')


def send(line, pattern, wait=60.0):
    pos = len(log_text())
    with open(cmd_path, 'a') as f:
        f.write(line + '\n')
    end = time.time() + wait
    while time.time() < end:
        if proc.poll() is not None:
            raise RuntimeError('the game ended')
        for l in log_text()[pos:].split('\n'):
            if re.search(pattern, l):
                return l.strip()
        time.sleep(0.02)
    raise RuntimeError('no answer to "%s"' % line)


def ok(line):
    name = line.split()[0]
    a = send(line, r'(OK|ERROR) ' + re.escape(name) + r'\b')
    if 'ERROR' in a:
        raise RuntimeError(a)


try:
    send('state', r'^STATE \{', 120)
    pid = None
    for _ in range(50):
        pid = game_pid()
        if pid:
            break
        time.sleep(0.2)
    # the loading screen's play link needs one click before `goto` works
    while True:
        l = send('state', r'^STATE \{')
        if '"screen":"loading"' not in l:
            break
        ok('click game 320 451')
        time.sleep(0.5)
    sizes = []
    ROTATION = ('arcade', 'classic', 'strategy', 'options', 'highscores', 'credits', 'lost')
    for c in range(args.cycles):
        mode = ROTATION[c % len(ROTATION)]
        ok('goto %s 2' % ('arcade' if mode == 'lost' else mode) if mode in ('arcade', 'classic', 'strategy', 'lost') else 'goto %s' % mode)
        time.sleep(args.updates * 0.004)
        if mode == 'lost':   # the game over flow: the level-up screen, buy rockets, the high-score name box
            ok('lose')
            time.sleep(args.updates * 0.004)
        ok('goto menu')
        time.sleep(0.3)
        b = private_bytes(pid)
        sizes.append(b)
        print('cycle %2d %-8s private bytes %d' % (c + 1, mode, b), flush=True)
    k = len(sizes) // 3
    tail = sizes[k:]
    per = (tail[-1] - tail[0]) / max(1, len(tail) - 1)
    print('RESULT: private bytes %d -> %d over %d cycles; the last two thirds grew %.0f bytes a cycle' % (sizes[0], sizes[-1], len(sizes), per))
except Exception as e:
    print('RESULT FAILED:', e)
finally:
    try:
        with open(cmd_path, 'a') as f:
            f.write('quit\n')
        proc.wait(timeout=20)
    except Exception:
        proc.kill()
