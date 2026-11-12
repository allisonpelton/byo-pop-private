"""Plays the Cosmic Bugs port with real input only (CLAUDE.md, "Done means identical"): every click and key goes through
-realinput into SDL's own queue and takes a hand's path from there (docs/REAL_INPUT.md). The port's own bot chooses
every shot (the `move` test command, docs/TEST_COMMANDS.md); this script sends the input the move names over the live
channel (-commands) and checks what the game's `state` says next.

    build/native/rundesk.exe python games/cosmicbugs/tools/play/drive.py <out dir> [--games manual-full,manual-window,
        auto-full,auto-window,manual-win1x,auto-win2x] [--maxtime S] [--exe E] [--fast]

(win1x and win2x start in a window, 800x600 or the framework's default 1600x1200, where the window's own surface shows the
screen, and at the end go to full screen and back with Alt+Enter.)

The player's modes are the shooter's, manual and auto: the game file has no difficulty item (the code of its setting is
in the exe and the setting stays "normal"). A game of each, in the full screen the game starts in and in a window
(Alt+Enter), each on a fresh user folder: the title, Options (the shooter chosen there, back), Start Game, the first
run's instructions page, Play, then boards on the bot's shots until the game is over (all lives gone), the click that
leaves the game over, the Hall of Fame, Back to Main Menu, the title. On the first board of each game: Esc pauses and
Enter resumes, and a focus loss freezes the game (the original stops its updates while its window is inactive) and a
regain starts it again. A game still going after --maxtime seconds (default 480) is ended by the `lose` test command,
and says so. Real time, as the bot's shots are chosen from the board as it is: with -fast the board runs thousands of
updates between a move and its answer. The log is <out dir>/<game>/drive.log, the game's own log game.log, its
screenshots *.png; the last line of the log, and of stdout, is "RESULT ok" or "RESULT FAILED: <why>". Nothing here
clicks a link button.
"""
import json
import os
import subprocess
import sys
import threading
import time

# the title's menu (test/tours/menus.txt: the items' centres)
TITLE = {'help': (175, 269), 'start': (175, 313), 'options': (175, 357), 'halloffame': (175, 401),
         'instructions': (175, 444), 'credits': (175, 489), 'quit': (175, 532), 'user': (175, 590)}
OPTIONS = {'shooter': (400, 418), 'back': (400, 520)}  # the options' Shooter Mode item and Back to Main Menu
PLAY = (400, 563)       # the instructions page's "Click here to play"
HALL_BACK = (400, 543)  # the Hall of Fame's Back to Main Menu
NATIVE_KEYS = {'Enter': 'Return'}  # a move's key names are the page's (DOM); SDL's differ only here


class Failed(Exception):
    pass


