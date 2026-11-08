# Seconds from the port's process creation to its first screen that takes input: a click on the title's Play link is sent
# through the live channel every 50 ms until the state says the menu is up. Run under rundesk from the worktree root:
#   python loadtime.py <out dir> <runs>
import ctypes, ctypes.wintypes as W, json, os, shutil, subprocess, sys, threading, time
k32 = ctypes.WinDLL('kernel32', use_last_error=True)
def ft(t): return ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7
out, runs, root = os.path.abspath(sys.argv[1]), int(sys.argv[2]), os.getcwd()
exe = os.path.join(root, 'build', 'native', 'sevenseas.exe')
res = []
for i in range(runs):
    d = os.path.join(out, 'run%d' % i); shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    cmds = os.path.join(d, 'cmds.txt'); open(cmds, 'w').close()
    p = subprocess.Popen([exe, '-userdir=' + os.path.join(d, 'u'), '-commands=' + cmds], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    a, b, c, e = W.FILETIME(), W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetProcessTimes(int(p._handle), ctypes.byref(a), ctypes.byref(b), ctypes.byref(c), ctypes.byref(e))
    created, found = ft(a), []
    def reader():
        for line in p.stdout:
            if line.startswith('STATE ') and '"screen":"menu"' in line:
                n = W.FILETIME(); k32.GetSystemTimeAsFileTime(ctypes.byref(n)); found.append(ft(n) - created); return
    th = threading.Thread(target=reader, daemon=True); th.start()
    f = open(cmds, 'a')
    while not found and time.time() - created < 1e12:
        f.write('click 320 385\nstate\n'); f.flush(); time.sleep(0.05)
        if p.poll() is not None: break
    f.write('quit\n'); f.close()
    try: p.wait(timeout=20)
    except Exception: p.kill()
    res.append(round(found[0], 3) if found else None); print('run', i, res[-1])
print('seconds to the menu:', res)
