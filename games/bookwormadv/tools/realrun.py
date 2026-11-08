"""Plays the port with real input: Windows messages posted to its window, as a player's mouse and keys reach it, with
no -script (a -script run drops real input and real focus changes, SexyAppBase::HandleEvent). Run it under rundesk,
so the window lives on rundesk's private desktop:

    build/native/rundesk.exe python games/bookwormadv/tools/realrun.py <steps> <out dir> -- <exe> [game args...]

The steps run on the wall clock, one per line ('#' starts a comment):

    wait S              seconds
    move X Y            WM_MOUSEMOVE at the client point
    click X Y           a move, then a move with the press and a move with the release (bursts: see the code)
    key VK              a key's down and up (a virtual-key code, decimal)
    text ABC            WM_CHAR for each character
    focus lose|regain   WM_ACTIVATEAPP, WM_ACTIVATE and WM_KILLFOCUS / WM_SETFOCUS, as visrun posts them
    minimize | restore  ShowWindow: the window really minimizes (SDL's MINIMIZED and focus events), then comes back
    shot NAME           the window's own picture (PrintWindow) into <out dir>/NAME.png, the game's 800x600

Positions are the game's 800x600: in full screen they are scaled to the client area as the game is (fitted, centred).
    quit                WM_CLOSE, then up to 30 s for the game to end

The game's sound log (BYOPOP_SOUNDLOG, set by the caller) shows what was heard. Every step is logged with its time in
<out dir>/realrun.log.
"""
import ctypes
import ctypes.wintypes as wt
import os
import subprocess
import sys
import threading
import time

from PIL import Image

u32 = ctypes.WinDLL('user32', use_last_error=True)
g32 = ctypes.WinDLL('gdi32', use_last_error=True)
u32.GetDC.restype = wt.HDC
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.SelectObject.restype = wt.HGDIOBJ
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p,
                          ctypes.c_uint]
g32.DeleteObject.argtypes = [wt.HGDIOBJ]
g32.DeleteDC.argtypes = [wt.HDC]
u32.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u32.PrintWindow.argtypes = [wt.HWND, wt.HDC, ctypes.c_uint]
u32.ShowWindow.argtypes = [wt.HWND, ctypes.c_int]


def find_window(pid):
    found = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(hw, lp):
        p = wt.DWORD()
        u32.GetWindowThreadProcessId(hw, ctypes.byref(p))
        r = wt.RECT()
        u32.GetClientRect(hw, ctypes.byref(r))
        if p.value == pid and u32.IsWindowVisible(hw) and r.right >= 800 and r.bottom >= 600:
            found.append(hw)
        return True
    u32.EnumWindows(cb, 0)
    return found[0] if found else None


