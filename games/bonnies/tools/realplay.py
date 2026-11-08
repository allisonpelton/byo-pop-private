"""Plays the port with real input only: posted window messages, as a player's mouse and keyboard send them, never -script
(which drops real input) or autoplay. Run it under rundesk, which puts it and the game on a private desktop:

    build/native/rundesk.exe python games/bonnies/tools/realplay.py <steps> <out dir> -- <bonnies.exe> <args...>

The steps file has one command a line ('#' starts a comment); x and y are the game's 800x600 coordinates:
    wait <ms>            move <x> <y>           click <x> <y>        (a move, 150 ms, then down and up)
    drag <x1> <y1> <x2> <y2>                     type <text>          (WM_KEYDOWN, WM_CHAR, WM_KEYUP for each character)
    key <name>           (RETURN, ESCAPE, BACK, SPACE, TAB, F4...)  shot <name> (PrintWindow of the game's window)
    focus lose|regain    (WM_ACTIVATEAPP and WM_KILLFOCUS / WM_SETFOCUS, as switching to another window sends them)
    close                (WM_CLOSE: the window's close box)
    fullscreen           (Alt+Enter, posted)     quit-when-gone <ms>  (waits up to ms for the game to end by itself)
The window's client area is mapped to the game's 800x600 whatever its size (windowed or full screen).
"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import time

from PIL import Image

u32 = ctypes.WinDLL('user32', use_last_error=True)
g32 = ctypes.WinDLL('gdi32', use_last_error=True)
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u32.PrintWindow.argtypes = [wt.HWND, wt.HDC, ctypes.c_uint]
u32.GetDC.restype = wt.HDC
u32.GetDC.argtypes = [wt.HWND]
u32.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.SelectObject.restype = wt.HGDIOBJ
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint]
g32.DeleteObject.argtypes = [wt.HGDIOBJ]
g32.DeleteDC.argtypes = [wt.HDC]

WM_ACTIVATE, WM_SETFOCUS, WM_KILLFOCUS, WM_ACTIVATEAPP = 0x0006, 0x0007, 0x0008, 0x001C
WM_KEYDOWN, WM_KEYUP, WM_CHAR, WM_SYSKEYDOWN, WM_SYSKEYUP = 0x0100, 0x0101, 0x0102, 0x0104, 0x0105
WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP = 0x0200, 0x0201, 0x0202
VK = {'RETURN': 0x0D, 'ESCAPE': 0x1B, 'BACK': 0x08, 'SPACE': 0x20, 'TAB': 0x09, 'LEFT': 0x25, 'UP': 0x26,
      'RIGHT': 0x27, 'DOWN': 0x28, 'F4': 0x73, 'DELETE': 0x2E, 'MENU': 0x12}


def find_window(pid, timeout):
    found = []
    end = time.time() + timeout
    proto = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)

    def cb(hwnd, _):
        p = wt.DWORD()
        u32.GetWindowThreadProcessId(hwnd, ctypes.byref(p))
        if p.value == pid and u32.IsWindowVisible(hwnd) and client_size(hwnd)[0] > 0:
            found.append(hwnd)
            if VERBOSE:
                c = ctypes.create_unicode_buffer(256)
                u32.GetClassNameW(wt.HWND(hwnd), c, 256)
                print('realplay: candidate', hwnd, c.value, client_size(hwnd))
        return True
    while time.time() < end:
        found.clear()
        u32.EnumWindows(proto(cb), 0)
        if found:
            return found[0]
        time.sleep(0.2)
    return None


def client_size(hwnd):
    r = wt.RECT()
    u32.GetClientRect(hwnd, ctypes.byref(r))
    return r.right, r.bottom


def lparam_at(hwnd, x, y):
    w, h = (800, 600) if os.environ.get('REALPLAY_RAW') == '1' else client_size(hwnd)
    cx, cy = int(x * w / 800), int(y * h / 600)
    return (cy << 16) | (cx & 0xFFFF)


def shot(hwnd, path):
    w, h = client_size(hwnd)
    hdc = u32.GetDC(hwnd)
    mdc = g32.CreateCompatibleDC(hdc)
    bmp = g32.CreateCompatibleBitmap(hdc, w, h)
    old = g32.SelectObject(mdc, bmp)
    u32.PrintWindow(hwnd, mdc, 3)  # PW_CLIENTONLY | PW_RENDERFULLCONTENT
    class BMI(ctypes.Structure):
        _fields_ = [('biSize', wt.DWORD), ('biWidth', ctypes.c_long), ('biHeight', ctypes.c_long),
                    ('biPlanes', wt.WORD), ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                    ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', ctypes.c_long), ('biYPelsPerMeter', ctypes.c_long),
                    ('biClrUsed', wt.DWORD), ('biClrImportant', wt.DWORD), ('pad', wt.DWORD * 4)]
    bmi = BMI()
    bmi.biSize, bmi.biWidth, bmi.biHeight, bmi.biPlanes, bmi.biBitCount = 40, w, -h, 1, 32
    buf = ctypes.create_string_buffer(w * h * 4)
    g32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bmi), 0)
    g32.SelectObject(mdc, old)
    g32.DeleteObject(bmp)
    g32.DeleteDC(mdc)
    u32.ReleaseDC(hwnd, hdc)
    Image.frombuffer('RGBA', (w, h), buf, 'raw', 'BGRA', 0, 1).convert('RGB').save(path)


def main():
    try:
        return run()
    finally:
        for p in PROCS:
            if p.poll() is None:
                p.kill()


PROCS = []
VERBOSE = os.environ.get('REALPLAY_VERBOSE') == '1'


def run():
    # the game is not DPI-aware: its posted coordinates and client size are its own (800x600 windowed), not the
    # screen's pixels, so the driver sees the window as the game does
    u32.SetThreadDpiAwarenessContext.restype = ctypes.c_void_p
    u32.SetThreadDpiAwarenessContext(ctypes.c_void_p(-1))  # DPI_AWARENESS_CONTEXT_UNAWARE
    sep = sys.argv.index('--')
    steps, out = sys.argv[1], sys.argv[2]
    cmd = sys.argv[sep + 1:]
    os.makedirs(out, exist_ok=True)
    u32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
    u32.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]
    u32.IsWindowVisible.argtypes = [wt.HWND]
    proc = subprocess.Popen(cmd)
    PROCS.append(proc)
    hwnd = find_window(proc.pid, 30)
    if not hwnd:
        print('realplay: no window')
        proc.kill()
        return 1
    cls = ctypes.create_unicode_buffer(256)
    u32.GetClassNameW(wt.HWND(hwnd), cls, 256)
    print('realplay: window', hwnd, cls.value, client_size(hwnd))
    post = lambda m, w, l: u32.PostMessageW(hwnd, m, w, l)
    for line in open(steps, encoding='utf-8'):
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        if proc.poll() is not None:
            print('realplay: the game ended before', line)
            break
        hwnd = find_window(proc.pid, 5) or hwnd
        a = line.split()
        c = a[0]
        if c == 'wait':
            time.sleep(int(a[1]) / 1000)
        elif c == 'move':
            post(WM_MOUSEMOVE, 0, lparam_at(hwnd, int(a[1]), int(a[2])))
        elif c == 'click':
            l = lparam_at(hwnd, int(a[1]), int(a[2]))
            post(WM_MOUSEMOVE, 0, l)
            time.sleep(0.15)
            post(WM_LBUTTONDOWN, 1, l)
            time.sleep(0.08)
            post(WM_LBUTTONUP, 0, l)
        elif c == 'drag':
            x1, y1, x2, y2 = map(int, a[1:5])
            post(WM_MOUSEMOVE, 0, lparam_at(hwnd, x1, y1))
            time.sleep(0.15)
            post(WM_LBUTTONDOWN, 1, lparam_at(hwnd, x1, y1))
            for i in range(1, 11):
                time.sleep(0.03)
                post(WM_MOUSEMOVE, 1, lparam_at(hwnd, x1 + (x2 - x1) * i // 10, y1 + (y2 - y1) * i // 10))
            time.sleep(0.08)
            post(WM_LBUTTONUP, 0, lparam_at(hwnd, x2, y2))
        elif c == 'type':
            for ch in line.split(None, 1)[1]:
                vk = u32.VkKeyScanW(ord(ch)) & 0xFF
                post(WM_KEYDOWN, vk, 1)
                post(WM_CHAR, ord(ch), 1)
                post(WM_KEYUP, vk, 0xC0000001)
                time.sleep(0.06)
        elif c == 'key':
            vk = VK[a[1]]
            post(WM_KEYDOWN, vk, 1)
            if vk in (0x0D, 0x1B, 0x08, 0x20, 0x09):
                post(WM_CHAR, vk, 1)
            post(WM_KEYUP, vk, 0xC0000001)
        elif c == 'close':
            post(0x0010, 0, 0)  # WM_CLOSE, as the window's close box sends it
        elif c == 'fullscreen':
            post(WM_SYSKEYDOWN, VK['RETURN'], 0x20000001)
            post(WM_SYSKEYUP, VK['RETURN'], 0xE0000001)
        elif c == 'focus':
            if a[1] == 'lose':
                post(WM_ACTIVATEAPP, 0, 0)
                post(WM_ACTIVATE, 0, 0)
                post(WM_KILLFOCUS, 0, 0)
            else:
                post(WM_ACTIVATEAPP, 1, 0)
                post(WM_ACTIVATE, 1, 0)
                post(WM_SETFOCUS, 0, 0)
        elif c == 'shot':
            time.sleep(0.1)
            try:
                shot(hwnd, os.path.join(out, a[1] + '.png'))
            except Exception as e:
                print('realplay: shot', a[1], 'failed:', e)
        elif c == 'quit-when-gone':
            try:
                proc.wait(int(a[1]) / 1000)
            except subprocess.TimeoutExpired:
                pass
        else:
            print('realplay: unknown step', line)
    if proc.poll() is None:
        print('realplay: the game is still running at the end: stopped')
        proc.kill()
        return 1
    print('realplay: the game ended by itself, exit', proc.returncode)
    return 0


if __name__ == '__main__':
    sys.exit(main())
