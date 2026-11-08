"""Plays the port with real input only: posted window messages, as a player's mouse and keyboard send them, never -script
(which drops real input) or autoplay. Run it under rundesk, which puts it and the game on a private desktop:

    build/native/rundesk.exe python games/bonnies/tools/realplay.py <steps> <out dir> -- <bonnies.exe> <args...>

The steps file has one command a line ('#' starts a comment); x and y are the game's 800x600 coordinates:
    wait <ms>            move <x> <y>           click <x> <y>        (a move, 150 ms, then a move, down and up together)
    drag <x1> <y1> <x2> <y2>                     type <text>          (WM_KEYDOWN and WM_KEYUP for each character)
    key <name>           (RETURN, ESCAPE, BACK, SPACE, TAB, F4...)  shot <name> (PrintWindow of the game's window)
    focus lose|regain    (WM_ACTIVATEAPP and WM_KILLFOCUS / WM_SETFOCUS, as switching to another window sends them)
    close                (WM_CLOSE: the window's close box)
    minimize, restore    (WM_SYSCOMMAND, as the title bar's and the taskbar's buttons send it)
    state <name>         (the window's client size and the game's state file, kept as <name>.txt)
    fullscreen           (Alt+Enter, posted)     quit-when-gone <ms>  (waits up to ms for the game to end by itself)
    play <s>             (plays the classic game on screen for up to s seconds, as a player who takes the game's own
                         hints: see play() below)
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


# The words of buttons a player never presses while playing on: leaving the game, the web, and the options
DENY = ('quit', 'menu', 'more', 'buy', 'web', 'exit', 'cancel', 'register', 'option', 'help', 'back', 'high', 'score',
        'credit', 'no')


def read_state(path):
    """The port's test state (game/GameScreen.cpp, testState), positions in the game's 800x600; None while absent"""
    try:
        with open(path, encoding='latin-1') as f:
            text = f.read()
    except OSError:
        return None
    st = {'modal': 0, 'buttons': [], 'game': None, 'balloon': None, 'hint': (0, []), 'says': '', 'marked': []}
    for line in text.splitlines():
        a = line.split(' ')
        if a[0] == 'modal':
            st['modal'] = int(a[1])
        elif a[0] == 'button':
            st['buttons'].append((int(a[1]), int(a[2]), float(a[3]) * 800, float(a[4]) * 600, ' '.join(a[5:])))
        elif a[0] == 'game':
            st['game'] = [int(v) for v in a[1:]]
        elif a[0] == 'balloon':
            st['balloon'] = [int(v) for v in a[1:]]
        elif a[0] == 'says':
            st['says'] = ' '.join(a[1:])
        elif a[0] == 'marked':
            st['marked'] = [(a[1 + 5 * i], int(a[2 + 5 * i]), int(a[3 + 5 * i]), float(a[4 + 5 * i]) * 800,
                             float(a[5 + 5 * i]) * 600) for i in range((len(a) - 1) // 5)]
        elif a[0] == 'hint':
            n = int(a[2])
            st['hint'] = (int(a[1]), [(a[3 + 3 * i], float(a[4 + 3 * i]) * 800, float(a[5 + 3 * i]) * 600)
                                      for i in range(n)])
    return st


def allowed(text):
    t = text.lower()
    return t.strip() != '' and not any(w in t.split() or (len(w) > 3 and w in t) for w in DENY)


def neighbours(c1, r1, c2, r2):
    """Whether two cells touch: odd columns sit half a cell lower (game/Game.h)"""
    if c1 == c2:
        return abs(r1 - r2) == 1
    if abs(c1 - c2) != 1:
        return False
    low = r1 if c1 % 2 else r2   # the row of the cell in the odd column
    high = r2 if c1 % 2 else r1
    return low == high or low == high - 1


def tutorial_path(says, marked):
    """The marked tiles in the order of the word the tutorial's balloon names in capitals (FROG), or None"""
    words = [w.strip('.,!?"\'') for w in says.split()]
    words = [w for w in words if len(w) >= 3 and w.isalpha() and w.isupper()]
    if not words or not marked:
        return None
    word = words[0]

    def walk(path):
        if len(path) == len(word):
            return path
        for m in marked:
            if m in path or m[0] != word[len(path)]:
                continue
            if path and not neighbours(path[-1][1], path[-1][2], m[1], m[2]):
                continue
            found = walk(path + [m])
            if found:
                return found
        return None
    return walk([])


