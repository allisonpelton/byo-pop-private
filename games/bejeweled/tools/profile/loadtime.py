# Seconds from the port's process creation to its first screen that takes input (the state command's "title": the loading
# screen is over and the play link answers), several runs. Run under rundesk from the worktree root:
#   python loadtime.py <out-dir> <runs> [-fast]
# The state command is asked every few milliseconds through the live channel (the game reads it once an update, 10 ms), so a
# time is good to about 10 ms. The original has no such command: perfmin.py's thread count gives its loading thread's end.
import ctypes, ctypes.wintypes as W, json, os, shutil, sys, time
sys.argv, args = sys.argv[:1], sys.argv[1:]
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'realplay'))
import drive

k32 = ctypes.WinDLL('kernel32', use_last_error=True)


def ft(t):
    return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7


out = os.path.abspath(args[0])
runs = int(args[1])
root = os.getcwd()
exe = os.path.join(root, 'build', 'native', 'bejeweled.exe')
gamedir = os.path.join(root, 'assets', 'bejeweled')
res = []
for i in range(runs):
    d = os.path.join(out, 'run%d' % i)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(os.path.join(d, 'user'))
    with open(os.path.join(d, 'user', 'registry.ini'), 'w') as f:
        f.write('[SOFTWARE\Steam\Bejeweled]\nScreenMode=dword:0\n')
    dr = drive.Driver(exe, gamedir, os.path.join(d, 'user'), d, '-fast' in args, ['-seed=3'])
    a, b, c, e = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(int(dr.proc._handle), ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(e))
    created = ft(a)
    t = None
    while t is None:
        s = dr.send('state')
        if s.startswith('STATE ') and json.loads(s[6:]).get('screen') == 'title':
            n = W.FILETIME()
            k32.GetSystemTimeAsFileTime(ctypes.byref(n))
            t = ft(n) - created
    dr.send('quit')
    dr.proc.wait(timeout=20)
    res.append(round(t, 3))
    print('run', i, t)
print('loading seconds:', res, 'median', sorted(res)[len(res) // 2])
