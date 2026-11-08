"""Plays the Alchemy port with real input only (CLAUDE.md, "Done means identical": autoplay and script commands never pass
through that path): window messages posted to the game's own window, as a player's mouse and keys would give them, the
game's state read from its log, and each move taken from the port's own bot (the test commands `state` and `move`,
docs/TEST_COMMANDS.md). Code plays; a session reads the result file and the shots.

    build/native/rundesk.exe python games/alchemy/tools/realplay/drive.py <out dir> <step>... [--seed N] [--updates N]

Run it under rundesk only: the window is then on rundesk's private desktop, and so is this script, which finds the window
by the game's process id. The game runs in real time, started with -script (a state and a move every 5 updates, so the
driver always has a fresh answer) and BYOPOP_POSTED_INPUT, which keeps the posted mouse and keys of such a run.

Steps (run in order in one game process; a step ends when its goal is reached or fails the whole run):
    launch                the title: wait for the link, click it; the selector comes up
    game <mode> <skill>   from the selector: click the skill and mode buttons and Start, then play with the bot's moves
                          (hover, click; right click for the forge; the wizard's boxes; the cleared-board panel; the name
                          entry; the score table) until the selector is back. mode: strategic | timetrial; skill 1..3
    tutorial              from the selector: click the tutorial picture, then play the 14 steps by the bot until the
                          selector is back
    windowed | fullscreen the options dialog from the side panel: the Fullscreen box set the other way, then DONE
    shot <name>           a picture of the window (the game's own screenshot) in <out dir>/<name>.png
    wait <seconds>
    focus lose | regain   WM_ACTIVATE / WM_KILLFOCUS / WM_SETFOCUS posted to the window (a -script run drops these on the
                          game's side, so this only shows the driver sent them; the focus pass uses -realinput)
    quit                  the side panel's quit button, then Yes

--modsdir <dir> starts the game with that mods folder (the fixes mod: one folder named fixes inside it).
Script lines: --line "<update> <command>" adds one (focus lose, focus regain at fixed updates), repeatable.
Limits: --game-seconds N caps one game (default 420). --stop-after-pieces N ends a game early by `lose` (not real input,
only to bound a run). Exit code 0 when every step reached its goal.
"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import subprocess
import sys
import threading
import time

u32 = ctypes.WinDLL('user32', use_last_error=True)
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u32.MapVirtualKeyW.argtypes = [ctypes.c_uint, ctypes.c_uint]

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
EXE = os.path.join(ROOT, 'build', 'native', 'alchemy.exe')
WM_CLOSE, WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP = 0x10, 0x200, 0x201, 0x202, 0x204, 0x205
WM_KEYDOWN, WM_KEYUP, WM_CHAR = 0x100, 0x101, 0x102
WM_ACTIVATE, WM_SETFOCUS, WM_KILLFOCUS, WM_ACTIVATEAPP = 0x6, 0x7, 0x8, 0x1c
GAME_W, GAME_H = 640, 480


class Failure(Exception):
    pass


class Game:
    def __init__(self, out, seed=1, updates=120000, step=5, extra=(), lines=()):
        self.out = os.path.abspath(out)
        os.makedirs(self.out, exist_ok=True)
        self.log = open(os.path.join(self.out, 'drive.log'), 'w', buffering=1)
        script = os.path.join(self.out, 'script.txt')
        with open(script, 'w') as f:
            allv = [(n, 'state') for n in range(0, updates, step)] + [(n, 'move') for n in range(0, updates, step)]
            allv += [(int(ln.split()[0]), ln.split(None, 1)[1]) for ln in lines]   # e.g. "4000 focus lose"
            for n, cmd in sorted(allv, key=lambda v: v[0]):
                f.write('%d %s\n' % (n, cmd))
        self.shotreq = os.path.join(self.out, 'shot.req')
        env = dict(os.environ, BYOPOP_POSTED_INPUT='1', BYOPOP_SHOT_REQUEST=self.shotreq)
        self.stdout_path = os.path.join(self.out, 'stdout.txt')
        self.p = subprocess.Popen([EXE, '-userdir=' + os.path.join(self.out, 'user'), '-seed=%d' % seed, '-script=' + script] + list(extra),
                                  env=env, stdout=open(self.stdout_path, 'w', buffering=1), stderr=open(os.path.join(self.out, 'stderr.txt'), 'w'))
        self.hwnd = None
        for _ in range(600):
            self.hwnd = self.find_window()
            if self.hwnd or self.p.poll() is not None:
                break
            time.sleep(0.1)
        if not self.hwnd:
            raise Failure('no window; exit %s' % self.p.poll())
        self.reader = open(self.stdout_path)
        self.buf = ''
        self.state = None
        self.pending_move = None
        self.sent = None          # (signature, time) of the last move sent
        self.last_places = 0
        self.last_tut = None
        self.stats = dict(places=0, forges=0, clicks=0, dialogs=0, retries=0)
        self.say('window %r' % self.title())

    def say(self, *a):
        print('%.1f' % time.time(), *a, file=self.log, flush=True)

    # ---- the window ----
    def title(self):
        b = ctypes.create_unicode_buffer(256)
        u32.GetWindowTextW(self.hwnd, b, 256)
        return b.value

    def find_window(self):
        found = []
        PROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)

        def cb(h, _):
            pid = wt.DWORD()
            u32.GetWindowThreadProcessId(h, ctypes.byref(pid))
            if pid.value == self.p.pid and u32.IsWindowVisible(h):
                found.append(h)
            return True
        u32.EnumWindows(PROC(cb), 0)
        return found[0] if found else None

    def client(self):
        r = wt.RECT()
        u32.GetClientRect(self.hwnd, ctypes.byref(r))
        return r.right, r.bottom

    def pt(self, x, y):
        # the game's picture is fitted into the client area and centred (letterboxed in full screen)
        w, h = self.client()
        k = min(w / GAME_W, h / GAME_H)
        bx, by = (w - GAME_W * k) / 2, (h - GAME_H * k) / 2
        cx, cy = int(round(bx + (x + 0.5) * k)), int(round(by + (y + 0.5) * k))
        return (cy << 16) | (cx & 0xffff)

    def post(self, m, w, l):
        u32.PostMessageW(self.hwnd, m, w, l)

    def move(self, x, y):
        self.post(WM_MOUSEMOVE, 0, self.pt(x, y))

    def click(self, x, y, right=False):
        # one burst: SDL answers a posted move with TrackMouseEvent, and the leave that follows would take the press elsewhere
        p = self.pt(x, y)
        self.post(WM_MOUSEMOVE, 0, p)
        self.post(WM_RBUTTONDOWN if right else WM_LBUTTONDOWN, 2 if right else 1, p)
        self.post(WM_RBUTTONUP if right else WM_LBUTTONUP, 0, p)
        self.stats['clicks'] += 1

    def key(self, name):
        if len(name) == 1:
            vk = ord(name.upper())
            sc = u32.MapVirtualKeyW(vk, 0)
            self.post(WM_KEYDOWN, vk, 1 | (sc << 16))
            time.sleep(0.03)
            self.post(WM_KEYUP, vk, 1 | (sc << 16) | 0xC0000000)
        else:
            vk = {'Enter': 0x0d, 'Escape': 0x1b, ' ': 0x20}[name]
            sc = u32.MapVirtualKeyW(vk, 0)
            self.post(WM_KEYDOWN, vk, 1 | (sc << 16))
            time.sleep(0.03)
            self.post(WM_KEYUP, vk, 1 | (sc << 16) | 0xC0000000)
        time.sleep(0.05)

    def focus(self, on):
        if on:
            self.post(WM_ACTIVATEAPP, 1, 0); self.post(WM_ACTIVATE, 1, 0); self.post(WM_SETFOCUS, 0, 0)
        else:
            self.post(WM_ACTIVATE, 0, 0); self.post(WM_KILLFOCUS, 0, 0); self.post(WM_ACTIVATEAPP, 0, 0)

    def shot(self, name):
        png = os.path.join(self.out, name + '.png')
        if os.path.exists(png):
            os.remove(png)
        open(self.shotreq, 'w').write(png)
        for _ in range(200):
            if not os.path.exists(self.shotreq) and os.path.exists(png):
                break
            time.sleep(0.05)
        self.say('shot', name, 'ok' if os.path.exists(png) else 'MISSING', 'client %dx%d' % self.client())

    def close(self):
        if self.p.poll() is None:
            self.post(WM_CLOSE, 0, 0)
            try:
                self.p.wait(15)
            except subprocess.TimeoutExpired:
                self.p.kill()
                self.say('killed')
        self.say('exit', self.p.returncode)

    # ---- the game's answers ----
    def pump(self):
        """Reads the lines the game printed since the last call; returns the last state seen (None if no new one)."""
        chunk = self.reader.read()
        fresh = None
        if chunk:
            self.buf += chunk
            *lines, self.buf = self.buf.split('\n')
            for ln in lines:
                ln = ln.rstrip('\r')
                if ln.startswith('STATE '):
                    prev = (self.state or {}).get('active', True)
                    self.state = json.loads(ln[6:])
                    if self.state.get('active', True) != prev:
                        b = self.state.get('board') or {}
                        self.say('focus: active now', self.state.get('active'), 'at update', self.state['updates'], 'score',
                                 self.state.get('score'), 'moves', b.get('moves'), 'paused', b.get('paused'), 'muted', self.state.get('muted'))
                    self.pending_move = None
                    fresh = self.state
                elif ln.startswith('MOVE ') and self.state is not None:
                    body = ln[5:]
                    self.pending_move = None if body == 'null' else json.loads(body)
                    if self.pending_move is not None:
                        self.act(self.pending_move)
                elif ln.startswith('ERROR') or 'ERROR' in ln[:12]:
                    self.say('game:', ln)
        return fresh

    def signature(self, s):
        b = s.get('board') or {}
        t = b.get('tutorial') or {}
        return json.dumps([s.get('screen'), s.get('dialog'), b.get('state'), b.get('moves'), b.get('forged'), b.get('boards'),
                           t.get('step'), t.get('waiting'), b.get('piece'), (s.get('table') or {}).get('revealed'), s.get('ready')],
                          sort_keys=True)

    def act(self, mv):
        if not self.state.get('active', True):
            return          # a player cannot click a window that does not have the focus
        sig = self.signature(self.state) + mv['kind']
        now = time.time()
        if self.sent and self.sent[0] == sig and now - self.sent[1] < 1.5:
            return          # the input of this very move is on its way; the game has not yet shown its effect
        if self.sent and self.sent[0] == sig:
            self.stats['retries'] += 1
        self.sent = (sig, now)
        k = mv['kind']
        if k == 'place':
            self.stats['places'] += 1
        elif k == 'forge':
            self.stats['forges'] += 1
        else:
            self.stats['dialogs'] += 1
        for st in mv['input']:
            if 'move' in st:
                self.move(*st['move'])
                time.sleep(0.02)
            elif 'wait' in st:
                time.sleep(max(0.03, st['wait'] * 0.012))
            elif 'click' in st:
                self.click(*st['click'], right=(st.get('button') == 'right'))
            elif 'key' in st:
                self.key(st['key'])
        self.sent = (sig, time.time())

    def wait_state(self, pred, timeout, what, play=False):
        t0 = time.time()
        while time.time() - t0 < timeout:
            if self.p.poll() is not None:
                raise Failure('the game exited (%s) while waiting for %s' % (self.p.returncode, what))
            st = self.pump()
            if st is not None and pred(st):
                return st
            time.sleep(0.01)
        raise Failure('timeout (%.0f s) waiting for %s; last state %s' % (timeout, what, json.dumps(self.state)[:400]))

    # ---- steps ----
    def s_launch(self):
        self.wait_state(lambda s: s['screen'] == 'title' and s.get('ready'), 180, 'the title link')
        self.shot('title')
        # the title link is the bot's first move
        self.wait_state(lambda s: s['screen'] == 'menu', 30, 'the selector after the link', play=True)
        time.sleep(0.5)
        self.shot('menu')

    def press(self, getpos, what):
        s = self.state
        x, y = getpos(s)
        self.move(x, y)
        time.sleep(0.12)
        self.click(x, y)
        time.sleep(0.3)
        self.pump()
        self.say('clicked', what, x, y)

    def s_game(self, mode, skill, limit):
        self.wait_state(lambda s: s['screen'] == 'menu', 60, 'the selector')
        self.press(lambda s: s['selector']['skill%s' % skill], 'skill %s' % skill)
        self.press(lambda s: s['selector'][mode], mode)
        self.shot('selector-%s-%s' % (mode, skill))
        self.press(lambda s: s['selector']['start'], 'start')
        self.wait_state(lambda s: s['screen'] == 'game', 20, 'the board')
        self.say('game %s %s started: %s' % (mode, skill, json.dumps(self.state['board']['piece'])))
        self.shot('game-%s-%s-start' % (mode, skill))
        self.play_until_menu('game-%s-%s' % (mode, skill), limit)

    def play_until_menu(self, tag, limit):
        t0 = time.time()
        last_shot = t0
        seen = {}
        while time.time() - t0 < limit:
            if self.p.poll() is not None:
                raise Failure('the game exited (%s) in %s' % (self.p.returncode, tag))
            st = self.pump()
            if st is not None:
                if st['screen'] == 'menu':
                    self.say('%s back at the selector after %.0f s' % (tag, time.time() - t0), json.dumps(self.stats))
                    return
                b = st.get('board') or {}
                for key in ('over',):
                    pass
                name = (b.get('state'), st.get('dialog'), st['screen'])
                if name not in seen and (b.get('state') in ('cleared', 'ending', 'gameover') or st['screen'] in ('highscores',) or st.get('dialog')):
                    seen[name] = 1
                    self.shot('%s-%s-%s-%s' % (tag, b.get('state'), st['screen'], st.get('dialog') or 'x'))
                tut = (b.get('tutorial') or {})
                if tut and (tut.get('step'), tut.get('waiting')) != self.last_tut:
                    self.last_tut = (tut.get('step'), tut.get('waiting'))
                    self.shot('%s-step%02d%s' % (tag, tut['step'], 'w' if tut.get('waiting') else ''))
                if self.stats['places'] // 60 != self.last_places // 60:
                    self.last_places = self.stats['places']
                    self.shot('%s-play%04d' % (tag, self.stats['places']))
                if time.time() - last_shot > 60:
                    last_shot = time.time()
                    self.say(tag, 'score', st.get('score'), 'level', st.get('level'), 'forge', st.get('forge'), json.dumps(self.stats))
            time.sleep(0.005)
        raise Failure('%s: the selector did not come back in %d s; last state %s' % (tag, limit, json.dumps(self.state)[:500]))

    def s_tutorial(self, limit):
        self.wait_state(lambda s: s['screen'] == 'menu', 60, 'the selector')
        self.press(lambda s: s['selector']['tutorial'], 'tutorial')
        self.wait_state(lambda s: s['screen'] == 'game', 20, 'the tutorial board')
        self.play_until_menu('tutorial', limit)

    def s_screenmode(self, windowed):
        self.wait_state(lambda s: s['screen'] == 'menu', 60, 'the selector')
        if self.state['windowed'] == windowed:
            self.say('already', 'windowed' if windowed else 'fullscreen')
            return
        self.press(lambda s: s['panel']['options'], 'options')
        self.wait_state(lambda s: s.get('dialog') == 'options', 10, 'the options dialog')
        self.press(lambda s: s['options']['fullscreen'], 'fullscreen box')
        self.shot('options-%s' % ('windowed' if windowed else 'fullscreen'))
        self.press(lambda s: s['options']['done'], 'done')
        self.wait_state(lambda s: s['screen'] == 'menu' and s['windowed'] == windowed, 20, 'the screen mode to change')
        time.sleep(1.0)
        self.say('screen mode now', 'windowed' if windowed else 'fullscreen', 'client %dx%d' % self.client())
        self.shot('menu-%s' % ('windowed' if windowed else 'fullscreen'))

    def s_quit(self):
        self.wait_state(lambda s: s['screen'] == 'menu', 60, 'the selector')
        self.press(lambda s: s['panel']['quit'], 'quit')
        self.wait_state(lambda s: s.get('dialog') == 'quit', 10, 'the quit question')
        self.shot('quit')
        # the question's No first: the game goes on at the selector
        self.press(lambda s: s['buttons']['no'], 'No')
        self.wait_state(lambda s: s.get('dialog') != 'quit', 10, 'the quit question to close')
        self.say('No closed the quit question; the selector is', 'up' if self.state['screen'] == 'menu' else 'GONE')
        if self.state['screen'] != 'menu':
            raise Failure('the selector did not stay after No')
        # then Yes: the game leaves by itself
        self.press(lambda s: s['panel']['quit'], 'quit again')
        self.wait_state(lambda s: s.get('dialog') == 'quit', 10, 'the quit question again')
        self.press(lambda s: s['buttons']['yes'], 'Yes')
        t0 = time.time()
        while self.p.poll() is None and time.time() - t0 < 20:
            self.pump()
            time.sleep(0.05)
        if self.p.poll() is None:
            raise Failure('the game did not exit after Yes')
        self.say('the game left by itself after Yes, exit code', self.p.returncode)


def main():
    args = sys.argv[1:]
    seed, updates, game_seconds = 1, 120000, 420
    lines = []
    extra = []
    steps = []
    out = args.pop(0)
    while args:
        a = args.pop(0)
        if a == '--seed':
            seed = int(args.pop(0))
        elif a == '--updates':
            updates = int(args.pop(0))
        elif a == '--game-seconds':
            game_seconds = int(args.pop(0))
        elif a == '--line':
            lines.append(args.pop(0))
        elif a == '--modsdir':
            extra.append('-modsdir=' + os.path.abspath(args.pop(0)))
        else:
            steps.append([a])
            n = {'game': 2, 'shot': 1, 'wait': 1, 'focus': 1}.get(a, 0)
            for _ in range(n):
                steps[-1].append(args.pop(0))
    g = Game(out, seed=seed, updates=updates, lines=lines, extra=extra)
    code = 0
    try:
        for st in steps:
            g.say('STEP', ' '.join(st))
            c = st[0]
            if c == 'launch':
                g.s_launch()
            elif c == 'game':
                g.s_game(st[1], int(st[2]), game_seconds)
            elif c == 'tutorial':
                g.s_tutorial(game_seconds)
            elif c == 'windowed':
                g.s_screenmode(True)
            elif c == 'fullscreen':
                g.s_screenmode(False)
            elif c == 'shot':
                g.shot(st[1])
            elif c == 'wait':
                end = time.time() + float(st[1])
                while time.time() < end:
                    g.pump()
                    time.sleep(0.01)
            elif c == 'focus':
                g.focus(st[1] == 'regain')
            elif c == 'quit':
                g.s_quit()
            else:
                raise Failure('unknown step %s' % c)
    except Failure as e:
        g.say('FAILED:', e)
        print('FAILED:', e)
        code = 1
    finally:
        g.say('stats', json.dumps(g.stats))
        json.dump(dict(stats=g.stats, state=g.state, failed=code), open(os.path.join(g.out, 'result.json'), 'w'))
        g.close()
    return code


if __name__ == '__main__':
    sys.exit(main())
