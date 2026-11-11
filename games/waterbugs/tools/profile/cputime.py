"""The processor time of one real-time scripted run of the port, for the performance pass (GAME_NOTES.md, "Performance").

From the worktree root, under rundesk (the game then has a private desktop, silent audio and below-normal priority):

    build/native/rundesk.exe <python.exe> games/waterbugs/tools/profile/cputime.py <exe> <script> [more game args]

Starts <exe> with the script (a list of "<update> <command>" lines, ended by quit; an update is 10 ms of game time, and the
run is in real time), waits for it to end and prints one JSON line: the process's kernel and user time added (every thread),
its wall time, and the peak working set and private bytes the system recorded. Alternate two exes in turn (pairs.sh) so
the machine's own changes hit both.
"""
import ctypes
import ctypes.wintypes as W
import json
import os
import subprocess
import sys
import time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)


class FILETIME(ctypes.Structure):
    _fields_ = [('lo', W.DWORD), ('hi', W.DWORD)]

    def seconds(self):
        return ((self.hi << 32) | self.lo) / 1e7


class COUNTERS(ctypes.Structure):
    _fields_ = [('cb', W.DWORD), ('PageFaultCount', W.DWORD), ('PeakWorkingSetSize', ctypes.c_size_t),
                ('WorkingSetSize', ctypes.c_size_t), ('QuotaPeakPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPagedPoolUsage', ctypes.c_size_t), ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t),
                ('QuotaNonPagedPoolUsage', ctypes.c_size_t), ('PagefileUsage', ctypes.c_size_t),
                ('PeakPagefileUsage', ctypes.c_size_t), ('PrivateUsage', ctypes.c_size_t)]


def main():
    exe, script = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
    root = os.getcwd()
    user = os.path.join(root, 'build', 'perf2', 'user_cpu')
    os.makedirs(user, exist_ok=True)
    with open(os.path.join(user, 'cp.cfg'), 'w') as f:
        f.write('<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n')
    args = [exe, '-gamedir=' + os.path.join(root, 'assets', 'waterbugs'), '-userdir=' + user, '-script=' + script] + sys.argv[3:]
    start = time.time()
    p = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    p.wait()
    wall = time.time() - start
    handle = ctypes.c_void_p(int(p._handle))
    created, exited, kernel, usr = FILETIME(), FILETIME(), FILETIME(), FILETIME()
    k32.GetProcessTimes(handle, ctypes.byref(created), ctypes.byref(exited), ctypes.byref(kernel), ctypes.byref(usr))
    counters = COUNTERS()
    counters.cb = ctypes.sizeof(counters)
    k32.K32GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb)
    print(json.dumps({'exe': os.path.basename(exe), 'cpu': round(kernel.seconds() + usr.seconds(), 2), 'wall': round(wall, 2),
                      'peakWorkingMB': round(counters.PeakWorkingSetSize / 1048576, 1),
                      'peakPrivateMB': round(counters.PeakPagefileUsage / 1048576, 1), 'exit': p.returncode}))


main()