class Game:
    def __init__(self, out, exe, fast, windowed_start=None, extra_args=None):
        self.out = os.path.abspath(out)
        os.makedirs(self.out, exist_ok=True)
        extra = list(extra_args or [])  # more arguments for the exe (a test's -modsdir, -livesplitlog)
        if windowed_start:
            # a window from the start (cp.cfg's fullscreen="0"), the window's own surface showing the screen: at 1x the game's size
            # (-windowscale=1, the original's window) or at the 2x the framework starts at
            os.makedirs(os.path.join(self.out, 'user'), exist_ok=True)
            with open(os.path.join(self.out, 'user', 'cp.cfg'), 'w') as cfg:
                cfg.write('<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n')
            if windowed_start == 'win1x':
                extra.append('-windowscale=1')
        self.log = open(os.path.join(self.out, 'drive.log'), 'a')
        self.cf = os.path.join(self.out, 'commands.txt')
        empty = os.path.join(self.out, 'realinput.txt')
        open(self.cf, 'w').close()
        open(empty, 'w').close()
        cmd = [exe, '-gamedir=' + os.path.abspath('assets/cosmicbugs'), '-userdir=' + os.path.join(self.out, 'user'),
               '-commands=' + self.cf, '-realinput=' + empty] + extra + (['-fast'] if fast else [])
        self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.lines, self.cond = [], threading.Condition()
        self.gamelog = open(os.path.join(self.out, 'game.log'), 'w')
        threading.Thread(target=self.reader, daemon=True).start()

    def say(self, text):
        print('%8.2f %s' % (time.time() - T0, text), file=self.log, flush=True)

    def reader(self):
        for line in self.proc.stdout:
            self.gamelog.write(line)
            self.gamelog.flush()
            line = line.rstrip('\n')
            if line.startswith('INFO: '):
                line = line[6:]
            with self.cond:
                self.lines.append(line)
                self.cond.notify_all()
        with self.cond:
            self.lines.append(None)
            self.cond.notify_all()

    def ask(self, line, timeout=30.0):
        """Sends one line and returns its answer: the STATE or MOVE value for state and move, else the OK line"""
        with self.cond:
            start = len(self.lines)
        with open(self.cf, 'a', newline='\n') as f:
            f.write(line + '\n')
        word = line.split()[0]
        end = time.time() + timeout
        i = start
        with self.cond:
            while True:
                while i < len(self.lines):
                    ln = self.lines[i]
                    i += 1
                    if ln is None:
                        raise Failed('the game exited at "%s"' % line)
                    if line == 'state' and ln.startswith('STATE '):
                        return json.loads(ln[6:])
                    if line == 'move' and ln.startswith('MOVE '):
                        return json.loads(ln[5:])
                    if ln == 'OK ' + line or ln.startswith('OK ' + line):
                        return ln
                    if ln.startswith('ERROR ' + word):
                        raise Failed(ln)
                if not self.cond.wait(max(0.0, end - time.time())) and time.time() > end:
                    raise Failed('no answer to "%s"' % line)

    def state(self):
        return self.ask('state')

    def until(self, what, test, timeout=60.0):
        end = time.time() + timeout
        st = None
        while time.time() < end:
            st = self.state()
            if test(st):
                return st
            time.sleep(0.02)
        raise Failed('%s: not reached; last state %s' % (what, json.dumps(st)))

    def click(self, x, y, button=None):
        """A click is a press, a frame, a release: the game polls its buttons once a frame (10 of its updates, which
        `state` counts), so a press and a release inside one frame would be lost"""
        extra = ' right' if button == 'right' else ''
        self.ask('down game %d %d%s' % (x, y, extra))
        self.wait(12)
        self.ask('up game %d %d%s' % (x, y, extra))

    def move(self, x, y):
        """The pointer to (x, y), by a step beside it first: when the ship comes in (after a life is lost) the game
        puts the pointer back at the board's middle, which a test run does not do to -realinput's hand, so the hand
        may already be at (x, y) and a move there makes no event"""
        self.ask('move game %d %d' % (x + 1 if x < 799 else x - 1, y))
        self.ask('move game %d %d' % (x, y))

    def key(self, name):
        """A key held for a frame, as a hand holds it (the game reads a press once a frame)"""
        name = NATIVE_KEYS.get(name, name)
        self.ask('keydown ' + name)
        self.wait(12)
        self.ask('keyup ' + name)

    def send(self, mv):
        for step in mv.get('input', []):
            if 'move' in step:
                self.move(*step['move'])
            elif 'click' in step:
                self.click(step['click'][0], step['click'][1], step.get('button'))
            elif 'key' in step:
                self.key(step['key'])
            elif 'wait' in step:
                self.wait(step['wait'])

    def wait(self, n):
        u = self.state()['updates']
        self.until('%d updates' % n, lambda s: s['updates'] >= u + n)

    def shot(self, name):
        self.ask('screenshot ' + os.path.join(self.out, name + '.png').replace('\\', '/'))

    def close(self):
        if self.proc.poll() is None:
            try:
                self.ask('quit', 10)
            except Failed:
                pass
            try:
                self.proc.wait(15)
            except subprocess.TimeoutExpired:
                self.proc.kill()


def toggle_fullscreen(g, on):
    g.ask('keydown Left Alt')
    g.ask('key Return')
    g.ask('keyup Left Alt')
    g.until('full screen %s' % ('on' if on else 'off'), lambda s: s['fullscreen'] == on)
    g.wait(30)
    g.say('full screen %s' % ('on' if on else 'off'))


def choose_shooter(g, shooter):
    g.click(*TITLE['options'])
    g.until('the options', lambda s: s['screen'] == 'options')
    g.wait(30)
    if g.state()['shooter'] != shooter:
        g.click(*OPTIONS['shooter'])
        g.until('the shooter %s' % shooter, lambda s: s['shooter'] == shooter)
    g.shot('options-%s' % shooter)
    g.click(*OPTIONS['back'])
    g.until('the title', lambda s: s['screen'] == 'title')
    g.say('shooter mode %s chosen in the options' % shooter)


def checks(g):
    """Esc pauses and Enter resumes; a focus loss freezes the game and a regain runs it on"""
    g.key('Escape')
    g.until('paused', lambda s: s['paused'])
    g.wait(20)
    g.shot('paused')
    g.key('Return')
    g.until('resumed', lambda s: not s['paused'])
    g.say('Esc paused the board, Enter resumed it')
    u = g.state()['updates']
    g.ask('focus lost')
    time.sleep(0.3)
    a = g.state()['updates']
    time.sleep(0.5)
    b = g.state()['updates']
    g.shot('focus-lost')
    g.ask('focus gained')
    time.sleep(0.5)
    c = g.state()['updates']
    g.say('focus lost: updates %d, then %d and %d while it was lost, %d half a second after the regain' % (u, a, b, c))
    if b != a:
        raise Failed('the game kept updating with the window inactive (%d, %d)' % (a, b))
    if c <= b:
        raise Failed('the game did not run again after the focus came back (%d, %d)' % (b, c))


