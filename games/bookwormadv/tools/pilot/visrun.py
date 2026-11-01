"""Runs one of the pilot's original scripts (scripts/<steps>.orig.txt, made by gen.py) with the original on the user's
visible desktop, for what Windows Sandbox cannot give: the original's 3D mode (docs/ORIGRUN.md, "Hardware
acceleration"). The user allowed this on 2026-11-01: the game on their desktop with sound, and shots of its own window.

    python visrun.py <exe> <script> <shots dir> [--3d on|off]

The copy must lie on a short path: the game builds paths in a 256-byte buffer (0x6cbb60) and dies at start under a
worktree's long path. Before the run the game's registry key (HKCU\\Software\\PopCap\\WinBAD) and its users folder are
backed up into <shots dir>\\..\\backup, the pilot's start state goes in (build/scratch/pilot/start.reg, Is3D from
--3d, the pilot's Tester.bwa), and afterwards both are put back.

The script's commands are done as origrun does them: "at N <cmd>" waits for the game's update counter (app+0x484) to
reach N, holds the game (its threads suspended), posts the input (move, click, focus lose/regain) and lets it go, so
the input is read in update N+1. A shot waits until the game has drawn its last update (mHasPendingDraw, app+0x45a, is
clear), holds it, and copies the window's client area from the screen. The window is kept topmost, and a shot is
refused while any other visible window overlaps it, so nothing but the game's own pixels is ever copied. Each shot is
written as origrun's are, the client area at (3, 26) of an 806x629 picture, and listed in landed.tsv with its update.
"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import shutil
import subprocess
import sys
import threading
import time
import winreg

from PIL import Image

ROOT = os.path.dirname(os.path.abspath(__file__))
APP, COUNTER, PENDING, FLAG3D = 0x8259e4, 0x484, 0x45a, (0x36c, 0x38)
KEY = r'Software\PopCap\WinBAD'
USERS = r'C:\ProgramData\PopCap Games\WinBAD\users'

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
u32 = ctypes.WinDLL('user32', use_last_error=True)
g32 = ctypes.WinDLL('gdi32', use_last_error=True)
ntdll = ctypes.WinDLL('ntdll')
k32.OpenProcess.restype = wt.HANDLE
u32.GetDC.restype = wt.HDC
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.SelectObject.restype = wt.HGDIOBJ
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.BitBlt.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, wt.HDC, ctypes.c_int,
                       ctypes.c_int, wt.DWORD]
g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p,
                          ctypes.c_uint]
g32.DeleteObject.argtypes = [wt.HGDIOBJ]
g32.DeleteDC.argtypes = [wt.HDC]
u32.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
u32.GetWindow.restype = wt.HWND
u32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]


class Game:
    def __init__(self, exe):
        self.p = subprocess.Popen([exe], cwd=os.path.dirname(exe))
        self.h = k32.OpenProcess(0x0010 | 0x0400 | 0x0800, False, self.p.pid)   # VM_READ, QUERY_INFO, SUSPEND_RESUME
        self.hwnd = None

    def rd(self, addr, size=4):
        v = ctypes.c_uint32()
        n = ctypes.c_size_t()
        if not addr or not k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), ctypes.byref(v), size, ctypes.byref(n)):
            return None
        return v.value & (0xff if size == 1 else 0xffffffff)

    def app(self):
        return self.rd(APP)

    def counter(self):
        a = self.app()
        return self.rd(a + COUNTER) if a else None

    def pending(self):
        a = self.app()
        return self.rd(a + PENDING, 1) if a else None

    def flag3d(self):
        a = self.app()
        dd = self.rd(a + FLAG3D[0]) if a else None
        return self.rd(dd + FLAG3D[1], 1) if dd else None

    def hold(self):
        ntdll.NtSuspendProcess(self.h)

    def release(self):
        ntdll.NtResumeProcess(self.h)

    def window(self):
        if self.hwnd and u32.IsWindow(self.hwnd):
            return self.hwnd
        found = []

        @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
        def cb(hw, lp):
            pid = wt.DWORD()
            u32.GetWindowThreadProcessId(hw, ctypes.byref(pid))
            r = wt.RECT()
            u32.GetClientRect(hw, ctypes.byref(r))
            if pid.value == self.p.pid and u32.IsWindowVisible(hw) and r.right >= 800 and r.bottom >= 600:
                found.append(hw)
            return True
        u32.EnumWindows(cb, 0)
        self.hwnd = found[0] if found else None
        if self.hwnd:
            # topmost, not activated: nothing of the user's can come over it while it is captured
            u32.SetWindowPos(self.hwnd, wt.HWND(-1), 0, 0, 0, 0, 0x0001 | 0x0002 | 0x0010)
        return self.hwnd


def watch(g, path):
    """origrun's --watch lines for loads.py, as run.sh asks them: the chapter loading screen's timer
    ([[app+0x83c]+0xb0]), the startup load thread's start (app+0x4f9) and its end (app+0x919)."""
    def vals():
        a = g.app()
        if not a:
            return ['-'] * 3
        s = g.rd(a + 0x83c)
        t = g.rd(s + 0xb0) if s else None
        out = [str(t) if t is not None else '-']
        for off in (0x4f9, 0x919):
            v = g.rd(a + off)
            out.append(str(v) if v is not None else '-')
        return out
    with open(path, 'w') as f:
        last = ['?'] * 3
        while g.p.poll() is None:
            v, c = vals(), g.counter()
            for i in range(3):
                if v[i] != last[i]:
                    f.write('watch %d: %s -> %s%s\n' % (i + 1, last[i], v[i], ' at update %d' % c if c else ''))
                    last[i] = v[i]
            f.flush()
            time.sleep(0.0005)


