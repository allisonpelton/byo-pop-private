"""Plays the port with real window messages only (the native pass of "Done means identical"): the mouse moves and
buttons and the keys are posted to the game's window, as Windows would post them, and the game runs on the real clock.
Run it under rundesk so the window, the sound and the priority stay out of the user's way:

    build/native/rundesk.exe python games/sevenseas/tools/realinput.py <scenario> <out dir> [--mod]

Pictures come from PrintWindow of the game's own window. Scenarios are functions at the end of the file."""
import ctypes, ctypes.wintypes as wt, os, subprocess, sys, time, shutil
from PIL import Image
u32 = ctypes.WinDLL('user32', use_last_error=True)
g32 = ctypes.WinDLL('gdi32')
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u32.GetDC.restype = wt.HDC
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint]
u32.PrintWindow.argtypes = [wt.HWND, wt.HDC, ctypes.c_uint]
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
EXE = os.path.join(ROOT, 'build', 'native', 'sevenseas.exe')
WM = dict(MOVE=0x200, LDOWN=0x201, LUP=0x202, KDOWN=0x100, KUP=0x101, CHAR=0x102)

class Game:
    def __init__(self, userdir, args=()):
        self.out = None
        os.makedirs(userdir, exist_ok=True)
        self.p = subprocess.Popen([EXE, '-userdir=' + userdir] + list(args), cwd=ROOT)
        self.hwnd = None
        for _ in range(200):
            time.sleep(0.1)
            self.hwnd = self.find()
            if self.hwnd: break
        assert self.hwnd, 'no window'
        self.focus(); time.sleep(1.5)
    def focus(self, on=True):
        # what Windows posts when the window is activated or deactivated
        if on:   # the real focus of the private desktop, so the window is active as it is for a player
            k32 = ctypes.WinDLL('kernel32'); tid = u32.GetWindowThreadProcessId(self.hwnd, None); me = k32.GetCurrentThreadId()
            u32.AttachThreadInput(me, tid, True); u32.SetForegroundWindow(self.hwnd); u32.SetActiveWindow(self.hwnd)
            u32.SetFocus(self.hwnd); u32.SwitchToThisWindow(self.hwnd, True); u32.AttachThreadInput(me, tid, False)
            print('foreground', u32.GetForegroundWindow(), 'window', self.hwnd, flush=True)
        self.post(0x1C, int(on), 0); self.post(0x6, 1 if on else 0, 0); self.post(0x7 if on else 0x8, 0, 0)
    def find(self):
        found = []
        CB = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
        def cb(h, _):
            pid = wt.DWORD()
            u32.GetWindowThreadProcessId(h, ctypes.byref(pid))
            if pid.value == self.p.pid and u32.IsWindowVisible(h): found.append(h)
            return True
        u32.EnumWindows(CB(cb), 0)
        return found[0] if found else None
    def post(self, m, w, l): u32.PostMessageW(self.hwnd, m, w, l)
    def sc(self, x, y):
        # the game is scaled to fit the window and centred, so a full screen of another shape has bars at the sides
        r = wt.RECT(); u32.GetClientRect(self.hwnd, ctypes.byref(r)); k = min(r.right / 640.0, r.bottom / 480.0)
        return int(x * k + (r.right - 640 * k) / 2), int(y * k + (r.bottom - 480 * k) / 2)
    def move(self, x, y):
        x, y = self.sc(x, y); self.post(WM['MOVE'], 0, (y << 16) | x)
    def click(self, x, y, hold=0.06):
        # move, press and release in one burst: Windows sends the game its own mouse messages for the real cursor (at the
        # corner of the private desktop), and any of those between the move and the press would take the press elsewhere
        a, b = self.sc(x, y); pos = (b << 16) | a
        self.post(WM['MOVE'], 0, pos); time.sleep(0.12)   # hover first, as a hand does
        self.post(WM['MOVE'], 0, pos); self.post(WM['LDOWN'], 1, pos); time.sleep(hold)
        self.post(WM['MOVE'], 1, pos); self.post(WM['LUP'], 0, pos); time.sleep(0.3)
    def key(self, vk, hold=0.06):
        # SDL takes the key from the scan code in bits 16-23 of lParam, so it is filled in as the keyboard driver does
        sc = u32.MapVirtualKeyW(vk, 0); ext = 1 if vk in (0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2D, 0x2E) else 0
        base = 1 | (sc << 16) | (ext << 24)
        self.post(WM['KDOWN'], vk, base); time.sleep(hold)
        self.post(WM['KUP'], vk, base | 0xC0000000); time.sleep(0.2)
    def shot(self, name):
        r = wt.RECT(); u32.GetClientRect(self.hwnd, ctypes.byref(r)); w, h = r.right, r.bottom
        dc = u32.GetDC(self.hwnd); mdc = g32.CreateCompatibleDC(dc); bm = g32.CreateCompatibleBitmap(dc, w, h)
        g32.SelectObject(mdc, bm); u32.PrintWindow(self.hwnd, mdc, 3)
        class BI(ctypes.Structure):
            _fields_ = [('s', wt.DWORD), ('w', wt.LONG), ('h', wt.LONG), ('p', wt.WORD), ('b', wt.WORD), ('c', wt.DWORD),
                        ('i', wt.DWORD), ('x', wt.LONG), ('y', wt.LONG), ('u', wt.DWORD), ('m', wt.DWORD)]
        bi = BI(40, w, -h, 1, 32, 0, 0, 0, 0, 0, 0); buf = ctypes.create_string_buffer(w * h * 4)
        g32.GetDIBits(mdc, bm, 0, h, buf, ctypes.byref(bi), 0)
        Image.frombuffer('RGBA', (w, h), buf, 'raw', 'BGRA', 0, 1).convert('RGB').save(os.path.join(self.out, name + '.png'))
    def alive(self): return self.p.poll() is None
    def close(self):
        if self.alive(): self.p.terminate()

