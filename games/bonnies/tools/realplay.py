"""Plays the port with real input only, never -script (which drops real input) or autoplay. Run it under rundesk, which
puts it and the game on a private desktop:

    build/native/rundesk.exe python games/bonnies/tools/realplay.py <steps> <out dir> [--live] -- <bonnies.exe> <args...>

Without --live the input is posted window messages, as a hand's reach the window: the menus, the name box, the difficulty,
the story's pages, the dialogs and the window's own commands answer them (realplay_tour.txt, realplay_window.txt), but the
board does not: it follows a held press from frame to frame, and a posted click is a move, press and release in one burst
(a press held a while would be dragged to the real cursor, docs/IDENTICAL_PLAY.md, "Real input natively"), so no tile ever
takes it. A `play` step stalled on the tutorial's first word that way (FROG, 0 of 44), and now refuses to run without --live.

With --live the game also gets -realinput (an empty file) and -commands (docs/REAL_INPUT.md, docs/TEST_COMMANDS.md), and
every step goes through that channel: its clicks, keys, typed text and focus changes are pushed into SDL's own event
queue at an update, a press held to the next frame as a hand's is, so the board takes them. Run it with -fast to play on
the virtual clock. `play` asks the port's own bot for each move with the test command "move" (game/GameScreen.cpp,
testMove) and sends back the clicks it returns, so that a later bot mod reuses the bot. The game's log is <out>/game.log,
the driver's <out>/play.log.

The steps file has one command a line ('#' starts a comment); x and y are the game's 800x600 coordinates:
    wait <ms>            move <x> <y>           click <x> <y>        (posted: a move, 150 ms, then a move, down and up)
    drag <x1> <y1> <x2> <y2>                     type <text>          (a key press and release for each character)
    key <name>           (RETURN, ESCAPE, BACK, SPACE, TAB, F4...)  shot <name> (the game's window as it shows)
    focus lose|regain    (posted: WM_ACTIVATEAPP and WM_KILLFOCUS / WM_SETFOCUS; live: the window's focus events)
    close                (the window's close box)
    minimize, restore    (as the title bar's and the taskbar's buttons send it)
    state <name>         (the window's client size and the game's state file, kept as <name>.txt)
    fullscreen           (Alt+Enter)             quit-when-gone <ms>  (waits up to ms for the game to end by itself)
    play <s>             (--live only: plays the classic game on screen for up to s seconds, from the story's first page
                         or the board, as a player who takes the bot's moves: see play() below)
The window's client area is mapped to the game's 800x600 whatever its size (windowed or full screen).
"""
import ctypes
import json
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


KEYS = {'RETURN': 'Return', 'ESCAPE': 'Escape', 'BACK': 'Backspace', 'SPACE': 'Space', 'TAB': 'Tab', 'LEFT': 'Left',
        'UP': 'Up', 'RIGHT': 'Right', 'DOWN': 'Down', 'F4': 'F4', 'DELETE': 'Delete', 'MENU': 'Left Alt'}  # SDL's names