def grab(hw):
    """The window's client area as the window itself draws it (PrintWindow, PW_RENDERFULLCONTENT), from a helper
    thread, since PrintWindow waits for the game's thread. None if it fails or takes over 5 s."""
    r = wt.RECT()
    u32.GetWindowRect(hw, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    pt = wt.POINT(0, 0)
    u32.ClientToScreen(hw, ctypes.byref(pt))
    sdc = u32.GetDC(None)
    mdc = g32.CreateCompatibleDC(sdc)
    bmp = g32.CreateCompatibleBitmap(sdc, w, h)
    u32.ReleaseDC(None, sdc)
    old = g32.SelectObject(mdc, bmp)
    done = []
    t = threading.Thread(target=lambda: done.append(u32.PrintWindow(hw, mdc, 2)), daemon=True)
    t.start()
    t.join(5)
    if t.is_alive() or not done or not done[0]:
        return None
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
    cw, ch, k, gx, gy = geometry(hw)
    ox, oy = pt.x - r.left + gx, pt.y - r.top + gy
    im = Image.frombuffer('RGBA', (w, h), buf, 'raw', 'BGRA', 0, 1).convert('RGB')
    im = im.crop((int(round(ox)), int(round(oy)), int(round(ox + 800 * k)), int(round(oy + 600 * k))))
    return im if im.size == (800, 600) else im.resize((800, 600), Image.LANCZOS)


def geometry(hw):
    """The client area's size, and where the game's 800x600 lies in it: full screen scales it to fit, centred."""
    r = wt.RECT()
    u32.GetClientRect(hw, ctypes.byref(r))
    cw, ch = r.right, r.bottom
    k = min(cw / 800.0, ch / 600.0)
    return cw, ch, k, (cw - 800 * k) / 2, (ch - 600 * k) / 2


def xy(hw, x, y):
    cw, ch, k, ox, oy = geometry(hw)
    x, y = int(round(ox + x * k)), int(round(oy + y * k))
    return (y << 16) | (x & 0xffff)


def main():
    if '--' not in sys.argv:
        raise SystemExit(__doc__)
    i = sys.argv.index('--')
    steps, out = sys.argv[1:i]
    cmd = sys.argv[i + 1:]
    os.makedirs(out, exist_ok=True)
    log = open(os.path.join(out, 'realrun.log'), 'w')
    game_log = open(os.path.join(out, 'game.log'), 'w')
    p = subprocess.Popen(cmd, stdout=game_log, stderr=subprocess.STDOUT)
    t0 = time.time()

    def note(s):
        print('%8.2f %s' % (time.time() - t0, s), file=log, flush=True)

    hw = None
    while hw is None and time.time() - t0 < 120 and p.poll() is None:
        time.sleep(0.2)
        hw = find_window(p.pid)
    if hw is None:
        note('no window')
        p.kill()
        raise SystemExit('realrun: no window')
    note('window %#x, client %dx%d' % ((hw,) + geometry(hw)[:2]))
    # a player's window is the active one: on rundesk's desktop it can be made so (Seven Seas' realinput.py)
    k32 = ctypes.WinDLL('kernel32')
    tid, me = u32.GetWindowThreadProcessId(hw, None), k32.GetCurrentThreadId()
    u32.AttachThreadInput(me, tid, True)
    u32.SetForegroundWindow(hw)
    u32.SetActiveWindow(hw)
    u32.SetFocus(hw)
    u32.AttachThreadInput(me, tid, False)
    note('foreground: %s' % (u32.GetForegroundWindow() == hw))
    for line in open(steps):
        line = line.split('#')[0].strip()
        if not line:
            continue
        if p.poll() is not None:
            note('the game ended (%d) before: %s' % (p.returncode, line))
            break
        op, args = line.split()[0], line.split()[1:]
        if op == 'wait':
            time.sleep(float(args[0]))
            continue
        note(line)
        if op == 'move':
            u32.PostMessageW(hw, 0x0200, 0, xy(hw, int(args[0]), int(args[1])))
        elif op == 'click':
            # SDL 2 reads Windows' cursor, which sits in a corner of rundesk's desktop and sends moves of its own: a
            # move posted just before the press and the release, in one burst, keeps them where they are meant
            lp = xy(hw, int(args[0]), int(args[1]))
            u32.PostMessageW(hw, 0x0200, 0, lp)
            time.sleep(0.12)
            u32.PostMessageW(hw, 0x0200, 0, lp)
            u32.PostMessageW(hw, 0x0201, 1, lp)
            time.sleep(0.06)
            u32.PostMessageW(hw, 0x0200, 1, lp)
            u32.PostMessageW(hw, 0x0202, 0, lp)
        elif op == 'key':
            vk = int(args[0])
            sc = u32.MapVirtualKeyW(vk, 0)   # SDL takes the key from the scan code in lParam
            ext = 1 if vk in (0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2D, 0x2E) else 0
            u32.PostMessageW(hw, 0x0100, vk, 1 | (sc << 16) | (ext << 24))
            time.sleep(0.06)
            u32.PostMessageW(hw, 0x0101, vk, 1 | (sc << 16) | (ext << 24) | 0xC0000000)
        elif op == 'text':
            for ch in ' '.join(args):
                u32.PostMessageW(hw, 0x0102, ord(ch), 1)
                time.sleep(0.05)
        elif op == 'focus':
            if args[0] == 'regain':
                u32.PostMessageW(hw, 0x001C, 1, 0)
                u32.PostMessageW(hw, 0x0006, 1, 0)
                u32.PostMessageW(hw, 0x0007, 0, 0)
            else:
                u32.PostMessageW(hw, 0x001C, 0, 0)
                u32.PostMessageW(hw, 0x0006, 0, 0)
                u32.PostMessageW(hw, 0x0008, 0, 0)
        elif op == 'minimize':
            u32.ShowWindow(hw, 6)    # SW_MINIMIZE
        elif op == 'restore':
            u32.ShowWindow(hw, 9)    # SW_RESTORE
        elif op == 'shot':
            im = grab(hw)
            if im is None:
                note('shot %s failed' % args[0])
            else:
                im.save(os.path.join(out, args[0] + '.png'))
        elif op == 'quit':
            u32.PostMessageW(hw, 0x0010, 0, 0)
            try:
                p.wait(30)
            except subprocess.TimeoutExpired:
                note('no end after WM_CLOSE: killed')
                p.kill()
            break
        else:
            note('unknown step: ' + line)
    if p.poll() is None:
        p.kill()
    note('end (%s)' % p.returncode)


if __name__ == '__main__':
    main()
