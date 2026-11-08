"""Plays Bejeweled Deluxe with real input only (CLAUDE.md, "Done means identical"): a driver, not a player. The game runs
under rundesk with -commands=<file> and -realinput=<empty file> (docs/REAL_INPUT.md, docs/TEST_COMMANDS.md "Sending
them"), so every click and key it is sent is an SDL event that takes a hand's path (HandleEvent, the window-to-game mapping,
the widget manager). The moves come from the port's own bot through the test command "move"; this script is the thin loop
around it: it asks "state" and "move", sends the move's input steps as `move game x y` and `click game x y` lines, and
waits for each line's answer before the next.

    build/native/rundesk.exe python games/bejeweled/tools/realplay/drive.py --out <dir> [--exe E] [--gamedir G]
        [--realtime] [--moves N] [steps...]

The steps (default: every one, in this order) are
    launch      from the loading screen to the title, the play link, the first game's hint (the tutorial), and its Options
                and Help dialogs, all by clicks
    normal      a whole Normal game, to its end (no move left), and the name entry after it
    timed       a whole Timed game, to its end (time up), and the name entry after it
    focus       mid-game: the focus lost (the game pauses, the clock stands still) and regained
    fullscreen  mid-game: Alt+Enter to full screen, moves made there, and back
Each prints one line, "step <name>: ok ..." or what failed, and the last line is "realplay: ok" or "realplay: FAILED". Pictures
and the game's log are in <dir>. Run it under rundesk so that nothing opens on the user's desktop.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time

ANSWER = re.compile(r'^(STATE |MOVE |OK |ERROR )')

# game pixels (640x480, the original's coordinates)
PLAY_LINK = (320, 398)       # the title's "click here to play" (TitleScreen: PlayLinkY 386 + 12)
NEW_GAME = (96, 162)         # the side bar's buttons (WinDMApp::MakeButton rectangles' middles)
NORMAL_MODE = (57, 200)
TIMED_MODE = (135, 200)
OPTIONS = (96, 261)
OPTIONS_HELP = (330, 232)    # the Options dialog's buttons (never "Play More Great Games!" at (408, 265): it opens a URL)
OPTIONS_DONE = (408, 299)
HELP_DONE = (320, 399)       # the How to Play dialog's Done
PLAY_AGAIN = (415, 401)      # the game-over screen's "click here to play again"
YES = (338, 170)             # the question "You must end the current game to switch to normal mode. Ok?"


class Driver:
    def __init__(self, exe, gamedir, userdir, out, fast, extra):
        self.out = out
        self.cmdfile = os.path.join(out, 'commands.txt')
        self.empty = os.path.join(out, 'realinput.txt')
        open(self.cmdfile, 'w').close()
        open(self.empty, 'w').close()
        self.logpath = os.path.join(out, 'game.log')
        args = [exe, '-gamedir=' + gamedir, '-userdir=' + userdir, '-commands=' + self.cmdfile,
                '-realinput=' + self.empty] + (['-fast'] if fast else []) + extra
        self.log = open(self.logpath, 'wb')
        self.proc = subprocess.Popen(args, stdout=self.log, stderr=subprocess.STDOUT)
        self.pos = 0
        self.sent = 0

    def alive(self):
        return self.proc.poll() is None

    def _new_lines(self):
        with open(self.logpath, 'rb') as f:
            f.seek(self.pos)
            data = f.read()
        end = data.rfind(b'\n')
        if end < 0:
            return []
        self.pos += end + 1
        return [l.decode('utf-8', 'replace').rstrip('\r') for l in data[:end].split(b'\n')]

    def send(self, line, timeout=60):
        """Appends a line to the commands file and waits for its answer (STATE/MOVE/OK/ERROR); returns the answer line."""
        self._new_lines()  # what came before is not this line's answer
        with open(self.cmdfile, 'ab') as f:
            f.write((line + '\n').encode())
        self.sent += 1
        deadline = time.time() + timeout
        while True:
            for l in self._new_lines():
                l = re.sub(r'^INFO: ', '', l)
                if ANSWER.match(l):
                    return l
            if not self.alive():
                raise RuntimeError('the game ended before the answer to "%s"' % line)
            if time.time() > deadline:
                raise RuntimeError('no answer to "%s" in %d s' % (line, timeout))
            time.sleep(0.005)

    def ok(self, line):
        a = self.send(line)
        if not a.startswith('OK'):
            raise RuntimeError('"%s": %s' % (line, a))

    def state(self):
        a = self.send('state')
        if not a.startswith('STATE '):
            raise RuntimeError('state: ' + a)
        return json.loads(a[6:])

    def bot_move(self):
        a = self.send('move')
        if a.startswith('MOVE null'):
            return None
        if not a.startswith('MOVE '):
            raise RuntimeError('move: ' + a)
        return json.loads(a[5:])

    # a hand's input, in game pixels
    def move(self, p):
        self.ok('move game %d %d' % tuple(p))

    def click(self, p):
        self.ok('move game %d %d' % tuple(p))
        self.ok('click game %d %d' % tuple(p))

    def key(self, name):
        self.ok('key ' + name)

    def type(self, text):
        self.ok('type ' + text)

    def shot(self, name):
        path = os.path.join(self.out, name + '.png')
        self.ok('screenshot ' + path.replace('\\', '/'))
        return path

    def wait_updates(self, n, timeout=30):
        """Waits until the board has made n more updates (the board's tick), or the screen is no longer the game."""
        s = self.state()
        if 'tick' not in s:
            time.sleep(0.05)
            return s
        want = s['tick'] + n
        deadline = time.time() + timeout
        while s.get('tick', want) < want and s['screen'] == 'game' and time.time() < deadline:
            time.sleep(0.01)
            s = self.state()
        return s

    def wait_screen(self, screens, timeout=60):
        deadline = time.time() + timeout
        s = self.state()
        while s['screen'] not in screens:
            if time.time() > deadline:
                raise RuntimeError('screen %s, wanted %s' % (s['screen'], '/'.join(screens)))
            time.sleep(0.02)
            s = self.state()
        return s

    def play_input(self, steps):
        """A MOVE's input: an ordered list of steps in game pixels (docs/TEST_COMMANDS.md)."""
        for st in steps:
            if 'move' in st:
                self.move(st['move'])
            elif 'click' in st:
                p = st['click']
                self.ok('click game %d %d%s' % (p[0], p[1], ' right' if st.get('button') == 'right' else ''))
            elif 'press' in st:
                self.ok('down game %d %d' % tuple(st['press']))
            elif 'release' in st:
                self.ok('up game %d %d' % tuple(st['release']))
            elif 'key' in st:
                self.key(st['key'])
            elif 'wait' in st:
                self.wait_updates(st['wait'])

    def close(self):
        try:
            if self.alive():
                with open(self.cmdfile, 'ab') as f:
                    f.write(b'quit\n')
                try:
                    self.proc.wait(10)
                except subprocess.TimeoutExpired:
                    self.proc.kill()
        finally:
            self.log.close()


def fail(msg):
    raise RuntimeError(msg)


def play_game(d, label, max_moves):
    """Plays one game by the bot's moves as real input until it ends; returns a summary dict."""
    made = nulls = 0
    info = {}
    levels = set()
    t0 = time.time()
    s = None
    while True:
        s = d.state()
        if s['screen'] != 'game':
            break
        levels.add(s['level'])
        if s['moves'] >= 1 and 'tutorial after the first move' not in info:
            info['tutorial after the first move'] = s['tutorial']
        if s['over']:
            d.wait_updates(5)
            time.sleep(0.05)
            continue
        if made >= max_moves:
            return {'stop': 'move limit', 'moves': made, 'levels': sorted(levels), 'state': s, 'info': info}
        m = d.bot_move()
        if m is None:
            nulls += 1
            if nulls > 400:
                fail('%s: no move for a long time (%s)' % (label, json.dumps({k: s.get(k) for k in ('phase', 'busy', 'over')})))
            time.sleep(0.01)
            continue
        nulls = 0
        d.play_input(m['input'])
        if m['kind'] == 'wait':
            continue
        made += 1
        if made % 100 == 0:
            d.shot('%s-%d' % (label, made))
    return {'stop': s['screen'], 'moves': made, 'levels': sorted(levels), 'seconds': round(time.time() - t0), 'info': info}


def play_some(d, n):
    """n bot moves as real input, from a game in progress."""
    made = 0
    deadline = time.time() + 120
    while made < n and time.time() < deadline:
        s = d.state()
        if s['screen'] != 'game' or s['over']:
            break
        m = d.bot_move()
        if m is None:
            time.sleep(0.01)
            continue
        d.play_input(m['input'])
        if m['kind'] != 'wait':
            made += 1
    return made


def alt_enter(d):
    """Alt+Enter as a hand types it: Left Alt down, Return, Left Alt up."""
    d.ok('keydown Left Alt')
    d.key('Return')
    d.ok('keyup Left Alt')


def step_launch(d, rep):
    """The loading screen, the title, the play link, the first game's hint (the tutorial) and the Options and Help dialogs."""
    d.wait_screen(('title',), 180)
    d.shot('title')
    d.click(PLAY_LINK)
    s = d.wait_screen(('game',), 30)
    # the hint dialog and the tutorial arrows come at the board's update 150, while the game has made no scoring move
    deadline = time.time() + 60
    while not s.get('tutorial') and time.time() < deadline:
        time.sleep(0.02)
        s = d.state()
    if not s.get('tutorial'):
        fail('the first game showed no tutorial')
    d.shot('hint')
    rep['tutorial'] = True
    # Options by a click, Help in it, and out again by Done twice
    d.click(OPTIONS)
    d.wait_screen(('options',), 10)
    d.shot('options')
    d.click(OPTIONS_HELP)
    d.wait_screen(('help',), 10)
    d.shot('help')
    d.click(HELP_DONE)
    s = d.wait_screen(('options', 'game'), 10)
    if s['screen'] == 'options':
        d.click(OPTIONS_DONE)
        d.wait_screen(('game',), 10)
    rep['dialogs'] = 'options, help'


def enter_name(d, name):
    """The name entry after a good game: typed with real keys, then Enter."""
    d.shot('recordscore')
    d.type(name)
    d.key('Return')


def settle_end(d):
    """The game is over: waits for the name entry, which comes a moment after the game-over screen when the score earns a place
    in the table, types it in if it comes, and returns the state of the game-over screen and whether a name was entered."""
    s = d.wait_screen(('recordscore', 'gameover'), 60)
    for _ in range(80):
        if s['screen'] != 'gameover':
            break
        time.sleep(0.05)
        s = d.state()
    named = s['screen'] == 'recordscore'
    if named:
        enter_name(d, 'Driver')
        s = d.wait_screen(('gameover',), 30)
    return s, named


def finish_game(d, rep, label, played):
    """After a game ends: the name entry if the game asks for one, then the game-over screen."""
    s, named = settle_end(d)
    if named:
        rep[label + ' name'] = 'entered'
    d.shot(label + '-over')
    rep[label] = {k: played.get(k) for k in ('moves', 'levels', 'seconds', 'stop')}
    rep[label]['score'] = s.get('score')


def to_game(d, mode, fresh=False):
    """Gets a game of the mode, by clicks as a hand does: the name entry typed, the question "end the current game" answered
    Yes, play again clicked, the mode's button clicked (once, when a new game is wanted)."""
    clicked = False
    for _ in range(16):
        s = d.state()
        scr = s['screen']
        if scr == 'recordscore':
            enter_name(d, 'Driver')
            d.wait_screen(('gameover',), 30)
        elif scr == 'dialog':
            d.shot('question-' + mode)
            d.click(YES)
            time.sleep(0.2)
        elif scr == 'game' and s.get('over'):
            settle_end(d)
        elif scr == 'gameover':
            d.click(PLAY_AGAIN)
            d.wait_screen(('game', 'dialog'), 30)
        elif scr == 'game':
            if s['mode'] == mode and (clicked or not fresh):
                return s
            d.click(TIMED_MODE if mode == 'timed' else NORMAL_MODE)
            clicked = True
            time.sleep(0.2)
        else:
            time.sleep(0.2)
    fail('no %s game after 16 tries (%s)' % (mode, json.dumps({k: s.get(k) for k in ('screen', 'mode', 'over')})))


def step_game(d, rep, mode, max_moves):
    first = to_game(d, mode)
    played = play_game(d, mode, max_moves)
    if first.get('tutorial'):
        gone = played['info'].get('tutorial after the first move') is False
        rep['tutorial gone after the first move'] = gone
        if not gone:
            fail('the hint of the first game was still shown after its first move')
    if played['stop'] == 'move limit':
        rep[mode] = played
        fail('%s game not over after %d moves' % (mode, played['moves']))
    finish_game(d, rep, mode, played)


def step_focus(d, rep):
    """The focus lost and regained in a Timed game: the board pauses (its clock and updates stand still), then runs again."""
    to_game(d, 'timed')
    play_some(d, 6)
    s = d.state()
    if s['screen'] != 'game' or s['over']:
        fail('the Timed game ended before the focus check (%s)' % json.dumps({k: s.get(k) for k in ('screen', 'phase', 'drain')}))
    d.ok('focus lost')
    time.sleep(0.3)
    a = d.state()
    time.sleep(0.5)
    b = d.state()
    if not a['paused'] or a['tick'] != b['tick'] or a['drain'] != b['drain']:
        fail('focus lost: the game did not stand still (%s)' % json.dumps({'a': [a['paused'], a['tick'], a['drain']], 'b': [b['tick'], b['drain']]}))
    d.shot('focus-lost')
    d.ok('focus gained')
    time.sleep(0.5)
    c = d.state()
    rep['focus'] = {'lost': {'paused': a['paused'], 'tick': a['tick']}, 'gained': {'paused': c['paused'], 'tick': c['tick'], 'screen': c['screen']}}
    d.shot('focus-gained')
    if c['paused'] and c['tick'] == b['tick']:
        fail('focus gained: the game did not resume (%s)' % json.dumps(rep['focus']))


def step_fullscreen(d, rep):
    """Alt+Enter to full screen, moves made there, Alt+Enter back, moves made in the window again."""
    to_game(d, 'normal', fresh=True)
    s = d.state()
    if not s['windowed']:
        fail('the game does not start windowed')
    alt_enter(d)
    time.sleep(0.5)
    s = d.state()
    if s['windowed']:
        fail('Alt+Enter did not make the window full screen')
    d.shot('fullscreen')
    n1 = play_some(d, 8)
    alt_enter(d)
    time.sleep(0.5)
    s = d.state()
    if not s['windowed']:
        fail('the second Alt+Enter did not return to the window')
    n2 = play_some(d, 8)
    s2 = d.state()
    if n1 < 1 or n2 < 1:
        fail('no moves were made full screen (%d) or after (%d)' % (n1, n2))
    rep['fullscreen'] = {'moves in full screen': n1, 'moves in the window after': n2, 'score': s2.get('score')}
    d.shot('fullscreen-back')


STEPS = ('launch', 'normal', 'timed', 'focus', 'fullscreen')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    ap.add_argument('--exe')
    ap.add_argument('--gamedir')
    ap.add_argument('--realtime', action='store_true', help='no -fast: real time')
    ap.add_argument('--moves', type=int, default=4000, help='moves a game is stopped at (a game that ends earlier is whole)')
    ap.add_argument('steps', nargs='*')
    a = ap.parse_args()
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.abspath(os.path.join(here, '..', '..', '..', '..'))
    exe = os.path.abspath(a.exe or os.path.join(root, 'build', 'native', 'bejeweled.exe'))
    gamedir = os.path.abspath(a.gamedir or os.path.join(root, 'assets', 'bejeweled'))
    out = os.path.abspath(a.out)
    os.makedirs(out, exist_ok=True)
    userdir = os.path.join(out, 'user')
    os.makedirs(userdir, exist_ok=True)
    with open(os.path.join(userdir, 'registry.ini'), 'w') as f:
        f.write('[SOFTWARE\\Steam\\Bejeweled]\nScreenMode=dword:0\n')
    steps = a.steps or list(STEPS)
    d = Driver(exe, gamedir, userdir, out, not a.realtime, [])
    rep = {}
    ok = True
    try:
        for name in steps:
            try:
                if name == 'launch':
                    step_launch(d, rep)
                elif name in ('normal', 'timed'):
                    step_game(d, rep, name, a.moves)
                elif name == 'focus':
                    step_focus(d, rep)
                elif name == 'fullscreen':
                    step_fullscreen(d, rep)
                else:
                    fail('unknown step ' + name)
                print('step %s: ok %s' % (name, json.dumps(rep)))
            except Exception as e:
                ok = False
                print('step %s: FAILED %s' % (name, e))
                break
    finally:
        d.close()
    print('realplay: ' + ('ok' if ok else 'FAILED'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