def play_game(g, shooter, maxtime, windowed, windowed_start=False):
    g.until('the title', lambda s: s['screen'] == 'title', 90)
    g.wait(20)
    g.shot('title')
    if windowed_start and g.state()['fullscreen']:
        raise Failed('a windowed start came up in full screen')
    if windowed:
        toggle_fullscreen(g, False)
        g.shot('title-windowed')
    choose_shooter(g, shooter)
    g.click(*TITLE['start'])
    g.until('the instructions', lambda s: s['screen'] == 'instructions')
    g.wait(30)
    g.shot('instructions')
    g.click(*PLAY)
    g.until('a board', lambda s: s['screen'] == 'board')
    g.say('Start Game, the instructions page, Play: a board')
    start = time.time()
    level, done_checks, boards, stuck_since, last_progress, ended = None, False, 0, time.time(), None, False
    st = g.state()
    while True:
        st = g.state()
        scr = st['screen']
        if scr == 'board':
            if st['level'] != level:
                if level is not None:
                    boards += 1
                level = st['level']
                g.say('board %d: quota %d%%, score %d, lives %d' % (level, st['quota'], st['score'], st['lives']))
                if boards < 2:
                    g.shot('board%d' % level)
            if st['over']:
                g.say('game over on board %d: score %d, fill %d%%' % (level, st['score'], st['fill']))
                g.shot('gameover')
                g.wait(30)
                g.click(400, 300)
                g.until('the hall of fame', lambda s: s['screen'] in ('highscores', 'highscore_entry'))
                ended = True
                continue
            if not ended and time.time() - start > maxtime:
                try:
                    g.ask('lose')
                except Failed:  # between two boards (a won one's bonus texts): try again a moment later
                    g.wait(5)
                    continue
                g.say('%d s: ended by the lose test command' % maxtime)
                ended = True
                continue
            if not done_checks and level == 1 and st['shots'] >= 2:
                checks(g)
                done_checks = True
                continue
            mv = g.ask('move')
            progress = (st['score'], st['fill'], st['lives'], st['level'], st['shots'], st['won'])
            if progress != last_progress:
                last_progress, stuck_since = progress, time.time()
            elif time.time() - stuck_since > 60:
                g.shot('stuck')
                raise Failed('no progress for 60 s: %s, move %s' % (json.dumps(st), json.dumps(mv)))
            if mv is None:
                g.wait(3)
                continue
            g.send(mv)
        elif scr in ('highscores', 'highscore_entry'):
            g.wait(200)
            g.shot('halloffame')
            g.say('the hall of fame, score %d' % st['score'])
            g.click(*HALL_BACK)
            g.until('the title', lambda s: s['screen'] == 'title')
            break
        elif scr == 'mission_complete':
            g.say('the mission-complete screen')
            g.shot('missioncomplete')
            g.click(400, 300)
            g.wait(30)
        else:
            raise Failed('an unexpected screen %s' % json.dumps(st))
    g.say('game: %d boards, last score %d, %.0f s real time' % (boards + 1, st['score'], time.time() - start))
    g.shot('title-end')
    if windowed:
        toggle_fullscreen(g, True)
    if windowed_start:
        # full screen is the display's renderer mode (the desktop is no whole multiple of the game's size); back in a window the
        # display keeps its renderer, as it keeps one it has had (framework/SurfaceDisplay.h)
        toggle_fullscreen(g, True)
        g.shot('title-full')
        toggle_fullscreen(g, False)
        g.shot('title-window-again')


def run_game(out, name, exe, fast, maxtime):
    shooter, screen = name.split('-')
    start = screen if screen in ('win1x', 'win2x') else None
    g = Game(os.path.join(out, name), exe, fast, start)
    result = 'ok'
    try:
        play_game(g, shooter, maxtime, screen == 'window', start is not None)
    except Failed as e:
        result = 'FAILED: %s' % e
        try:
            g.shot('failed')
        except Failed:
            pass
    finally:
        g.close()
    g.say('RESULT ' + result)
    print('%s: RESULT %s' % (name, result), flush=True)
    return result == 'ok'


def main():
    args = sys.argv[1:]
    out, names, exe, fast, maxtime = None, ['manual-full', 'manual-window', 'auto-full', 'auto-window'], \
        'build/native/cosmicbugs.exe', False, 480
    while args:
        a = args.pop(0)
        if a == '--games':
            names = args.pop(0).split(',')
        elif a == '--maxtime':
            maxtime = int(args.pop(0))
        elif a == '--exe':
            exe = args.pop(0)
        elif a == '--fast':
            fast = True
        else:
            out = a
    ok = True
    for name in names:
        ok = run_game(os.path.abspath(out), name, os.path.abspath(exe), fast, maxtime) and ok
    print('RESULT ' + ('ok' if ok else 'FAILED'))
    return 0 if ok else 1


T0 = time.time()
if __name__ == '__main__':
    sys.exit(main())