def client_rect(hw):
    r = wt.RECT()
    u32.GetClientRect(hw, ctypes.byref(r))
    pt = wt.POINT(0, 0)
    u32.ClientToScreen(hw, ctypes.byref(pt))
    return pt.x, pt.y, r.right, r.bottom


def overlapped(hw):
    """A visible window above the game's in the z-order that covers part of its client area, or None."""
    x, y, w, h = client_rect(hw)
    o = u32.GetWindow(hw, 3)   # GW_HWNDPREV
    while o:
        if u32.IsWindowVisible(o) and not u32.IsIconic(o):
            r = wt.RECT()
            u32.GetWindowRect(o, ctypes.byref(r))
            if r.left < x + w and r.right > x and r.top < y + h and r.bottom > y and r.right > r.left and r.bottom > r.top:
                buf = ctypes.create_unicode_buffer(256)
                u32.GetClassNameW(o, buf, 256)
                return buf.value
        o = u32.GetWindow(o, 3)
    return None


def grab(hw):
    x, y, w, h = client_rect(hw)
    sdc = u32.GetDC(None)
    mdc = g32.CreateCompatibleDC(sdc)
    bmp = g32.CreateCompatibleBitmap(sdc, w, h)
    old = g32.SelectObject(mdc, bmp)
    g32.BitBlt(mdc, 0, 0, w, h, sdc, x, y, 0x00CC0020)
    buf = ctypes.create_string_buffer(w * h * 4)

    class BMI(ctypes.Structure):
        _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG), ('biPlanes', wt.WORD),
                    ('biBitCount', wt.WORD), ('biCompression', wt.DWORD), ('biSizeImage', wt.DWORD),
                    ('biXPelsPerMeter', wt.LONG), ('biYPelsPerMeter', wt.LONG), ('biClrUsed', wt.DWORD),
                    ('biClrImportant', wt.DWORD), ('pad', wt.DWORD * 4)]
    bmi = BMI()
    bmi.biSize = 40
    bmi.biWidth, bmi.biHeight, bmi.biPlanes, bmi.biBitCount = w, -h, 1, 32
    g32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bmi), 0)
    g32.SelectObject(mdc, old)
    g32.DeleteObject(bmp)
    g32.DeleteDC(mdc)
    u32.ReleaseDC(None, sdc)
    im = Image.frombuffer('RGBA', (w, h), buf, 'raw', 'BGRA', 0, 1).convert('RGB').crop((0, 0, 800, 600))
    canvas = Image.new('RGB', (806, 629))
    canvas.paste(im, (3, 26))
    return canvas


def xy(x, y):
    return (y << 16) | (x & 0xffff)


def post_input(g, cmd, args):
    hw = g.window()
    if cmd == 'move':
        u32.PostMessageW(hw, 0x0200, 0, xy(int(args[0]), int(args[1])))
    elif cmd == 'click':
        lp = xy(int(args[0]), int(args[1]))
        u32.PostMessageW(hw, 0x0200, 0, lp)
        u32.PostMessageW(hw, 0x0201, 1, lp)
        u32.PostMessageW(hw, 0x0202, 0, lp)
    elif cmd == 'focus':
        if args[0] == 'regain':
            u32.PostMessageW(hw, 0x001C, 1, 0)
            u32.PostMessageW(hw, 0x0006, 1, 0)
            u32.PostMessageW(hw, 0x0007, 0, 0)
        else:
            u32.PostMessageW(hw, 0x001C, 0, 0)
            u32.PostMessageW(hw, 0x0006, 0, 0)
            u32.PostMessageW(hw, 0x0008, 0, 0)
    else:
        raise SystemExit('visrun: cannot do %s' % cmd)


def wait_counter(g, n, log, timeout=60):
    t0 = time.time()
    while time.time() - t0 < timeout:
        c = g.counter()
        if c is not None and c >= n:
            return c
        if g.p.poll() is not None:
            return None
        time.sleep(0.001)
    print('counter %d not reached in %d s' % (n, timeout), file=log, flush=True)
    return None