class Posted:
    """The input as posted window messages, as a hand's reach the window"""
    live = False

    def __init__(self, proc, out, log):
        self.proc, self.out = proc, out
        self.hwnd = find_window(proc.pid, 30)
        if not self.hwnd:
            raise RuntimeError('no window')
        cls = ctypes.create_unicode_buffer(256)
        u32.GetClassNameW(wt.HWND(self.hwnd), cls, 256)
        print('realplay: window', self.hwnd, cls.value, client_size(self.hwnd))

    def refresh(self):
        self.hwnd = find_window(self.proc.pid, 5) or self.hwnd

    def post(self, m, w, l):
        u32.PostMessageW(self.hwnd, m, w, l)

    def move(self, x, y):
        self.post(WM_MOUSEMOVE, 0, lparam_at(self.hwnd, x, y))

    def click(self, x, y):
        # the move, press and release go in together, as visrun posts them: SDL asks Windows to say when the mouse
        # leaves the window, and Windows says so at once (the real cursor is elsewhere on rundesk's desktop), after
        # which SDL puts the mouse where Windows' cursor is. Messages already queued come before that leave, so a
        # press and release posted with their move land on the move's place; a press held a while would be dragged off
        # it. A move before them hovers, as a hand reaching the button does. The board takes no such click: it
        # follows a held press from frame to frame (docs/IDENTICAL_PLAY.md, "Real input natively"), hence --live.
        l = lparam_at(self.hwnd, x, y)
        self.post(WM_MOUSEMOVE, 0, l)
        time.sleep(0.15)
        self.post(WM_MOUSEMOVE, 0, l)
        self.post(WM_LBUTTONDOWN, 1, l)
        self.post(WM_LBUTTONUP, 0, l)

    def drag(self, x1, y1, x2, y2):
        h = self.hwnd
        self.post(WM_MOUSEMOVE, 0, lparam_at(h, x1, y1))
        time.sleep(0.15)
        self.post(WM_MOUSEMOVE, 0, lparam_at(h, x1, y1))
        self.post(WM_LBUTTONDOWN, 1, lparam_at(h, x1, y1))
        for i in range(1, 11):
            time.sleep(0.03)
            self.post(WM_MOUSEMOVE, 1, lparam_at(h, x1 + (x2 - x1) * i // 10, y1 + (y2 - y1) * i // 10))
        time.sleep(0.08)
        self.post(WM_MOUSEMOVE, 1, lparam_at(h, x2, y2))
        self.post(WM_LBUTTONUP, 0, lparam_at(h, x2, y2))

    def key(self, name):
        vk = VK[name]
        sc = u32.MapVirtualKeyW(vk, 0)
        self.post(WM_KEYDOWN, vk, 1 | (sc << 16))
        time.sleep(0.06)
        self.post(WM_KEYUP, vk, 0xC0000001 | (sc << 16))

    def type(self, text):
        for ch in text:
            vk = u32.VkKeyScanW(ord(ch)) & 0xFF
            # a real keyboard sends no WM_CHAR: the window's TranslateMessage (SDL calls it) makes it from the
            # key-down, with the keyboard's real shift state, so only lowercase letters, digits and blanks type
            # the key is held a moment, as a finger holds it: the WM_CHAR that TranslateMessage queues behind the
            # key-down must come before the key-up, or the host takes the character for a key of its own
            sc = u32.MapVirtualKeyW(vk, 0)
            self.post(WM_KEYDOWN, vk, 1 | (sc << 16))
            time.sleep(0.06)
            self.post(WM_KEYUP, vk, 0xC0000001 | (sc << 16))
            time.sleep(0.06)

    def shot(self, name):
        shot(self.hwnd, os.path.join(self.out, name + '.png'))

    def focus(self, lose):
        if lose:
            self.post(WM_ACTIVATEAPP, 0, 0)
            self.post(WM_ACTIVATE, 0, 0)
            self.post(WM_KILLFOCUS, 0, 0)
        else:
            self.post(WM_ACTIVATEAPP, 1, 0)
            self.post(WM_ACTIVATE, 1, 0)
            self.post(WM_SETFOCUS, 0, 0)

    def minimize(self):
        self.post(0x0112, 0xF020, 0)  # WM_SYSCOMMAND SC_MINIMIZE, as the title bar's button sends it

    def restore(self):
        self.post(0x0112, 0xF120, 0)  # WM_SYSCOMMAND SC_RESTORE, as the taskbar's button sends it

    def close(self):
        self.post(0x0010, 0, 0)  # WM_CLOSE, as the window's close box sends it

    def fullscreen(self):
        # Alt, then Enter, as a keyboard sends them (SDL keeps Alt's state from its own key-down)
        self.post(WM_SYSKEYDOWN, VK['MENU'], (0x38 << 16) | 1 | (1 << 29))
        time.sleep(0.05)
        self.post(WM_SYSKEYDOWN, VK['RETURN'], (0x1C << 16) | 1 | (1 << 29))
        time.sleep(0.05)
        self.post(WM_SYSKEYUP, VK['RETURN'], (0x1C << 16) | 1 | (1 << 29) | (3 << 30))
        time.sleep(0.05)
        self.post(WM_KEYUP, VK['MENU'], (0x38 << 16) | 1 | (3 << 30))

    def client(self):
        return client_size(self.hwnd)


class Live:
    """The input through the framework's live channel (-realinput with an empty file, and -commands: docs/REAL_INPUT.md,
    docs/TEST_COMMANDS.md): each click, key and focus change goes into SDL's own event queue at an update, and its
    press is held to the next frame, as a hand's is, so the board takes it. The game's test commands (state, move)
    answer through the same file."""
    live = True

    def __init__(self, proc, out, log, commands, game_log):
        self.proc, self.out, self.log, self.commands, self.game_log = proc, out, log, commands, game_log
        self.seen = 0

    def refresh(self):
        pass

    def new_lines(self):
        with open(self.game_log, encoding='latin-1') as f:
            f.seek(self.seen)
            text = f.read()
        cut = text.rfind('\n') + 1
        self.seen += len(text[:cut].encode('latin-1'))
        return text[:cut].splitlines()

    def send(self, line, answer=None, timeout=20):
        """Appends a command and waits for its answer line (OK/ERROR <line>, or one starting with answer)"""
        with open(self.commands, 'a', encoding='utf-8') as f:
            f.write(line + '\n')
        end = time.time() + timeout
        while time.time() < end:
            for l in self.new_lines():
                # (the framework's answers come as "INFO: OK <line>", the game's own as they are)
                l = l[6:] if l.startswith('INFO: ') else l
                if l.startswith('OK ' + line) or l.startswith('ERROR ' + line) or (answer and l.startswith(answer)):
                    return l
            if self.proc.poll() is not None:
                return None
            time.sleep(0.02)
        self.log('no answer to ' + line)
        return None

    def state(self):
        """The game's state (the test command "state", docs/TEST_COMMANDS.md) as JSON, or None"""
        l = self.send('state', 'STATE ')
        try:
            return json.loads(l[6:]) if l and l.startswith('STATE ') else None
        except ValueError:
            return None

    def bot_move(self):
        """The port's bot's next move (the test command "move"): {"kind": button|tutorial|word|hint, "word", "cells",
        "input": [{"click": [x, y]}...]}, or None when it has none"""
        l = self.send('move', 'MOVE ')
        try:
            return json.loads(l[5:]) if l and l.startswith('MOVE {') else None
        except ValueError:
            return None

    def move(self, x, y):
        self.send('move game %d %d' % (round(x), round(y)))

    def click(self, x, y):
        self.send('click game %d %d' % (round(x), round(y)))

    def drag(self, x1, y1, x2, y2):
        self.send('move game %d %d' % (x1, y1))
        self.send('down game %d %d' % (x1, y1))
        for i in range(1, 11):
            self.send('wait 2')
            self.send('move game %d %d' % (x1 + (x2 - x1) * i // 10, y1 + (y2 - y1) * i // 10))
        self.send('wait 2')
        self.send('up game %d %d' % (x2, y2))

    def key(self, name):
        self.send('key ' + KEYS[name])

    def type(self, text):
        self.send('type ' + text)

    def shot(self, name):
        self.send('screenshot ' + os.path.join(self.out, name + '.png').replace('\\', '/'))

    def focus(self, lose):
        self.send('focus lost' if lose else 'focus gained')

    def minimize(self):
        self.send('focus lost')  # a hand's minimize also takes the focus away
        self.send('minimize')

    def restore(self):
        self.send('restore')
        self.send('focus gained')

    def close(self):
        self.send('close')

    def fullscreen(self):
        self.send('keydown Left Alt')
        self.send('key Return')
        self.send('keyup Left Alt')

    def client(self):
        return (0, 0)


def play(seconds, drv, proc, state_path, log):
    """A player who takes the port's own bot's moves (the test command "move": a tutorial balloon's button or the word it
    names over the tiles it marks, else the best word, else the game's hint), clicking the move's tiles and Submit as
    live input. Around the board it answers a dialog with its first button that neither leaves the game nor opens
    anything (DENY), clicks on through the pages between levels, and presses Scramble when the latest words fill no new
    cell. Ends when the game screen is gone (the story's end, or the game lost), the game ends, or the time is up."""
    end = time.time() + seconds
    result = {'words': 0, 'levels': [], 'scrambles': 0, 'end': None}
    seen, still, last_summary = False, time.time(), ''
    filled_best, idle, fill_level, last_word, refused = -1, 0, 0, '', []

    def settle(before):
        """waits for the game to take a move (its score, moves, phase or dialogs change), then for the board to stand"""
        for _ in range(100):
            j = drv.state()
            if j is None or keys_of(j) != before:
                break
            time.sleep(0.05)
        board = None
        for _ in range(60):
            j = drv.state()
            b = j and j.get('board')
            if b == board:
                break
            board = b
            time.sleep(0.1)

    def keys_of(j):
        return [j.get(k) for k in ('level', 'score', 'moves', 'phase', 'modal')]
    while time.time() < end and proc.poll() is None:
        st = read_state(state_path)
        j = drv.state()
        if st is None or j is None:
            time.sleep(0.2)
            continue
        summary = json.dumps([j.get('screen'), j.get('modal'), j.get('level'), j.get('phase'),
                              [b[4] for b in st['buttons']] if st['modal'] else 0])
        if summary != last_summary:
            log('state ' + summary)
            last_summary, still = summary, time.time()
        if j.get('screen') == 'game' and j.get('level') and (not result['levels'] or result['levels'][-1] != j['level']):
            result['levels'].append(j['level'])
            drv.shoot('level%02d' % j['level'])
        if st['modal'] > 0:
            texts = ' / '.join(b[4] for b in st['buttons'])
            b = next((b for b in st['buttons'] if allowed(b[4])), None)
            if b:
                log('dialog (%s): %s' % (texts, b[4]))
                if seen and ('turns' in texts.lower() or 'over' in texts.lower()):
                    result['end'] = texts
                drv.click(b[2], b[3])
            elif time.time() - still > 3:
                log('dialog without a button to press (%s): RETURN' % texts)
                drv.key('RETURN')
                still = time.time()
            time.sleep(0.5)
            continue
        if j.get('screen') != 'game':
            if seen and j.get('screen') in ('menu', 'highscores', 'credits', 'title'):
                result['end'] = result['end'] or ('the game is over: ' + j['screen'])
                log(result['end'])
                break
            if time.time() - still > 4:
                # the story's pages, before the first board and between levels: the next arrow, as the tour clicks it
                log('before the board (%s): a click on the next arrow' % j.get('screen'))
                drv.click(745, 565)
                still = time.time()
            time.sleep(0.2)
            continue
        seen = True
        if j.get('phase') != 1:
            if time.time() - still > 4:
                log('nothing moves (phase %s): a click on the next arrow' % j.get('phase'))
                drv.click(745, 565)
                still = time.time()
            time.sleep(0.2)
            continue
        m = drv.bot_move()
        if m and m['kind'] == 'button':
            log('button %d' % m['id'])
            for s in m['input']:
                drv.click(*s['click'])
            still = time.time()
            time.sleep(0.5)
            continue
        balloon = bool(st['balloon'] and st['balloon'][0])
        if j.get('fill') is not None and not balloon:
            if j['level'] != fill_level:
                fill_level, filled_best = j['level'], -1
            if j['filled'] > filled_best:
                filled_best, idle = j['filled'], 0
            scramble = next((b for b in st['buttons'] if b[1] == 0xcb), None)
            if idle >= (3 if j['fill'] - filled_best <= 3 else 8) and scramble:
                log('scramble: %d cells left, none filled in %d words' % (j['fill'] - j['filled'], idle))
                drv.click(scramble[2], scramble[3])
                result['scrambles'] += 1
                idle = 0
                settle(keys_of(j))
                continue
        if not m:
            time.sleep(0.3)
            continue
        # the same word on an unchanged board twice: the game would not take it, so Scramble, as a player would
        if m['word'] == last_word and [j.get(k) for k in ('score', 'moves', 'board')] == refused:
            scramble = next((b for b in st['buttons'] if b[1] == 0xcb), None)
            log('the same word again, refused: ' + ('scramble' if scramble else 'waiting'))
            if scramble:
                drv.click(scramble[2], scramble[3])
                result['scrambles'] += 1
                settle(keys_of(j))
                last_word = ''
            else:
                time.sleep(0.5)
            continue
        last_word, refused = m['word'], [j.get(k) for k in ('score', 'moves', 'board')]
        log('%s %s at level %s (score %s, moves %s, filled %s of %s)' % (m['kind'], m['word'], j['level'], j['score'],
                                                                       j['moves'], j['filled'], j['fill']))
        for s in m['input']:
            drv.click(*s['click'])
        result['words'] += 1
        idle += 1
        settle(keys_of(j))
    log('the play ends: %s' % (result['end'] or ('the game ended' if proc.poll() is not None else 'the time is up')))
    return result


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
    steps, out = sys.argv[1], os.path.abspath(sys.argv[2])
    live = '--live' in sys.argv[3:sep]
    cmd = sys.argv[sep + 1:]
    os.makedirs(out, exist_ok=True)
    u32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
    u32.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]
    u32.IsWindowVisible.argtypes = [wt.HWND]
    state_path = os.path.join(out, 'state.txt')
    env = dict(os.environ, BONNIES_STATE=state_path)
    log_file = open(os.path.join(out, 'play.log'), 'a', encoding='utf-8')

    def log(text):
        log_file.write('%8.1f %s\n' % (time.time() - started, text))
        log_file.flush()
    started = time.time()
    if live:
        empty, commands, game_log = (os.path.join(out, n) for n in ('empty.txt', 'commands.txt', 'game.log'))
        for p in (commands, state_path):
            if os.path.exists(p):
                os.remove(p)
        open(empty, 'w').close()
        open(commands, 'w').close()
        with open(game_log, 'w', encoding='utf-8') as gl:
            proc = subprocess.Popen(cmd + ['-realinput=' + empty, '-commands=' + commands], env=env, stdout=gl,
                                    stderr=subprocess.STDOUT)
    else:
        proc = subprocess.Popen(cmd, env=env)
    PROCS.append(proc)
    if live:
        drv = Live(proc, out, log, commands, game_log)
        # the window exists once the state file does, and a -realinput run has the focus only when its file gives it
        for _ in range(600):
            if os.path.exists(state_path) or proc.poll() is not None:
                break
            time.sleep(0.1)
        drv.send('focus gained')
    else:
        try:
            drv = Posted(proc, out, log)
        except RuntimeError as e:
            print('realplay:', e)
            proc.kill()
            return 1

    def shoot(name):
        try:
            drv.shot(name)
        except Exception as e:
            print('realplay: shot', name, 'failed:', e)
    drv.shoot = shoot
    for line in open(steps, encoding='utf-8'):
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        if proc.poll() is not None:
            print('realplay: the game ended before', line)
            break
        drv.refresh()
        a = line.split()
        c = a[0]
        if c == 'wait':
            time.sleep(int(a[1]) / 1000)
        elif c == 'move':
            drv.move(int(a[1]), int(a[2]))
        elif c == 'click':
            drv.click(int(a[1]), int(a[2]))
        elif c == 'play':
            if not drv.live:
                # (this is what stalled in the tutorial: its tiles never took the posted clicks)
                print('realplay: play needs --live: the board takes no posted click (docs/IDENTICAL_PLAY.md, '
                      '"Real input natively")')
                log('play needs --live')
                continue
            result = play(int(a[1]), drv, proc, state_path, log)
            log('summary ' + json.dumps(result))
            print('realplay: played', json.dumps(result))
        elif c == 'drag':
            drv.drag(*map(int, a[1:5]))
        elif c == 'type':
            drv.type(line.split(None, 1)[1])
        elif c == 'key':
            drv.key(a[1])
        elif c == 'minimize':
            drv.minimize()
        elif c == 'restore':
            drv.restore()
        elif c == 'state':
            # the game's state (and the window's client size) at this point, kept as <name>.txt
            text = open(state_path, encoding='latin-1').read() if os.path.exists(state_path) else ''
            with open(os.path.join(out, a[1] + '.txt'), 'w', encoding='latin-1') as f:
                f.write('client %d %d\n' % drv.client() + text)
        elif c == 'close':
            drv.close()
        elif c == 'fullscreen':
            drv.fullscreen()
        elif c == 'focus':
            drv.focus(a[1] == 'lose')
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
