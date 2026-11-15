"""Plays Water Bugs with real input only (CLAUDE.md, "Done means identical"; the native pass of the window): window
messages posted to the game's own window, as a player's mouse and keys give them, and the window itself changed with the
Win32 calls a player's actions make (resize, minimise, restore, Alt+Enter), on the real clock. Run it under rundesk only
(the window, the sound and the priority stay out of the user's way):

    build/native/rundesk.exe python games/waterbugs/tools/realplay/play.py <steps file> <out dir> [--exe EXE]
        [--fullscreen] [--nocheck] [-- game arguments, e.g. -windowscale=1]

The game runs as a test run (-commands, the live channel the driver asks "state" and "pointer" over; the answers are
flushed line by line) with BYOPOP_POSTED_INPUT, so its input is the posted messages, and with SURFACE_VERIFY=1 (every frame of the
window's surface compared with the game's screen, unless --nocheck). The steps file has one step a line ('#' starts a comment):

    wait <seconds>        let the game run
    click <x> <y>         a move there, a left press, 0.1 s, the release (the point in the game's 800x600 pixels, mapped
                          as the window shows the game now: scaled to fit and centred)
    clickp <x> <y>        the same, and the game must then have the mouse at that point (its "pointer" answer, to within
                          a pixel of rounding at each size); not on the board, where the game moves the pointer itself
    probe <name>          input that reaches the game from the window as it is: the board's QUIT button must open its box
                          (the game's picture changes there) and the box's Cancel button close it
    key <vk>              a key press and release (a virtual-key code; 118 is F7, the game's full-screen key)
    altenter              Alt+Enter as Windows posts it
    resize <w> <h>        the window's client area, as dragging its edge sets it (moved to stay on the screen)
    resize! <w> <h>       the same where the window is, even when part of it is then beyond the screen's edge
    move <x> <y>          the window's top left corner on the screen
    min, restore, max     ShowWindow; "min" also posts the focus loss a click on the button gives
    focus lose|gain       the window loses or regains the focus
    state                 the game's state line (score, lives, board, filled cells) now
    beams <n> [watch]     n clicks on the board's water, 0.5 s apart; with "watch" the state must change within 10 s (a
                          beam that reached the game fills cells; later ones may fall on walls already built)
    shot <name>           the game's own screenshot (BYOPOP_SHOT_REQUEST) and PrintWindow of the window, compared: what
                          the window shows fitted into the game's rectangle against the game's picture (mean difference
                          per channel); both PNGs go to <out dir> as <name>.game.png and <name>.win.png
    end                   closes the window and waits for the game to exit

The log is <out dir>/play.log: every step with what it found, then the game's display and SurfaceDisplay lines. The last line
says PASS or FAIL: FAIL when a shot's difference is over MAX_MAD, a window shows nothing, the game exits early, the
verification found a frame that differs or the beams changed nothing."""
import ctypes
import ctypes.wintypes as wt
import os
import re
import subprocess
import sys
import time

from PIL import Image, ImageChops, ImageStat

u32 = ctypes.WinDLL('user32', use_last_error=True)
g32 = ctypes.WinDLL('gdi32')
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u32.ShowWindow.argtypes = [wt.HWND, ctypes.c_int]
u32.IsIconic.argtypes = [wt.HWND]
u32.IsZoomed.argtypes = [wt.HWND]
u32.GetDC.restype = wt.HDC
u32.GetDC.argtypes = [wt.HWND]
u32.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
u32.PrintWindow.argtypes = [wt.HWND, wt.HDC, ctypes.c_uint]
u32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
u32.GetWindowLongW.argtypes = [wt.HWND, ctypes.c_int]
u32.GetWindowLongW.restype = ctypes.c_long
u32.AdjustWindowRectEx.argtypes = [ctypes.POINTER(wt.RECT), wt.DWORD, wt.BOOL, wt.DWORD]
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint]
g32.DeleteObject.argtypes = [wt.HGDIOBJ]
g32.DeleteDC.argtypes = [wt.HDC]

WM_CLOSE, WM_MOUSEMOVE = 0x10, 0x200
WM_LBUTTONDOWN, WM_LBUTTONUP = 0x201, 0x202
WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, WM_SYSKEYUP = 0x100, 0x101, 0x104, 0x105
WM_ACTIVATE, WM_SETFOCUS, WM_KILLFOCUS, WM_ACTIVATEAPP = 0x6, 0x7, 0x8, 0x1c
SW_MINIMIZE, SW_MAXIMIZE, SW_RESTORE = 6, 3, 9
SWP_NOMOVE, SWP_NOZORDER, SWP_NOACTIVATE = 0x2, 0x4, 0x10
GWL_STYLE, GWL_EXSTYLE = -16, -20
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
MAX_MAD = 12.0        # mean difference per channel (0 to 255) over the game's rectangle of the window, the most a shot may differ
BEAM_POINTS = [(400, 300), (200, 200), (600, 400), (120, 500), (700, 150), (400, 100), (250, 350), (550, 250)]