def scn_flow(g):
    g.shot('a_title'); g.click(320, 385); time.sleep(1.5); g.shot("b_menu")
    g.click(405, 306); time.sleep(0.6); g.shot('c_wheel')
    g.click(502, 398); time.sleep(2.5); g.shot('d_game1'); time.sleep(3); g.shot('e_game2')

def scn_k(g):
    g.shot('k0'); g.key(0x20); time.sleep(1); g.shot('k1'); g.key(0x0D); time.sleep(1); g.shot('k2')
    g.focus(); time.sleep(0.5); g.click(320, 300); time.sleep(1); g.shot('k3')
    for i in range(5): g.click(320 + i, 300); time.sleep(0.3)
    g.shot('k4')

def scn_info(g):
    CB = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        pid = wt.DWORD(); u32.GetWindowThreadProcessId(h, ctypes.byref(pid))
        if pid.value == g.p.pid:
            n = ctypes.create_unicode_buffer(100); u32.GetClassNameW(h, n, 100); t = ctypes.create_unicode_buffer(100); u32.GetWindowTextW(h, t, 100)
            r = wt.RECT(); u32.GetClientRect(h, ctypes.byref(r))
            print(h, n.value, t.value, u32.IsWindowVisible(h), r.right, r.bottom, 'fg', u32.GetForegroundWindow(), 'focus', u32.GetFocus(), 'active', u32.GetActiveWindow(), flush=True)
        return True
    u32.EnumWindows(CB(cb), 0); print('chosen', g.hwnd)

def to_menu(g):
    g.click(320, 385); time.sleep(3)

def scn_tut0(g):
    to_menu(g); g.click(305, 397); time.sleep(1); g.shot('t0'); g.click(305, 397); time.sleep(3); g.shot('t1')

def scn_serve(g):
    # runs the commands appended to <out>/cmds.txt, one per line: click x y | key vk | shot name | wait s | focus | quit
    cmds = os.path.join(g.out, 'cmds.txt'); open(cmds, 'a').close(); done = 0
    while g.alive():
        lines = open(cmds).read().splitlines()
        if done >= len(lines): time.sleep(0.2); continue
        w = lines[done].split(); done += 1
        if not w: continue
        if w[0] == 'click': g.click(float(w[1]), float(w[2]))
        elif w[0] == 'key': g.key(int(w[1], 0))
        elif w[0] == 'shot': g.shot(w[1])
        elif w[0] == 'wait': time.sleep(float(w[1]))
        elif w[0] == 'focus': g.focus(w[1] != 'lose')
        elif w[0] == 'quit': return

