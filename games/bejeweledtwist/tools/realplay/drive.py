"""Plays the port with real input only: window messages posted to its own window, as a player's mouse and keys
would give them (CLAUDE.md, "Done means identical": autoplay and script commands never pass through that path).

    build/native/rundesk.exe python games/bejeweledtwist/tools/realplay/drive.py <steps file> <out dir> -- <exe> <args...>

Run it under rundesk only: the game's window is then on rundesk's private desktop, and so is this script, which finds
the window by the game's process id. The steps file has one step a line ('#' starts a comment):

    wait <seconds>            let the game run
    ready <seconds>           wait until the game updates (it takes a shot asked for), at most that long
    move <x> <y>              the mouse to (x, y) in the game's own coordinates (its width and height: --size)
    click <x> <y> [name]      a move there, a left button press and release (and that shot asked for at once)
    drag <x1> <y1> <x2> <y2>  press at the first point, move in steps to the second, release
    key <name>                a key press and release: escape, return, space, left, up, right, down, f1..f12, a letter
    focus lose|gain           the window loses or regains the focus (WM_ACTIVATE / WM_KILLFOCUS, WM_SETFOCUS)
    minimize, restore         the window is minimized or restored (WM_SYSCOMMAND), as Alt-Tab does in full screen
    shot <name>               the game's own screenshot (BYOPOP_SHOT_REQUEST) in <out dir>/<name>.png
    solve <n> <x0> <y0> <cell> [s]  n moves of a simple player: a shot, each gem's colour read at its square's middle
                              (the 8x8 board's top left at x0, y0, squares cell wide), and a hover and a click on a
                              square whose clockwise turn makes a line of 3; s seconds between moves (3)
    end                       closes the window (WM_CLOSE) and waits for the game to exit

With --follow the script waits for steps appended to the file until "end" (for a session that plays step by
step). The game's size comes from --size WxH before the steps file (default 800x600); a click at game (x, y) goes to the
client point scaled by the client area's size, so it also hits a full-screen window, letterboxed or not, given
--box X,Y,W,H for the drawn area in client pixels (default: the whole client area).
"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import subprocess
import sys
import time

from PIL import Image

u32 = ctypes.WinDLL('user32', use_last_error=True)
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]

WM_CLOSE, WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP = 0x10, 0x200, 0x201, 0x202
WM_KEYDOWN, WM_KEYUP, WM_CHAR = 0x100, 0x101, 0x102
WM_ACTIVATE, WM_SETFOCUS, WM_KILLFOCUS, WM_ACTIVATEAPP = 0x6, 0x7, 0x8, 0x1c
VK = {'escape': 0x1b, 'return': 0x0d, 'space': 0x20, 'left': 0x25, 'up': 0x26, 'right': 0x27, 'down': 0x28,
      'tab': 0x09, 'backspace': 0x08}
for i in range(1, 13):
    VK['f%d' % i] = 0x6f + i


title = None  # --title: a window of any process whose title has this text (a game started under a debugger)


def window_text(hwnd):
    buf = ctypes.create_unicode_buffer(256)
    u32.GetWindowTextW(hwnd, buf, 256)
    return buf.value


def find_window(pid):
    found = []
    PROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)

    def cb(hwnd, _):
        p = wt.DWORD()
        u32.GetWindowThreadProcessId(hwnd, ctypes.byref(p))
        if u32.IsWindowVisible(hwnd) and (p.value == pid or (title and title.lower() in window_text(hwnd).lower())):
            found.append(hwnd)
        return True
    u32.EnumWindows(PROC(cb), 0)
    return found[0] if found else None


def client_size(hwnd):
    r = wt.RECT()
    u32.GetClientRect(hwnd, ctypes.byref(r))
    return r.right - r.left, r.bottom - r.top


def gem_kind(img, cx, cy, r):
    """A gem's colour from the pixels around its centre: 'w' (white, unsaturated), a hue's letter, or '?'."""
    hues = []
    n = 0
    for y in range(int(cy - r), int(cy + r), 2):
        for x in range(int(cx - r), int(cx + r), 2):
            red, grn, blu = img.getpixel((x, y))[:3]
            n += 1
            mx, mn = max(red, grn, blu), min(red, grn, blu)
            if mx < 70 or mx - mn < 0.4 * mx:
                continue
            if mx == red:
                h = (60 * (grn - blu) / (mx - mn)) % 360
            elif mx == grn:
                h = 60 * (blu - red) / (mx - mn) + 120
            else:
                h = 60 * (red - grn) / (mx - mn) + 240
            hues.append(h)
    if len(hues) < 0.25 * n:
        return 'w'
    hues.sort()
    h = hues[len(hues) // 2]
    for limit, kind in ((15, 'r'), (40, 'o'), (75, 'y'), (165, 'g'), (255, 'b'), (330, 'p'), (360, 'r')):
        if h < limit:
            return kind
    return '?'


def find_twist(grid):
    """A 2x2 square (row, column of its top left) whose clockwise turn makes a line of 3, the lowest first."""
    rows, cols = len(grid), len(grid[0])
    for r in range(rows - 2, -1, -1):
        for c in range(cols - 1):
            g = [row[:] for row in grid]
            tl, tr, br, bl = g[r][c], g[r][c + 1], g[r + 1][c + 1], g[r + 1][c]
            g[r][c], g[r][c + 1], g[r + 1][c + 1], g[r + 1][c] = bl, tl, tr, br
            for (y, x) in ((r, c), (r, c + 1), (r + 1, c), (r + 1, c + 1)):
                k = g[y][x]
                if k == '?':
                    continue
                for dy, dx in ((0, 1), (1, 0)):
                    run = 1
                    for s in (1, -1):
                        yy, xx = y + dy * s, x + dx * s
                        while 0 <= yy < rows and 0 <= xx < cols and g[yy][xx] == k:
                            run += 1
                            yy, xx = yy + dy * s, xx + dx * s
                    if run >= 3:
                        return r, c
    return None


def main():
    args = sys.argv[1:]
    size, box, follow = (800, 600), None, False
    while args and args[0].startswith('--'):
        if args[0] == '--title':
            global title
            title = args[1]
            args = args[2:]
            continue
        if args[0] == '--follow':
            follow = True
            args = args[1:]
            continue
        if args[0] == '--size':
            size = tuple(int(v) for v in args[1].split('x'))
        elif args[0] == '--box':
            box = tuple(int(v) for v in args[1].split(','))
        args = args[2:]
    steps_file, out = args[0], args[1]
    cmd = args[args.index('--') + 1:]
    os.makedirs(out, exist_ok=True)
    log = open(os.path.join(out, 'drive.log'), 'w')
    req = os.path.abspath(os.path.join(out, 'shot.req'))
    env = dict(os.environ, BYOPOP_SHOT_REQUEST=req, BYOPOP_POSTED_INPUT="1")
    proc = subprocess.Popen(cmd, env=env, stdout=open(os.path.join(out, 'stdout.txt'), 'w'),
                            stderr=open(os.path.join(out, 'stderr.txt'), 'w'))
    hwnd = None
    for _ in range(3000):
        hwnd = find_window(proc.pid)
        if hwnd or proc.poll() is not None:
            break
        time.sleep(0.1)
    if not hwnd:
        print('no window; exit', proc.poll(), file=log)
        proc.kill()
        return 1
    print('window %r' % window_text(hwnd), file=log, flush=True)
    held = [False]

    def point(x, y):
        w, h = client_size(hwnd)
        if box:
            bx, by, bw, bh = box
        else:  # the game's picture fitted into the client area, centred (letterboxed in full screen)
            k = min(w / size[0], h / size[1])
            bw, bh = size[0] * k, size[1] * k
            bx, by = (w - bw) / 2, (h - bh) / 2
        cx, cy = round(bx + x * bw / size[0]), round(by + y * bh / size[1])
        return (cy << 16) | (cx & 0xffff)

    def move(x, y):
        u32.PostMessageW(hwnd, WM_MOUSEMOVE, 1 if held[0] else 0, point(x, y))

    def lines():  # the steps file's lines; with --follow, lines appended later too, until "end"
        f = open(steps_file)
        while True:
            line = f.readline()
            if line.endswith('\n') or (line and not follow):
                yield line
                if line.split('#')[0].strip() == 'end':
                    return
            elif not follow:
                return
            else:
                if line:
                    f.seek(f.tell() - len(line))
                if proc.poll() is not None:
                    return
                time.sleep(0.2)

    for n, line in enumerate(lines()):
        line = line.split('#')[0].strip()
        if not line:
            continue
        if not u32.IsWindow(hwnd):
            hwnd = find_window(proc.pid) or hwnd
        if proc.poll() is not None:
            print('game exited (%d) before line %d: %s' % (proc.returncode, n + 1, line), file=log)
            break
        f = line.split()
        c, v = f[0], f[1:]
        print('%.2f %s' % (time.time(), line), file=log, flush=True)
        if c == 'wait':
            time.sleep(float(v[0]))
        elif c == 'move':
            move(float(v[0]), float(v[1]))
            time.sleep(0.05)
        elif c == 'click':
            x, y = float(v[0]), float(v[1])
            # one batch: SDL answers the move with TrackMouseEvent, whose WM_MOUSELEAVE (the real cursor is not over
            # the window on rundesk's desktop) moves its mouse to GetCursorPos and drops the window's mouse focus;
            # posted together, the press and release are in the queue before that message
            move(x, y)
            u32.PostMessageW(hwnd, WM_LBUTTONDOWN, 1, point(x, y))
            u32.PostMessageW(hwnd, WM_LBUTTONUP, 0, point(x, y))
            if len(v) > 2:  # click x y <name>: a shot asked for at once, for what follows the click within 0.1 s
                png = os.path.abspath(os.path.join(out, v[2] + '.png'))
                open(req, 'w').write(png)
            time.sleep(0.2)
        elif c == 'drag':
            x1, y1, x2, y2 = (float(a) for a in v)
            move(x1, y1)
            time.sleep(0.15)
            u32.PostMessageW(hwnd, WM_LBUTTONDOWN, 1, point(x1, y1))
            held[0] = True
            for i in range(1, 11):
                move(x1 + (x2 - x1) * i / 10, y1 + (y2 - y1) * i / 10)
                time.sleep(0.03)
            u32.PostMessageW(hwnd, WM_LBUTTONUP, 0, point(x2, y2))
            held[0] = False
            time.sleep(0.1)
        elif c == 'key':
            k = v[0].lower()
            vk = VK.get(k, ord(k.upper()) if len(k) == 1 else None)
            u32.PostMessageW(hwnd, WM_KEYDOWN, vk, 1)
            time.sleep(0.08)
            u32.PostMessageW(hwnd, WM_KEYUP, vk, 0xC0000001)
            time.sleep(0.1)
        elif c == 'focus':
            if v[0] == 'lose':
                u32.PostMessageW(hwnd, WM_ACTIVATE, 0, 0)
                u32.PostMessageW(hwnd, WM_KILLFOCUS, 0, 0)
                u32.PostMessageW(hwnd, WM_ACTIVATEAPP, 0, 0)
            else:
                u32.PostMessageW(hwnd, WM_ACTIVATEAPP, 1, 0)
                u32.PostMessageW(hwnd, WM_ACTIVATE, 1, 0)
                u32.PostMessageW(hwnd, WM_SETFOCUS, 0, 0)
            time.sleep(0.2)
        elif c in ('minimize', 'restore'):  # as the taskbar or Alt-Tab does it
            u32.PostMessageW(hwnd, 0x112, 0xF020 if c == 'minimize' else 0xF120, 0)  # WM_SYSCOMMAND SC_MINIMIZE/RESTORE
            time.sleep(0.5)
        elif c == 'shot':
            png = os.path.abspath(os.path.join(out, v[0] + '.png'))
            if os.path.exists(png):
                os.remove(png)
            open(req, 'w').write(png)
            for _ in range(100):
                if not os.path.exists(req) and os.path.exists(png):
                    break
                time.sleep(0.05)
            print('  shot %s %s (client %dx%d)' % (v[0], 'ok' if os.path.exists(png) else 'MISSING',
                                                 *client_size(hwnd)), file=log, flush=True)
        elif c == 'solve':  # solve <moves> <x0> <y0> <cell> [seconds between moves]
            moves, x0, y0, cell = int(v[0]), float(v[1]), float(v[2]), float(v[3])
            pause = float(v[4]) if len(v) > 4 else 3.0
            png = os.path.abspath(os.path.join(out, '_solve.png'))
            for m in range(moves):
                if proc.poll() is not None:
                    break
                if os.path.exists(png):
                    os.remove(png)
                open(req, 'w').write(png)
                for _ in range(100):
                    if not os.path.exists(req) and os.path.exists(png):
                        break
                    time.sleep(0.05)
                try:
                    img = Image.open(png).convert('RGB')
                    img.load()
                except OSError:
                    time.sleep(1)
                    continue
                grid = [[gem_kind(img, x0 + cell * (j + 0.5), y0 + cell * (i + 0.5), cell * 0.22) for j in range(8)]
                        for i in range(8)]
                t = find_twist(grid)
                print('  %d %s -> %s' % (m, '/'.join(''.join(row) for row in grid), t), file=log, flush=True)
                if t is None:
                    time.sleep(pause)
                    continue
                x, y = x0 + cell * (t[1] + 1), y0 + cell * (t[0] + 1)
                move(x - 8, y - 8)  # the board takes the square under the cursor in its update: hover first
                time.sleep(0.3)
                move(x, y)
                time.sleep(0.3)
                move(x, y)
                u32.PostMessageW(hwnd, WM_LBUTTONDOWN, 1, point(x, y))
                u32.PostMessageW(hwnd, WM_LBUTTONUP, 0, point(x, y))
                time.sleep(pause)
        elif c == 'play':  # play <seconds> [shot every n moves]: the port's own bot chooses, this sends its input
            # The game's -script asks "state" and then "move" at the same update, every so often; a MOVE that follows a
            # STATE of an idle board is the twist to make now (docs/TEST_COMMANDS.md, "The bot behind move"). It ends
            # when a STATE shows the game left (another screen, or the game over), or after <seconds>.
            limit = float(v[0])
            every = int(v[1]) if len(v) > 1 else 0
            src = open(os.path.join(out, 'stdout.txt'))
            src.seek(0, 2)
            state, fresh, made, t0, buf, last = None, False, 0, time.time(), '', None
            while time.time() - t0 < limit and proc.poll() is None:
                chunk = src.read()
                if not chunk:
                    time.sleep(0.05)
                    continue
                buf += chunk
                *done, buf = buf.split('\n')
                for ln in done:
                    if ln.startswith('STATE '):
                        state = json.loads(ln[6:])
                        last = state
                        fresh = state.get('screen') == 'game' and not state.get('busy') and not state.get('over')
                    elif ln.startswith('MOVE ') and fresh:
                        fresh = False
                        mv = json.loads(ln[5:])
                        if mv is None:
                            continue
                        for step in mv['input']:
                            if 'move' in step:
                                x, y = step['move']
                                move(x - 8, y - 8)  # the board takes the square under the cursor in its update
                                time.sleep(0.25)
                                move(x, y)
                                time.sleep(0.25)
                            elif 'click' in step:
                                x, y = step['click']
                                move(x, y)
                                u32.PostMessageW(hwnd, WM_LBUTTONDOWN, 1, point(x, y))
                                u32.PostMessageW(hwnd, WM_LBUTTONUP, 0, point(x, y))
                        made += 1
                        what = ('r%d c%d%s' % (mv['row'], mv['col'], ' bomb' if mv['bomb'] else '')
                                if mv['kind'] == 'twist' else mv['kind'])
                        print('  move %d %s score %s' % (made, what, state.get('score')), file=log, flush=True)
                        if every and made % every == 0:
                            open(req, 'w').write(os.path.abspath(os.path.join(out, 'play%03d.png' % made)))
                if last is not None and last.get('screen') != 'game' and made:
                    break
                if last is not None and made and last.get('over'):
                    break
            summary = {k: last.get(k) for k in ('screen', 'mode', 'level', 'score', 'moves', 'timer', 'over')
                       if last and k in last}
            print('  play: %d moves in %.0f s, last %s' % (made, time.time() - t0, json.dumps(summary)), file=log,
                  flush=True)
        elif c == 'ready':  # until the game is updating (it answers a shot request), at most <seconds>
            png = os.path.abspath(os.path.join(out, '_ready.png'))
            t0 = time.time()
            while time.time() - t0 < float(v[0]) and proc.poll() is None:
                open(req, 'w').write(png)
                time.sleep(1)
                if not os.path.exists(req):
                    break
            if os.path.exists(req):
                os.remove(req)
            print('  ready after %.1f s' % (time.time() - t0), file=log, flush=True)
        elif c == 'end':
            u32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
            try:
                proc.wait(30)
            except subprocess.TimeoutExpired:
                pass
        else:
            print('unknown step', line, file=log)
    if proc.poll() is None:
        proc.kill()
        print('killed', file=log)
    print('exit', proc.returncode, file=log)
    return 0


if __name__ == '__main__':
    sys.exit(main())