def find_window(pid):
    found = []
    WNDENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)

    def cb(hwnd, _):
        p = wt.DWORD()
        u32.GetWindowThreadProcessId(hwnd, ctypes.byref(p))
        if p.value == pid and u32.IsWindowVisible(hwnd):
            found.append(hwnd)
        return True
    u32.EnumWindows(WNDENUMPROC(cb), 0)
    return found[0] if found else None


class Bi(ctypes.Structure):
    _fields_ = [('s', wt.DWORD), ('w', wt.LONG), ('h', wt.LONG), ('p', wt.WORD), ('b', wt.WORD), ('c', wt.DWORD),
                ('i', wt.DWORD), ('x', wt.LONG), ('y', wt.LONG), ('u', wt.DWORD), ('m', wt.DWORD)]


def main():
    args = sys.argv[1:]
    exe = os.path.join(ROOT, 'build', 'native', 'waterbugs.exe')
    fullscreen, check = False, True
    extra = []
    positional = []
    i = 0
    while i < len(args):
        if args[i] == '--exe':
            exe = os.path.abspath(args[i + 1])
            i += 2
        elif args[i] == '--fullscreen':
            fullscreen = True
            i += 1
        elif args[i] == '--nocheck':
            check = False
            i += 1
        elif args[i] == '--':
            extra = args[i + 1:]
            break
        else:
            positional.append(args[i])
            i += 1
    steps_file, out = positional[0], os.path.abspath(positional[1])
    os.makedirs(out, exist_ok=True)
    log = open(os.path.join(out, 'play.log'), 'w')
    failures = []

    def say(text):
        print(text, file=log, flush=True)

    def fail(text):
        failures.append(text)
        say('FAIL: ' + text)

    user = os.path.join(out, 'user')
    os.makedirs(user, exist_ok=True)
    with open(os.path.join(user, 'cp.cfg'), 'w') as f:
        f.write('<CP_CONFIG\n    fullscreen="%d"\n    desktopResolution="0"\n    wave="0"\n    music="0"\n/>\n' % (1 if fullscreen else 0))
    cmdfile = os.path.join(out, 'cmds.txt')   # the live channel: the driver asks "state" and "pointer", the game answers on its log
    open(cmdfile, 'w').close()
    req = os.path.join(out, 'shot.req')
    cmd = [exe, '-gamedir=' + os.path.join(ROOT, 'assets', 'waterbugs'), '-userdir=' + user, '-commands=' + cmdfile,
           '-modsdir=' + os.path.join(ROOT, 'build', 'native', 'mods-waterbugs')] + extra
    env = dict(os.environ, BYOPOP_SHOT_REQUEST=req, BYOPOP_POSTED_INPUT='1')
    if check:
        env['SURFACE_VERIFY'] = '1'
    stdout = os.path.join(out, 'stdout.txt')
    proc = subprocess.Popen(cmd, env=env, stdout=open(stdout, 'w'), stderr=subprocess.STDOUT)  # (SDL_Log goes to stderr)
    hwnd = None
    for _ in range(300):
        hwnd = find_window(proc.pid)
        if hwnd or proc.poll() is not None:
            break
        time.sleep(0.1)
    if not hwnd:
        say('no window; exit %s' % proc.poll())
        proc.kill()
        return 1
    src = open(stdout)
    buf = ['']
    states = []   # (time, score, lives, board, filled)
    pointers = []  # (time, x, y, in): where the mouse is as the game's widgets last saw it
    clicks_seen = [0]  # the clicks the game's pointer answer put where they were aimed
    probes_ok = [0]    # the probes that passed

    def read_lines():
        chunk = src.read()
        if not chunk:
            return []
        buf[0] += chunk
        *done, buf[0] = buf[0].split('\n')
        return done

    def poll_states():
        for ln in read_lines():
            m = re.search(r'POINTER [{]"x":(-?\d+),"y":(-?\d+),"in":(true|false)[}]', ln)
            if m:
                pointers.append((time.time(), int(m.group(1)), int(m.group(2)), m.group(3) == 'true'))
            m = re.search(r'state: score (-?\d+), lives (-?\d+), board (-?\d+), filled (-?\d+) of (-?\d+)%', ln)
            if m:
                states.append((time.time(),) + tuple(int(g) for g in m.groups()[:4]))

    def ask(command, answers, timeout=3.0):
        """send a live command and wait for its answer line (the next one of that kind on the log)"""
        poll_states()
        n = len(answers)
        with open(cmdfile, 'a') as f:
            f.write(command + '\n')
        t0 = time.time()
        while time.time() - t0 < timeout and proc.poll() is None:
            poll_states()
            if len(answers) > n:
                return answers[-1]
            time.sleep(0.02)
        return None

    def latest_state(timeout=3.0):
        return ask('state', states, timeout)

    def client():
        r = wt.RECT()
        u32.GetClientRect(hwnd, ctypes.byref(r))
        return r.right - r.left, r.bottom - r.top

    def fit():
        """the game's rectangle in the client area: scaled to fit and centred (x, y, w, h)"""
        w, h = client()
        k = min(w / 800.0, h / 600.0)
        return (w - 800 * k) / 2, (h - 600 * k) / 2, 800 * k, 600 * k

    def point(x, y):
        fx, fy, fw, fh = fit()
        cx, cy = round(fx + x * fw / 800.0), round(fy + y * fh / 600.0)
        return (cy << 16) | (cx & 0xffff)

    def click_checked(x, y):
        """click, and ask where the game's widgets then have the mouse: it must be the point the click was aimed at"""
        click(x, y)
        time.sleep(0.25)
        answer = ask('pointer', pointers, 3.0)
        if answer is None:
            fail('click %d,%d: the game gave no pointer answer' % (x, y))
            return
        _, px, py, inside = answer
        fx, fy, fw, fh = fit()
        tol = 1.5 + 800.0 / fw
        if abs(px - x) > tol or abs(py - y) > tol or not inside:
            fail('click %d,%d [client %dx%d]: the game saw the mouse at %d,%d (in: %s)' % (x, y, *client(), px, py, inside))
        else:
            clicks_seen[0] += 1

    def click(x, y, hold=0.1):
        # the engine reads the button as it is at each update (Sys_MouseButton), so unlike the framework's other games a press
        # must last past an update (10 ms): the move and the press, 0.1 s, then the release. (BYOPOP_POSTED_INPUT keeps the
        # window's own leave and cursor events from moving the mouse meanwhile.)
        p = point(x, y)
        u32.PostMessageW(hwnd, WM_MOUSEMOVE, 0, p)
        u32.PostMessageW(hwnd, WM_LBUTTONDOWN, 1, p)
        time.sleep(hold)
        u32.PostMessageW(hwnd, WM_MOUSEMOVE, 1, p)
        u32.PostMessageW(hwnd, WM_LBUTTONUP, 0, p)

    def key(vk):
        sc = u32.MapVirtualKeyW(vk, 0) << 16  # SDL takes the key from the scan code in lParam
        u32.PostMessageW(hwnd, WM_KEYDOWN, vk, sc | 1)
        u32.PostMessageW(hwnd, WM_KEYUP, vk, sc | 0xc0000001)

    def alt_enter():
        alt = (0x38 << 16) | 1 | (1 << 29)
        ret = (0x1c << 16) | 1 | (1 << 29)
        u32.PostMessageW(hwnd, WM_SYSKEYDOWN, 0x12, alt)
        time.sleep(0.05)
        u32.PostMessageW(hwnd, WM_SYSKEYDOWN, 0x0d, ret)
        time.sleep(0.05)
        u32.PostMessageW(hwnd, WM_SYSKEYUP, 0x0d, ret | 0xc0000000)
        u32.PostMessageW(hwnd, WM_KEYUP, 0x12, (0x38 << 16) | 1 | 0xc0000000)

    def resize(w, h, keep_place=False):
        r = wt.RECT(0, 0, w, h)
        style = u32.GetWindowLongW(hwnd, GWL_STYLE) & 0xffffffff
        ex = u32.GetWindowLongW(hwnd, GWL_EXSTYLE) & 0xffffffff
        u32.AdjustWindowRectEx(ctypes.byref(r), style, False, ex)
        ow, oh = r.right - r.left, r.bottom - r.top
        if keep_place:
            u32.SetWindowPos(hwnd, 0, 0, 0, ow, oh, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)
            return
        # a player drags an edge until it meets the screen's; Windows draws nothing of a window beyond the screen's edge, so
        # the window is kept inside it (see "resize!")
        cur = wt.RECT()
        u32.GetWindowRect(hwnd, ctypes.byref(cur))
        sw, sh = u32.GetSystemMetrics(0), u32.GetSystemMetrics(1)
        x = max(0, min(cur.left, sw - ow))
        y = max(0, min(cur.top, sh - oh))
        u32.SetWindowPos(hwnd, 0, x, y, ow, oh, SWP_NOZORDER | SWP_NOACTIVATE)

    def move_window(x, y):
        u32.SetWindowPos(hwnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | 0x1)  # SWP_NOSIZE

    def focus(on):
        u32.PostMessageW(hwnd, WM_ACTIVATEAPP, 1 if on else 0, 0)
        u32.PostMessageW(hwnd, WM_ACTIVATE, 1 if on else 0, 0)
        u32.PostMessageW(hwnd, WM_SETFOCUS if on else WM_KILLFOCUS, 0, 0)

    def capture():
        w, h = client()
        if w <= 0 or h <= 0:
            return None
        dc = u32.GetDC(hwnd)
        mdc = g32.CreateCompatibleDC(dc)
        bm = g32.CreateCompatibleBitmap(dc, w, h)
        g32.SelectObject(mdc, bm)
        u32.PrintWindow(hwnd, mdc, 3)
        bi = Bi(40, w, -h, 1, 32, 0, 0, 0, 0, 0, 0)
        pixels = ctypes.create_string_buffer(w * h * 4)
        g32.GetDIBits(mdc, bm, 0, h, pixels, ctypes.byref(bi), 0)
        g32.DeleteObject(bm)
        g32.DeleteDC(mdc)
        u32.ReleaseDC(hwnd, dc)
        return Image.frombuffer('RGBA', (w, h), pixels, 'raw', 'BGRA', 0, 1).convert('RGB')

    def shot(name):
        """the game's own picture (a PIL image) after comparing it with the window's, or None"""
        if u32.IsIconic(hwnd):
            say('shot %s: the window is minimised, nothing to compare' % name)
            return None
        game_png = os.path.join(out, name + '.game.png')
        if os.path.exists(game_png):
            os.remove(game_png)
        with open(req, 'w') as f:
            f.write(game_png)
        t0 = time.time()
        while os.path.exists(req) and time.time() - t0 < 5 and proc.poll() is None:
            time.sleep(0.01)
        win = capture()
        while not os.path.exists(game_png) and time.time() - t0 < 8 and proc.poll() is None:
            time.sleep(0.05)
        time.sleep(0.3)
        if win is None or not os.path.exists(game_png):
            fail('shot %s: no picture (game: %s, window: %s)' % (name, os.path.exists(game_png), win is not None))
            return None
        win.save(os.path.join(out, name + '.win.png'))
        fx, fy, fw, fh = fit()
        box = (round(fx), round(fy), round(fx + fw), round(fy + fh))
        part = win.crop(box).resize((800, 600), Image.BILINEAR)
        game = Image.open(game_png).convert('RGB')
        diff = ImageChops.difference(part, game)
        mad = sum(ImageStat.Stat(diff).mean) / 3.0
        lit = ImageStat.Stat(win.convert('L')).mean[0]
        w, h = client()
        say('shot %s: window %dx%d, the game at %d,%d %dx%d, mean difference %.2f, window mean brightness %.1f' % (
            name, w, h, box[0], box[1], box[2] - box[0], box[3] - box[1], mad, lit))
        if lit < 1.0:
            fail('shot %s: the window shows nothing' % name)
        elif mad > MAX_MAD:
            fail('shot %s: the window differs from the game by %.2f (over %.1f)' % (name, mad, MAX_MAD))
        return game

    DIALOG = (190, 220, 610, 380)   # where the board's "Do you really want to quit?" box is, in game pixels

    def region_change(a, b):
        return sum(ImageStat.Stat(ImageChops.difference(a.crop(DIALOG), b.crop(DIALOG))).mean) / 3.0

    def probe(name):
        """input that reaches the game from the window in its state now: the board's QUIT button opens a box (the game's
        picture changes where it is), and its Cancel button closes it"""
        a = shot(name + '_a')
        click(555, 590)
        time.sleep(0.8)
        b = shot(name + '_box')
        click(460, 330)
        time.sleep(0.8)
        c = shot(name + '_c')
        if a is None or b is None or c is None:
            fail('probe %s: no pictures to compare' % name)
            return
        opened, closed = region_change(a, b), region_change(a, c)
        say('probe %s: the box changed its area by %.1f, and after Cancel by %.1f' % (name, opened, closed))
        if opened < 25:
            fail('probe %s: the QUIT click did not open the box (change %.1f)' % (name, opened))
        elif closed > 12:
            fail('probe %s: the Cancel click did not close the box (change %.1f)' % (name, closed))
        else:
            probes_ok[0] += 1
        for suffix in ('_a', '_box', '_c'):   # (the pictures of a probe that passed are not kept)
            if not failures:
                for kind in ('.game.png', '.win.png'):
                    try:
                        os.remove(os.path.join(out, name + suffix + kind))
                    except OSError:
                        pass

    def show_state(why):
        s = latest_state()
        say('%s: state %s' % (why, 'none yet' if s is None else 'score %d, lives %d, board %d, filled %d' % s[1:]))
        return s

    for n, line in enumerate(open(steps_file)):
        line = line.split('#')[0].strip()
        if not line:
            continue
        if proc.poll() is not None:
            fail('the game exited (%d) before line %d: %s' % (proc.returncode, n + 1, line))
            break
        c, v = line.split()[0], line.split()[1:]
        say('%.2f %s   [client %dx%d]' % (time.time(), line, *client()))
        if c == 'wait':
            time.sleep(float(v[0]))
        elif c == 'click':
            click(float(v[0]), float(v[1]))
            time.sleep(0.3)
        elif c == 'clickp':
            click_checked(float(v[0]), float(v[1]))
        elif c == 'probe':
            probe(v[0])
        elif c == 'key':
            key(int(v[0]))
            time.sleep(0.5)
        elif c == 'altenter':
            alt_enter()
            time.sleep(1.0)
        elif c == 'resize':
            resize(int(v[0]), int(v[1]))
            time.sleep(0.8)
        elif c == 'resize!':
            resize(int(v[0]), int(v[1]), True)   # where it is, even if that puts part of it beyond the screen's edge
            time.sleep(0.8)
        elif c == 'move':
            move_window(int(v[0]), int(v[1]))
            time.sleep(0.8)
        elif c == 'min':
            focus(False)
            u32.ShowWindow(hwnd, SW_MINIMIZE)
            time.sleep(0.8)
        elif c == 'max':
            u32.ShowWindow(hwnd, SW_MAXIMIZE)
            time.sleep(0.8)
        elif c == 'restore':
            u32.ShowWindow(hwnd, SW_RESTORE)
            focus(True)
            time.sleep(0.8)
        elif c == 'focus':
            focus(v[0] == 'gain')
            time.sleep(0.3)
        elif c == 'state':
            show_state('state')
        elif c == 'beams':
            before = show_state('beams before')
            for k in range(int(v[0])):
                click(*BEAM_POINTS[k % len(BEAM_POINTS)])
                time.sleep(0.5)
            if len(v) > 1 and v[1] == 'watch':
                # the first board: the beams must reach the game, and it takes a few seconds for a beam to do something
                t0 = time.time()
                after = before
                while time.time() - t0 < 10 and (after is None or before is None or before[1:] == after[1:]):
                    time.sleep(0.5)
                    after = latest_state(1.5)
                say('beams after %.1f s: state %s' % (time.time() - t0, 'none' if after is None else
                    'score %d, lives %d, board %d, filled %d' % after[1:]))
                if before is None or after is None or before[1:] == after[1:]:
                    fail('%s beams changed nothing in the state in 10 s' % v[0])
        elif c == 'shot':
            shot(v[0])
        elif c == 'end':
            u32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
            try:
                proc.wait(timeout=20)
            except subprocess.TimeoutExpired:
                fail('the game did not exit after the window was closed')
            break
        else:
            fail('unknown step: ' + line)
    if proc.poll() is None:
        u32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
        try:
            proc.wait(timeout=20)
        except subprocess.TimeoutExpired:
            proc.kill()
            fail('the game had to be killed')
    time.sleep(0.2)
    poll_states()
    say('--- the game\'s display lines')
    bad_verify = False
    for ln in open(stdout, errors='replace'):
        if 'display:' in ln or 'SurfaceDisplay' in ln:
            say(ln.rstrip())
            if 'not the screen at' in ln or ('compared with the screen' in ln and 'none differing' not in ln):
                bad_verify = True
    if bad_verify:
        fail('SurfaceDisplay found a frame that differs from the screen')
    say('clicks the game saw where they were aimed: %d; probes passed: %d' % (clicks_seen[0], probes_ok[0]))
    say('exit code %s' % proc.returncode)
    say('PASS' if not failures else 'FAIL: %d: %s' % (len(failures), '; '.join(failures)))
    return 0 if not failures else 1


sys.exit(main())
