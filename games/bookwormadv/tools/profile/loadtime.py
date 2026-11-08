# Seconds from the port's process creation to its title screen with the loading over (the state command's "title"), several
# runs, natively in real time. Run under rundesk from the worktree root:
#   python games/bookwormadv/tools/profile/loadtime.py <out-dir> <runs> <a user folder with a player>
# A state line goes onto the live channel every 20 ms; the game answers it at its next update (10 ms), so a time is good to
# about 30 ms. The original has no such command: perfmin.py's thread count gives its loading thread's end.
import ctypes, ctypes.wintypes as W, json, os, shutil, subprocess, sys, time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)


def created(h):
    a, b, c, d = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(h, ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(d))
    return ((a.dwHighDateTime << 32) | a.dwLowDateTime) / 1e7


def now():
    t = W.FILETIME()
    k32.GetSystemTimePreciseAsFileTime(ctypes.byref(t))
    return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7


out, runs, profile = os.path.abspath(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
exe = os.path.join(os.getcwd(), 'build', 'native', 'bookwormadv.exe')
res = []
for i in range(runs):
    d = os.path.join(out, 'run%d' % i)
    shutil.rmtree(d, ignore_errors=True)
    shutil.copytree(profile, os.path.join(d, 'user'))
    cf, log = os.path.join(d, 'commands.txt'), os.path.join(d, 'game.log')
    open(cf, 'w').close()
    proc = subprocess.Popen([exe, '-userdir=' + os.path.join(d, 'user'), '-commands=' + cf], stdout=open(log, 'w'),
                            stderr=subprocess.STDOUT)
    h = k32.OpenProcess(0x1000, False, proc.pid)
    t0, found = created(h), None
    while found is None and now() - t0 < 60:
        with open(cf, 'a') as f:
            f.write('state\n')
        time.sleep(0.02)
        with open(log, errors='replace') as f:
            for line in f:
                if '"screen":"title"' in line and '"loaded":true' in line:
                    found = now() - t0
                    break
    with open(cf, 'a') as f:
        f.write('quit\n')
    proc.wait(30)
    res.append(round(found, 3) if found is not None else None)
    print('run %d: %s s' % (i, res[-1]), flush=True)
with open(os.path.join(out, 'loadtime.json'), 'w') as f:
    json.dump(res, f)
