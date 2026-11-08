"""Plays the port with real input only (CLAUDE.md, "Done means identical"): window messages posted to the game's own
window, as a player's mouse and keys give them, on the real clock. The moves come from the port's own bot: the game's
-script asks "state" and "move" every few updates (docs/TEST_COMMANDS.md), and this script sends a MOVE's input when it
follows the STATE of a game that waits for one. Run it under rundesk only (the window, the sound and the priority stay
out of the user's way):

    build/native/rundesk.exe python games/wordharmony/tools/realplay/play.py <steps file> <out dir> [port arguments]

The steps file has one step a line ('#' starts a comment):

    wait <seconds>          let the game run
    click <x> <y>           a move there, a left press and release, posted together
    rclick <x> <y>          the same with the right button
    key <vk>                a key press and release (a virtual-key code), with its WM_CHAR for letters
    text <letters>          a key for each letter
    focus lose|gain         the window loses or regains the focus
    shot <name>             the game's own screenshot (BYOPOP_SHOT_REQUEST) in <out dir>/<name>.png
    play <seconds> [until]  the bot's moves, until the state's screen is no longer "game" or the help, level panel
                            and loading screens the bot answers, or <seconds>; "until gameover" plays on through the
                            level panels until the game is over, "until nextlevel" until the next level's game
    end                     closes the window and waits for the game to exit

The log is <out dir>/play.log: each move with the state's score, level and water, and the last state.
"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import subprocess
import sys
import time

u32 = ctypes.WinDLL('user32', use_last_error=True)
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u32.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
WM_CLOSE, WM_MOUSEMOVE = 0x10, 0x200
WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP = 0x201, 0x202, 0x204, 0x205
WM_KEYDOWN, WM_KEYUP, WM_CHAR = 0x100, 0x101, 0x102
WM_ACTIVATE, WM_SETFOCUS, WM_KILLFOCUS, WM_ACTIVATEAPP = 0x6, 0x7, 0x8, 0x1c
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
EXE = os.path.join(ROOT, 'build', 'native', 'wordharmony.exe')
UPDATE_S = 0.033  # the game's frame period
ASK_EVERY = 20    # updates between the script's state and move


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


def main():
    steps_file, out = sys.argv[1], sys.argv[2]
    extra = sys.argv[3:]
    os.makedirs(out, exist_ok=True)
    log = open(os.path.join(out, 'play.log'), 'w')
    req = os.path.abspath(os.path.join(out, 'shot.req'))
    script = os.path.join(out, 'asks.txt')
    with open(script, 'w') as f:  # state and move every ASK_EVERY updates for three hours of play
        for n in range(200, 330000, ASK_EVERY):
            f.write('%d state\n%d move\n' % (n, n))
    user = os.path.join(out, 'user')
    os.makedirs(user, exist_ok=True)
    cmd = [EXE, '-gamedir=' + os.path.join(ROOT, 'assets', 'wordharmony'), '-userdir=' + user, '-script=' + script] + extra
    env = dict(os.environ, BYOPOP_SHOT_REQUEST=req, BYOPOP_POSTED_INPUT='1')
    stdout = os.path.join(out, 'stdout.txt')
    proc = subprocess.Popen(cmd, env=env, stdout=open(stdout, 'w'), stderr=open(os.path.join(out, 'stderr.txt'), 'w'))
    hwnd = None
    for _ in range(300):
        hwnd = find_window(proc.pid)
        if hwnd or proc.poll() is not None:
            break
        time.sleep(0.1)
    if not hwnd:
        print('no window; exit', proc.poll(), file=log)
        proc.kill()
        return 1
    held = [0]
    src = open(stdout)
    buf = ['']

    def point(x, y):
        r = wt.RECT()
        u32.GetClientRect(hwnd, ctypes.byref(r))
        w, h = r.right - r.left, r.bottom - r.top
        k = min(w / 800, h / 600)  # the picture fitted and centred (letterboxed in full screen)
        cx, cy = round((w - 800 * k) / 2 + x * k), round((h - 600 * k) / 2 + y * k)
        return (cy << 16) | (cx & 0xffff)

    def move(x, y):
        u32.PostMessageW(hwnd, WM_MOUSEMOVE, held[0], point(x, y))

    def click(x, y, right=False):
        move(x, y)
        down, up, flag = (WM_RBUTTONDOWN, WM_RBUTTONUP, 2) if right else (WM_LBUTTONDOWN, WM_LBUTTONUP, 1)
        u32.PostMessageW(hwnd, down, flag, point(x, y))
        u32.PostMessageW(hwnd, up, 0, point(x, y))

    def key(vk):
        sc = u32.MapVirtualKeyW(vk, 0) << 16  # SDL takes the key from the scan code in lParam
        u32.PostMessageW(hwnd, WM_KEYDOWN, vk, sc | 1)
        if 0x41 <= vk <= 0x5a:
            u32.PostMessageW(hwnd, WM_CHAR, vk + 32, sc | 1)
        u32.PostMessageW(hwnd, WM_KEYUP, vk, sc | 0xc0000001)

    def read_lines():
        chunk = src.read()
        if not chunk:
            return []
        buf[0] += chunk
        *done, buf[0] = buf[0].split('\n')
        return done

    def send(mv):
        for st in mv.get('input', []):
            if 'move' in st:
                move(st['move']['x'], st['move']['y'])
            elif 'down' in st:
                move(st['down']['x'], st['down']['y'])
                u32.PostMessageW(hwnd, WM_LBUTTONDOWN, 1, point(st['down']['x'], st['down']['y']))
                held[0] = 1
            elif 'up' in st:
                move(st['up']['x'], st['up']['y'])
                u32.PostMessageW(hwnd, WM_LBUTTONUP, 0, point(st['up']['x'], st['up']['y']))
                held[0] = 0
            elif 'click' in st:
                c = st['click']
                click(c['x'], c['y'], c.get('button') == 'right')
            elif 'key' in st:
                key(st['key'])
            elif 'wait' in st:
                time.sleep(st['wait'] * UPDATE_S)

    def play(limit, until):
        t0, state, last, made, after, first = time.time(), None, None, 0, -1, None
        read_lines()  # only what the game says from now on
        while time.time() - t0 < limit and proc.poll() is None:
            lines = read_lines()
            if not lines:
                time.sleep(0.03)
                continue
            for ln in lines:
                if ln.startswith('STATE '):
                    state = last = json.loads(ln[6:])
                    if first is None and last.get('screen') == 'game':
                        first = last.get('level', 0)
                elif ln.startswith('MOVE ') and state is not None:
                    st, state = state, None
                    if st['frame'] < after:
                        continue
                    mv = json.loads(ln[5:])
                    if mv is None:
                        continue
                    if mv.get('what') != 'wait':
                        made += 1
                        print('%.1f move %d %s %s level %s score %s water %s' % (time.time() - t0, made, mv.get('what'),
                              mv.get('word', ''), st.get('level'), st.get('score'), st.get('water')), file=log, flush=True)
                    send(mv)
                    after = st['frame'] + sum(s.get('wait', 0) for s in mv.get('input', [])) + mv.get('updates', 1)
            if last is not None and last.get('over') and until != 'gameover':
                break
            if until == 'nextlevel' and last is not None and first is not None and last.get('level', 0) > first                     and last.get('screen') == 'game':
                break
            if last is not None and last.get('screen') in ('gameover', 'nameentry', 'highscores', 'menu', 'congrats'):
                if made:
                    break
        print('play: %d moves in %.0f s, last %s' % (made, time.time() - t0,
              json.dumps({k: v for k, v in (last or {}).items() if k != 'balls'})), file=log, flush=True)

    for n, line in enumerate(open(steps_file)):
        line = line.split('#')[0].strip()
        if not line:
            continue
        if proc.poll() is not None:
            print('game exited (%d) before line %d: %s' % (proc.returncode, n + 1, line), file=log)
            break
        c, v = line.split()[0], line.split()[1:]
        print('%.2f %s' % (time.time(), line), file=log, flush=True)
        if c == 'wait':
            time.sleep(float(v[0]))
        elif c in ('click', 'rclick'):
            click(float(v[0]), float(v[1]), c == 'rclick')
            time.sleep(0.2)
        elif c == 'key':
            key(int(v[0]))
            time.sleep(0.1)
        elif c == 'text':
            for ch in v[0].upper():
                key(ord(ch))
                time.sleep(0.15)
        elif c == 'focus':
            if v[0] == 'lose':
                u32.PostMessageW(hwnd, WM_ACTIVATE, 0, 0)
                u32.PostMessageW(hwnd, WM_KILLFOCUS, 0, 0)
                u32.PostMessageW(hwnd, WM_ACTIVATEAPP, 0, 0)
            else:
                u32.PostMessageW(hwnd, WM_ACTIVATEAPP, 1, 0)
                u32.PostMessageW(hwnd, WM_ACTIVATE, 1, 0)
                u32.PostMessageW(hwnd, WM_SETFOCUS, 0, 0)
        elif c == 'shot':
            open(req, 'w').write(os.path.abspath(os.path.join(out, v[0] + '.png')))
            time.sleep(0.6)
        elif c == 'play':
            play(float(v[0]), v[1] if len(v) > 1 else '')
        elif c == 'end':
            u32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
            try:
                proc.wait(30)
            except subprocess.TimeoutExpired:
                proc.kill()
            break
    if proc.poll() is None:
        proc.kill()
    print('exit %s' % proc.poll(), file=log, flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