def shot(g, out, name, landed, log):
    hw = g.window()
    o = overlapped(hw)
    if o:
        print('shot %s refused: a window (%s) overlaps the game' % (name, o), file=log, flush=True)
        return
    for _ in range(200):
        if g.pending() == 0:
            g.hold()
            if g.pending() == 0:
                break
            g.release()
        time.sleep(0.001)
    else:
        print('shot %s refused: no drawn update in 200 ms' % name, file=log, flush=True)
        return
    try:
        c = g.counter()
        im = grab(hw)
    finally:
        g.release()
    path = os.path.join(out, name + '.png')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im.save(path)
    landed.write('%s\t%s\tdrawn\n' % (name, c))
    landed.flush()


def stage(backup, threed):
    if os.path.exists(backup):
        raise SystemExit('visrun: %s exists: an earlier run was not restored' % backup)
    os.makedirs(backup)
    subprocess.run(['reg', 'export', 'HKCU\\' + KEY, os.path.join(backup, 'winbad.reg'), '/y'], check=True,
                   capture_output=True)
    shutil.copytree(USERS, os.path.join(backup, 'users'))
    subprocess.run(['reg', 'delete', 'HKCU\\' + KEY, '/f'], capture_output=True)
    subprocess.run(['reg', 'import', os.path.join(ROOT, '..', '..', '..', '..', 'build', 'scratch', 'pilot',
                                                    'start.reg')], check=True, capture_output=True)
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, KEY, 0, winreg.KEY_SET_VALUE) as k:
        winreg.SetValueEx(k, 'Is3D', 0, winreg.REG_DWORD, 1 if threed else 0)
    pilot = os.path.join(ROOT, '..', '..', '..', '..', 'build', 'scratch', 'pilot')
    shutil.copy(os.path.join(pilot, 'portuser0', 'users', 'Tester.bwa'), os.path.join(USERS, 'Tester.bwa'))


def restore(backup):
    subprocess.run(['reg', 'delete', 'HKCU\\' + KEY, '/f'], capture_output=True)
    subprocess.run(['reg', 'import', os.path.join(backup, 'winbad.reg')], check=True, capture_output=True)
    shutil.rmtree(USERS)
    shutil.copytree(os.path.join(backup, 'users'), USERS)
    shutil.rmtree(backup)


def main():
    a = [x for x in sys.argv[1:]]
    threed = True
    if '--3d' in a:
        i = a.index('--3d')
        threed = a[i + 1] == 'on'
        del a[i:i + 2]
    exe, script, out = a
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    backup = os.path.join(os.path.dirname(os.path.abspath(out)), 'visrun-backup')
    log = open(os.path.join(out, '..', os.path.basename(out) + '.visrun.log'), 'w')
    landed = open(os.path.join(out, 'landed.tsv'), 'w')
    stage(backup, threed)
    g = None
    try:
        g = Game(exe)
        flags = set()
        for line in open(script):
            w = line.split()
            if not w:
                continue
            if w[0] == 'waitshot':
                t0 = time.time()
                while not g.window() and time.time() - t0 < 60:
                    time.sleep(0.05)
                # watched from the window on: before the app is built its fields hold garbage
                threading.Thread(target=watch, args=(g, os.path.join(out, '..', os.path.basename(out) + '.watch.log')),
                                 daemon=True).start()
                continue
            if w[0] == 'quit':
                break
            if w[0] != 'at':
                print('skipped: %s' % line.strip(), file=log)
                continue
            n, cmd, args = int(w[1]), w[2], w[3:]
            if wait_counter(g, n, log) is None:
                break
            # the real focus is the user's to move: a step where the game is not in front is logged, since the
            # game hears of it (WM_ACTIVATEAPP) and the run no longer matches the sandbox's
            if u32.GetForegroundWindow() != g.window():
                print('update %d: the game is not the foreground window' % n, file=log, flush=True)
            f = g.flag3d()
            if f not in flags:
                flags.add(f)
                print('update %d: 3D flag %s' % (n, f), file=log, flush=True)
            if cmd == 'shot':
                shot(g, out, args[0], landed, log)
            elif cmd == 'burst':
                count, step = int(args[1]), int(args[3])
                for k in range(count):
                    if k and wait_counter(g, n + k * step, log) is None:
                        break
                    shot(g, out, '%s/%04d' % (args[0], k), landed, log)
                json.dump({'interval_ms': step * 10}, open(os.path.join(out, args[0], 'clip.json'), 'w'))
            else:
                g.hold()
                try:
                    post_input(g, cmd, args)
                finally:
                    g.release()
        print('end: update %s, 3D flags seen %s' % (g.counter(), sorted(flags, key=str)), file=log, flush=True)
    finally:
        if g and g.p.poll() is None:
            hw = g.window()
            if hw:
                u32.PostMessageW(hw, 0x0010, 0, 0)
            try:
                g.p.wait(20)
            except subprocess.TimeoutExpired:
                g.p.kill()
                g.p.wait(10)
                print('killed', file=log)
        restore(backup)
        print('restored the registry key and the users folder', file=log, flush=True)


if __name__ == '__main__':
    main()