import random
WHEEL = {'easy': (285, 307), 'average': (405, 306), 'hard': (525, 307)}
DIRS = [0x61, 0x62, 0x63, 0x64, 0x66, 0x67, 0x68, 0x69]   # the numpad around 5
ARROWS = [0x25, 0x26, 0x27, 0x28]

def start_game(g, level):
    to_menu(g); g.click(*WHEEL[level]); time.sleep(0.8); g.click(507, 397); time.sleep(4)
    if level == 'easy':
        g.shot('demo'); g.click(406, 440); time.sleep(40); g.shot('after_demo')   # the demo's "Click Here to Start!"

def play(g, seconds, tag, seed=1):
    # random keys, ship broadsides and clicks on random squares: the game must keep up, take them all and never freeze
    rnd = random.Random(seed); t0 = time.time(); n = 0
    while time.time() - t0 < seconds and g.alive():
        r = rnd.random()
        if r < 0.45: g.key(rnd.choice(DIRS))
        elif r < 0.55: g.key(rnd.choice(ARROWS))
        elif r < 0.65: g.key(0x20)
        else: g.click(173 + rnd.randrange(11) * 42 + 21, rnd.randrange(11) * 42 + 21)
        time.sleep(rnd.random() * 0.8)
        n += 1
        if n % 12 == 0: g.shot('%s_%03d' % (tag, n))

def scn_easy(g):
    start_game(g, 'easy'); play(g, 100, 'easy'); g.shot('easy_end')

def scn_average(g):
    start_game(g, 'average'); play(g, 100, 'avg'); g.shot('avg_end')

def scn_hard(g):
    start_game(g, 'hard'); play(g, 100, 'hard'); g.shot('hard_end')

def scn_saved(g):
    # a saved game, Average, "No" in the Continue dialog: do the squares under the pirate's box (columns 0-3, rows 0-4) answer?
    start_game(g, 'average'); g.click(404, 273); time.sleep(2)   # one move, then save
    g.click(87, 445); time.sleep(1.5); g.shot('new_game_dialog'); g.click(318, 202); time.sleep(3)   # Exit: New Game, Yes saves
    g.click(*WHEEL['average']); time.sleep(0.8); g.click(502, 398); time.sleep(3); g.shot('continue_dialog')
    g.click(459, 254); time.sleep(3); g.shot('after_no')
    g.click(362, 231); time.sleep(2.5); g.shot('west')         # the ship one square west, to (4, 5)
    g.move(320, 189); time.sleep(1.2); g.shot('hover')          # the square above-left of it, under the pirate's box
    g.click(320, 189); time.sleep(2.5); g.shot('clicked')

def scn_fullscreen(g):
    # Options: the full screen switch, then a few moves in it and back
    start_game(g, 'average'); g.click(87, 261); time.sleep(2); g.shot('options')
    g.click(274, 254); time.sleep(4); g.shot('fs_on')
    r = wt.RECT(); u32.GetClientRect(g.hwnd, ctypes.byref(r)); print('client', r.right, r.bottom, flush=True)
    g.click(405, 354); time.sleep(3); g.shot('fs_done'); print('client after', *[getattr(r, a) for a in ('right', 'bottom')], flush=True); u32.GetClientRect(g.hwnd, ctypes.byref(r)); print('client now', r.right, r.bottom, flush=True)
    g.click(404, 273); time.sleep(2); g.shot('fs_move'); g.key(0x68); time.sleep(2); g.shot('fs_key')
    g.click(87, 261); time.sleep(2); g.click(274, 254); time.sleep(4); g.shot('fs_off'); g.click(405, 354); time.sleep(1); g.click(404, 231); time.sleep(2); g.shot('win_move')

def scn_probe(g):
    g.shot('start'); time.sleep(3); g.shot('t3'); g.click(405, 306); g.shot('click')

if __name__ == '__main__':
    scn, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    g = Game(os.path.join(out, 'user'), sys.argv[3:])
    g.out = out
    try: globals()['scn_' + scn](g)
    finally: g.close()