def play(seconds, click, key, shoot, proc, state_path, log):
    """A player who takes the game's own hints: when the board is in play and the game shows its hint, clicks the
    hint's tiles in order and then Submit; answers a dialog with its first button that neither leaves the game nor
    opens anything (DENY); and clicks on through the pages between levels. Ends when the game screen is gone (the
    story's end, or the game lost), the game ends, or the time is up."""
    end = time.time() + seconds
    last, still, level, words, seen = None, time.time(), 0, 0, False
    while time.time() < end and proc.poll() is None:
        st = read_state(state_path)
        if st is None:
            time.sleep(0.2)
            continue
        g = st['game']
        summary = (st['modal'], tuple(g) if g else None, tuple(b[4] for b in st['buttons'] if st['modal'] > 0),
                   tuple(st['balloon']) if st['balloon'] else None)
        if summary != last:
            log('state modal %d game %s balloon %s buttons %s' % (st['modal'], g, st['balloon'],
                [(b[1], b[4]) for b in st['buttons']]))
            last, still = summary, time.time()
        if g is not None and g[0] != level:
            if level:
                shoot('level%02d' % g[0])
            level = g[0]
        if st['modal'] > 0:
            choice = [b for b in st['buttons'] if allowed(b[4])]
            if choice:
                log('dialog: %s' % choice[0][4])
                click(choice[0][2], choice[0][3])
            elif time.time() - still > 3:
                log('dialog without a button to press: RETURN')
                key('RETURN')
                still = time.time()
            time.sleep(1.5)
            continue
        if g is None:
            if seen:
                log('the game screen is gone: the play ends')
                return words
            if time.time() - still > 6:
                # the story's pages before the first board: its next arrow, as the tour clicks it
                log('before the board: a click on the next arrow')
                click(745, 565)
                still = time.time()
            time.sleep(0.3)
            continue
        seen = True
        lvl, score, goal, inprog, phase, busy, c60 = g[:7]
        # a balloon's own button (Continue): any button but the board's Menu, Submit and Scramble
        own = [b for b in st['buttons'] if b[1] not in (0xc9, 0xca, 0xcb) and allowed(b[4])]
        if own:
            log('button: %s' % own[0][4])
            click(own[0][2], own[0][3])
            still = time.time()
            time.sleep(1.2)
            continue
        shown, tiles = st['hint']
        # (busy is the balloon: GameScreen::isBusy)
        if phase == 1 and c60 == 0 and not shown and st['balloon'] and st['balloon'][0]:
            path = tutorial_path(st['says'], st['marked'])
            if path:
                tiles, shown = [(m[0], m[3], m[4]) for m in path], 1
                log('the tutorial asks for %s' % ''.join(m[0] for m in path))
        if phase == 1 and c60 == 0 and shown and len(tiles) >= 3:
            log('word %s at level %d (score %d of %d)' % (''.join(t[0] for t in tiles), lvl, score, goal))
            for t in tiles:
                click(t[1], t[2])
                time.sleep(0.1)
            submit = [b for b in st['buttons'] if b[1] == 0xca]
            click(*(submit[0][2:4] if submit else (450, 557)))
            words += 1
            still = time.time()
            time.sleep(1.0)
            continue
        # a page between levels waits for a click on the story's next arrow; on the board a click would take a tile,
        # so there the player only waits (for the game's hint)
        if time.time() - still > 6 and phase != 1:
            log('nothing moves (phase %d): a click on the next arrow' % phase)
            click(745, 565)
            still = time.time()
        time.sleep(0.3)
    log('the time is up' if proc.poll() is None else 'the game ended')
    return words


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
    state_path = os.path.join(out, 'state.txt')
    env = dict(os.environ, BONNIES_STATE=state_path)
    proc = subprocess.Popen(cmd, env=env)
    log_file = open(os.path.join(out, 'play.log'), 'a', encoding='utf-8')

    def log(text):
        log_file.write('%8.1f %s\n' % (time.time() - started, text))
        log_file.flush()
    started = time.time()
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

    def click(x, y):
        # the move, press and release go in together, as visrun posts them: SDL asks Windows to say when the mouse
        # leaves the window, and Windows says so at once (the real cursor is elsewhere on rundesk's desktop), after
        # which SDL puts the mouse where Windows' cursor is. Messages already queued come before that leave, so a
        # press and release posted with their move land on the move's place; a press held a while would be dragged off
        # it (the board then drops its tile). A move before them hovers, as a hand reaching the button does.
        l = lparam_at(hwnd, x, y)
        post(WM_MOUSEMOVE, 0, l)
        time.sleep(0.15)
        post(WM_MOUSEMOVE, 0, l)
        post(WM_LBUTTONDOWN, 1, l)
        post(WM_LBUTTONUP, 0, l)

    def key(name):
        vk = VK[name]
        sc = u32.MapVirtualKeyW(vk, 0)
        post(WM_KEYDOWN, vk, 1 | (sc << 16))
        time.sleep(0.06)
        post(WM_KEYUP, vk, 0xC0000001 | (sc << 16))

    def shoot(name):
        try:
            shot(hwnd, os.path.join(out, name + '.png'))
        except Exception as e:
            print('realplay: shot', name, 'failed:', e)
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
            click(int(a[1]), int(a[2]))
        elif c == 'play':
            words = play(int(a[1]), click, key, shoot, proc, state_path, log)
            print('realplay: played', words, 'words')
        elif c == 'drag':
            x1, y1, x2, y2 = map(int, a[1:5])
            post(WM_MOUSEMOVE, 0, lparam_at(hwnd, x1, y1))
            time.sleep(0.15)
            post(WM_MOUSEMOVE, 0, lparam_at(hwnd, x1, y1))
            post(WM_LBUTTONDOWN, 1, lparam_at(hwnd, x1, y1))
            for i in range(1, 11):
                time.sleep(0.03)
                post(WM_MOUSEMOVE, 1, lparam_at(hwnd, x1 + (x2 - x1) * i // 10, y1 + (y2 - y1) * i // 10))
            time.sleep(0.08)
            post(WM_MOUSEMOVE, 1, lparam_at(hwnd, x2, y2))
            post(WM_LBUTTONUP, 0, lparam_at(hwnd, x2, y2))
        elif c == 'type':
            for ch in line.split(None, 1)[1]:
                vk = u32.VkKeyScanW(ord(ch)) & 0xFF
                # a real keyboard sends no WM_CHAR: the window's TranslateMessage (SDL calls it) makes it from the
                # key-down, with the keyboard's real shift state, so only lowercase letters, digits and blanks type
                # the key is held a moment, as a finger holds it: the WM_CHAR that TranslateMessage queues behind the
                # key-down must come before the key-up, or the host takes the character for a key of its own
                sc = u32.MapVirtualKeyW(vk, 0)
                post(WM_KEYDOWN, vk, 1 | (sc << 16))
                time.sleep(0.06)
                post(WM_KEYUP, vk, 0xC0000001 | (sc << 16))
                time.sleep(0.06)
        elif c == 'key':
            key(a[1])
        elif c == 'minimize':
            post(0x0112, 0xF020, 0)  # WM_SYSCOMMAND SC_MINIMIZE, as the title bar's button sends it
        elif c == 'restore':
            post(0x0112, 0xF120, 0)  # WM_SYSCOMMAND SC_RESTORE, as the taskbar's button sends it
        elif c == 'state':
            # the game's state (and the window's client size) at this point, kept as <name>.txt
            text = open(state_path, encoding='latin-1').read() if os.path.exists(state_path) else ''
            with open(os.path.join(out, a[1] + '.txt'), 'w', encoding='latin-1') as f:
                f.write('client %d %d\n' % client_size(hwnd) + text)
        elif c == 'close':
            post(0x0010, 0, 0)  # WM_CLOSE, as the window's close box sends it
        elif c == 'fullscreen':
            # Alt, then Enter, as a keyboard sends them (SDL keeps Alt's state from its own key-down)
            post(WM_SYSKEYDOWN, VK['MENU'], (0x38 << 16) | 1 | (1 << 29))
            time.sleep(0.05)
            post(WM_SYSKEYDOWN, VK['RETURN'], (0x1C << 16) | 1 | (1 << 29))
            time.sleep(0.05)
            post(WM_SYSKEYUP, VK['RETURN'], (0x1C << 16) | 1 | (1 << 29) | (3 << 30))
            time.sleep(0.05)
            post(WM_KEYUP, VK['MENU'], (0x38 << 16) | 1 | (3 << 30))
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
            shoot(a[1])
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
