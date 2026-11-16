"""Wait for the machine to be quiet before a timing run: python -I quiet.py <max busy> [<timeout s>]

Samples the whole machine's processor use (GetSystemTimes) over 3 s, again and again, and exits 0 when the busy share
of all logical processors is at or under <max busy> (0.5 is half of the machine), or 1 when the timeout (default 3,600 s)
passes first. softpairs.sh calls it before each run: the number of game processes on the machine says little about the
load (most of them idle, a few busy), and the cycles of a pinned run grow with the load of the hyperthread beside it.
"""
import ctypes
import ctypes.wintypes as W
import sys
import time

k32 = ctypes.WinDLL('kernel32', use_last_error=True)


def times():
    idle, kernel, user = W.FILETIME(), W.FILETIME(), W.FILETIME()
    k32.GetSystemTimes(ctypes.byref(idle), ctypes.byref(kernel), ctypes.byref(user))
    f = lambda t: (t.dwHighDateTime << 32) | t.dwLowDateTime
    return f(idle), f(kernel) + f(user)    # kernel time includes the idle time


def busy(seconds=3.0):
    i0, t0 = times()
    time.sleep(seconds)
    i1, t1 = times()
    return 1.0 - (i1 - i0) / max(1, t1 - t0)


if __name__ == '__main__':
    limit = float(sys.argv[1]) if len(sys.argv) > 1 else 0.5
    timeout = float(sys.argv[2]) if len(sys.argv) > 2 else 3600.0
    start = time.time()
    while True:
        b = busy()
        if b <= limit:
            print('quiet: %.2f' % b)
            sys.exit(0)
        if time.time() - start >= timeout:
            print('busy: %.2f' % b)
            sys.exit(1)
        time.sleep(5)
