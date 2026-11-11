"""Phase one of the identical-play pass (docs/COORDINATION.md): the port plays a mode by its own bot with real input, in 2D at
640x480 (what the original does on rundesk's desktop, which has no 3D), and this writes down every input with the update at
which it ran, so that the same inputs can be given to the original at the same updates (origcmp/to_origrun.py) and to the
port again. The port's -trace lines (the board's score, level, state and generator position, as origrun's --watch reads them in
the original) stay in game.log.

    python games/bejeweled2/tools/origcmp/record.py --exe <exe> --userdir <a new folder> --out <dir> --mode classic [--moves 60]

--mode is a game mode, `hidden`, `resume` (Classic left by its menu button and loaded again), `load` (the saved game of the
player's folder, which make_save.py makes; --hyper swaps a hyper cube first), `tour` (phase two's first screens, each shot
when it stands still; the original takes the same shots at the same updates in Windows Sandbox) or `tourload` (the same for a
saved game: PROFILE=<make_save.py's folder>, the menu, "Load Game?" and the loaded board).

Writes <out>/events.json ({"play": the update of the title's play click, "events": [[update, "command line"], ...]}) and
<out>/game.log. Runs under rundesk (the Driver does that), held, fast and seeded.
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'realplay'))
import full  # noqa: E402
from driver_base import Driver  # noqa: E402

full.KEY_HINTS = True  # the port's 640 by 480 layout of the hint dialogs is not the original's: close them with the key, which no layout changes
INPUT_VERBS = ('move', 'click', 'down', 'up', 'key', 'type')


class Recorder(Driver):
    """A Driver that notes the update at which each input command runs (a command sent after a state reply runs one update
    later: -hold lets one update run for each new line)."""

    def __init__(self, *a, **k):
        super().__init__(*a, **k)
        self.events = []

    def ok(self, line):
        if line.split()[0] in INPUT_VERBS:
            s = self.state()
            # a third item, 1, marks an input that reaches a dialog: the original's 3D game takes such a click one update later than the
            # port does (to_origrun.py --dialog-shift)
            self.events.append([s['updates'] + 1, line] + ([1] if s.get('dialogs') else []))
        super().ok(line)

    def shot(self, name):
        """A screenshot of the port, noted with its update so that the original is captured at the same one (to_origrun.py
        writes `at <update> shot <name>`, which origrun takes only inside Windows Sandbox)."""
        s = self.state()
        # the picture is the frame already drawn for the update the state reply names (an input, sent after it, runs one update
        # later); the slide of the How To Play page showed it: with +1 the port's frame lagged the original's by one update
        self.events.append([s['updates'], 'shot ' + name])
        return super().shot(name)


def start_hidden(d):
    """The hidden mode: the mouse goes round the four mode buttons in order (classic, action, endless, puzzle), 32 steps in a row; the
    menu counts a step when a different button is under the mouse at an update, so each stop lasts a few updates."""
    full.until(d, full.menu_ready, 'the menu laid out', 60)
    spots = [d.state()['widgets'][k] for k in ('classic', 'action', 'endless', 'puzzle')]
    i = 0
    for _ in range(400):
        s = d.state()
        if s['screen'] == 'game' or s.get('mode') == 'hidden':
            return
        if s['dialogs']:
            # the welcome message that follows a new player's first visit to the menu: closed with the key, since the port's
            # 640 by 480 layout of the dialog is not the original's and a click on the port's button misses the original's
            # (the original's menu then never counted the circle: its message stayed up)
            if max(s['dialogs']) == 36:
                d.key('Escape')
                d.ok('wait 3')
            else:
                full.handle_dialog(d, s)
            continue
        x, y = spots[i % 4]
        i += 1
        d.ok('move game %d %d' % (x, y))
        d.ok('wait 3')
    raise SystemExit('the hidden mode did not start: ' + json.dumps(d.state()))


def resume(d, a):
    """A saved game resumed: Classic for --moves moves, the game's own menu button (the game is saved on the way out), the menu's Classic
    button, the "Load Game?" dialog answered yes, and --moves more moves of the loaded game."""
    r1 = full.play(d, 'classic', a.moves, a.out)
    d.click(d.state()['widgets']['menu'])  # the options dialog, with a Main Menu button while a game is on
    s = full.until(d, lambda s: 2 in s['dialogs'] and 'mainmenu' in (s.get('buttons') or {}), 'the options dialog', 60)
    d.click(s['buttons']['mainmenu'])
    full.to_menu(d)
    full.start_mode(d, 'classic')
    r2 = full.play(d, 'classic', a.moves, a.out)
    return dict(r2, first=r1)


def tour(d, a):
    """Phase two's first screens, one shot each once the screen stands still: the new player's welcome, the menu, the
    messages in front of the first game, the board, the options dialog over a game, and "Load Game?" on the way back in.
    (The title and the new-user dialog are shot by main, before the menu.)"""
    seen = set()

    def settle(pred, what, shots=True):
        while True:
            s = d.state()
            if s['dialogs']:
                top = max(s['dialogs'])
                if shots and top not in seen:
                    seen.add(top)
                    if top == 40 and os.environ.get('TOUR_FINE'):  # the slide's own curve, a shot every few updates (the port only)
                        for k in range(24):
                            d.shot('fine%02d' % k)
                            d.ok('wait 2')
                        continue
                    d.ok('wait 60')
                    d.shot('dialog%d' % top)
                    if top == 40:  # the How To Play page slides in over about 100 updates: three more shots along it
                        for k in 'bcd':
                            d.ok('wait 20')
                            d.shot('dialog40' + k)
                        for k in range(14):  # and its four pages (675 updates each) one after the other
                            d.ok('wait 200')
                            d.shot('dialog40p%02d' % k)
                    continue
                if top == 36:
                    d.key('Escape')
                    d.ok('wait 3')
                else:
                    full.handle_dialog(d, s)
            elif pred(s):
                return s
            else:
                d.idle()

    s = settle(full.menu_ready, 'the menu')
    d.ok('wait 120')
    s = settle(full.menu_ready, 'the menu')
    d.shot('menu')
    # the high scores, and the options dialog opened from the menu (its Done button is the yes), each shot once it stands still
    d.click(s['widgets']['scores'])
    full.until(d, lambda s: s['screen'] == 'highscores' and not s['dialogs'], 'the high scores', 60)
    d.ok('wait 120')
    d.shot('scores')
    d.click(d.state()['widgets']['back'])
    s = settle(full.menu_ready, 'the menu')
    d.ok('wait 60')
    d.click(s['widgets']['options'])
    s = full.until(d, lambda s: 2 in s['dialogs'] and 'yes' in (s.get('buttons') or {}), 'the options dialog', 60)
    d.ok('wait 60')
    d.shot('options-menu')
    d.click(d.state()['buttons']['yes'])
    s = settle(full.menu_ready, 'the menu')
    d.ok('wait 60')
    d.click(s['widgets']['classic'])
    settle(lambda s: s['screen'] == 'game' and s.get('phase') == 'idle', 'the board')
    d.ok('wait 60')
    d.shot('board')
    r = full.play(d, 'classic', a.moves, a.out)
    s = settle(lambda s: s['screen'] == 'game' and s.get('phase') == 'idle', 'the board')
    d.shot('board-played')
    d.click(s['widgets']['menu'])
    s = full.until(d, lambda s: 2 in s['dialogs'] and 'mainmenu' in (s.get('buttons') or {}), 'the options dialog', 60)
    d.ok('wait 60')
    d.shot('options')
    s = d.state()
    d.click(s['buttons']['mainmenu'])
    s = settle(full.menu_ready, 'the menu')
    d.ok('wait 100')
    d.shot('menu-back')
    d.click(d.state()['widgets']['classic'])
    s = full.until(d, lambda s: 17 in s['dialogs'], 'the load dialog', 60)
    d.ok('wait 60')
    d.shot('loadgame')
    s = d.state()
    d.click(s['buttons']['yes'])
    settle(lambda s: s['screen'] == 'game' and s.get('phase') == 'idle', 'the loaded board')
    d.ok('wait 60')
    d.shot('board-loaded')
    return r


def tourload(d, a):
    """The screens of a saved game (PROFILE, from make_save.py; the original gets the same player staged in Windows Sandbox): the
    title, the welcome back, the menu, "Load Game?" and the loaded board, each shot once it stands still."""
    s = full.until(d, lambda s: full.menu_ready(s) and not s['dialogs'], 'the menu', 600)
    d.ok('wait 120')
    s = full.until(d, lambda s: full.menu_ready(s) and not s['dialogs'], 'the menu', 60)
    d.shot('menu')
    d.click(s['widgets']['classic'])
    s = full.until(d, lambda s: 17 in s['dialogs'], 'the load dialog', 60)
    d.ok('wait 60')
    d.shot('loadgame')
    d.click(d.state()['buttons']['yes'])
    seen = set()
    for _ in range(3000):  # the messages in front of a loaded game (a flame gem's, a hyper cube's) are shot once each and answered
        s = d.state()
        if s['dialogs']:
            top = max(s['dialogs'])
            if top not in seen:
                seen.add(top)
                d.ok('wait 60')
                d.shot('dialog%d' % top)
            full.handle_dialog(d, s)
        elif s['screen'] == 'game' and s.get('phase') == 'idle':
            break
        else:
            d.idle()
    else:
        raise SystemExit('the loaded board did not settle: ' + json.dumps(d.state()))
    d.ok('wait 60')
    d.shot('board-loaded')
    return dict(loaded=True)


def tourdialogs(d, a):
    """The dialogs that no other tour reaches (`--mode tour:dialogs`, PROFILE = make_profile.py's player): "Quit?", the user list with its
    Rename, Delete and create dialogs, and the high scores' reset question, each shot once it stands still and answered No."""
    def menu():
        s = full.until(d, lambda s: full.menu_ready(s) and not s['dialogs'], 'the menu', 600)
        d.ok('wait 60')
        return full.until(d, lambda s: full.menu_ready(s) and not s['dialogs'], 'the menu', 60)

    def dialog(ident, name, wait=60):
        s = full.until(d, lambda s: ident in s['dialogs'] and (s.get('buttons') or {}).get('no'), 'dialog %d' % ident, 120)
        d.ok('wait %d' % wait)
        d.shot(name)
        return d.state()

    def answer_no():
        d.click(d.state()['buttons']['no'])
        d.ok('wait 60')

    s = menu()
    d.click(s['widgets']['quit'])
    dialog(1, 'quit')
    answer_no()
    s = menu()
    d.click(s['widgets']['user'])
    s = dialog(32, 'users')
    d.click(s['buttons']['row1'])
    d.ok('wait 30')
    d.shot('users-sel')
    d.click(d.state()['buttons']['rename'])
    dialog(29, 'rename')
    answer_no()
    d.click(d.state()['buttons']['delete'])
    dialog(30, 'delete')
    answer_no()
    d.click(d.state()['buttons']['row0'])
    dialog(27, 'create')
    answer_no()
    answer_no()  # the list's own Cancel
    s = menu()
    d.click(s['widgets']['scores'])
    s = full.until(d, lambda s: s['screen'] == 'highscores' and not s['dialogs'] and 'again' in (s.get('widgets') or {}), 'the high scores', 120)
    d.ok('wait 60')
    d.click(d.state()['widgets']['again'])
    dialog(33, 'reset')
    answer_no()
    return {}


def tourmode(d, a):
    """One game mode's screens (`--mode tour:<mode>`, the player of make_profile.py with every mode unlocked): the menu (the secret
    menu for a secret mode), every message and screen in front of the board, the board, the board after --moves moves with five
    shots along each level change, and, for a timed mode, the end screen. One shot each once the screen stands still. The original
    gets the same inputs at the same updates, one mode a run: it crashes in the sandbox when its options dialog closes."""
    mode = a.mode.split(':', 1)[1]
    if mode == 'dialogs':
        return tourdialogs(d, a)
    seen = set()

    def settle(pred, screens=False):
        for _ in range(4000):
            s = d.state()
            if s['dialogs']:
                top = max(s['dialogs'])
                if top not in seen:
                    seen.add(top)
                    d.ok('wait 60')
                    d.shot('dialog%d' % top)
                    continue
                if top == 36:
                    d.key('Escape')
                    d.ok('wait 3')
                else:
                    full.handle_dialog(d, s)
            elif pred(s):
                return s
            else:
                if screens and s['screen'] not in ('game', 'menu') and s['screen'] not in seen:
                    seen.add(s['screen'])
                    d.ok('wait 60')
                    d.shot('screen-' + s['screen'])
                    continue
                d.idle()
        d.shot('stuck')
        raise SystemExit('the screen did not settle: ' + full.describe(d.state()))

    s = settle(full.menu_ready)
    d.ok('wait 120')
    s = settle(full.menu_ready)
    d.shot('menu')
    if mode in full.SECRET:
        d.click(s['widgets']['secret'])
        d.ok('wait 80')
        d.shot('menu-secret')
        s = d.state()
    d.click(s['widgets'][full.SECRET.get(mode, mode)])
    for k in range(int(os.environ.get('TOUR_FINE', '0'))):  # the start of the game along its way, a shot every 6 updates
        d.ok('wait 6')
        d.shot('f%02d' % k)
    settle(lambda s: s['screen'] == 'game' and s.get('mode') == mode and s.get('phase') == 'idle', screens=True)
    d.ok('wait 60')
    d.shot('board')
    made, level, r = 0, d.state().get('level'), {}
    while made < a.moves:
        r = full.play(d, mode, 1, a.out)
        made += r['moves']
        s = d.state()
        if s['screen'] != 'game' or s.get('over'):
            break
        if s.get('level') != level:
            level = s.get('level')
            for k in range(5):
                d.ok('wait 40')
                d.shot('level%s-%d' % (level, k))
    if mode in full.PUZZLE_MODES:
        settle(lambda s: s['screen'] == 'game' and s.get('phase') in ('idle', 'puzzle solved'), screens=True)
    if mode in full.PUZZLE_MODES and d.state().get('phase') == 'puzzle solved':
        # the world is complete: the pod asks for a click on WARP (at the same place in every size of the game's layout, in game pixels)
        d.ok('wait 200')
        d.shot('solved')
        d.click([95, 103])
        for k in range(8):
            d.ok('wait 40')
            d.shot('warp%d' % k)
    if not d.state().get('over'):
        settle(lambda s: s['screen'] == 'game' and s.get('phase') == 'idle', screens=True)
        d.shot('board-played')
        for _ in range(400 if mode in ('action', 'hyper') else 0):  # the timed modes end by themselves
            if d.state().get('over'):
                break
            d.ok('wait 100')
    if d.state().get('over'):  # the end of a game: its screens, then the high scores (a name first when it is one)
        for k in range(4):
            d.ok('wait 100')
            d.shot('over%d' % k)
        settle(lambda s: s['screen'] == 'highscores', screens=True)
        d.ok('wait 100')
        d.shot('scores-after')
    return dict(r, moves=made)


def swap_hyper(d):
    """Swaps the first hyper cube on the board with a neighbour, as two clicks (the bot's own move tells where the squares are
    on the screen). Returns False when the board has none."""
    while True:
        s = d.state()
        if s['dialogs']:
            full.handle_dialog(d, s)
            continue
        m = d.bot_move()
        if m is not None and m.get('kind') == 'swap':
            break
        d.play_input(m['input']) if m is not None else d.idle()
    at = [(c, r) for r, row in enumerate(s['board']) for c, ch in enumerate(row) if ch == 'c']
    if not at:
        return False
    (c1, r1), (c2, r2) = m['from'], m['to']
    (x1, y1), (x2, y2) = m['input'][0]['move'], m['input'][2]['move']
    step = max(abs(x2 - x1), abs(y2 - y1))
    col, row = at[0]
    other = col + 1 if col < 7 else col - 1
    for c in (col, other):
        p = [x1 + (c - c1) * step, y1 + (row - r1) * step]
        d.move(p)
        d.ok('click game %d %d' % (p[0], p[1]))
    d.wait_updates(4)
    return True


def load(d, a):
    """A saved game that the player's folder holds (PROFILE, from make_save.py): the menu's Classic button, the "Load Game?"
    dialog answered yes, a hyper cube swapped with its neighbour when the board has one (--hyper), and --moves moves."""
    full.start_mode(d, 'classic')
    start = d.events[-1][0]
    hyper = swap_hyper(d) if a.hyper else None
    return dict(full.play(d, 'classic', a.moves, a.out), hyper=hyper), start


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--gamedir', default='')
    ap.add_argument('--userdir', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--mode', default='classic')
    ap.add_argument('--moves', type=int, default=60)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--lowres', action='store_true', help='true 2D at 640x480 (what the original does without 3D) instead of 1024x768')
    ap.add_argument('--nosparkle', action='store_true', help="send the test command nosparkle (Board::AddAlphaSparkle returns at once, as the original's patched copy does): the port has no 640 by 480 scaled images for its pixel test")
    ap.add_argument('--idle-end', type=int, default=0, help='after the moves, send no more input for up to this many updates, until the game is over (the timed modes end when the bar runs out) and 700 updates more, so that the original is compared through its ending')
    ap.add_argument('--hyper', action='store_true', help='--mode load: the first hyper cube of the loaded board is swapped with its neighbour before the moves')
    ap.add_argument('--drawevery', action='store_true', help='draw after every update (-drawevery), as the original does on its virtual clock: where the screen keeps the last frame, an effect that adds to it (the 2D whirlpool) then adds once per update, not once per -fast draw')
    ap.add_argument('--play-at', type=int, default=345, help='the update of the title play click: the first at which the original takes one (its title has loaded at 319)')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    d = Recorder(a.exe, a.gamedir, a.userdir, a.out, True, ['-seed=%d' % a.seed, '-trace'] + (['-drawevery'] if a.drawevery else []), windowed=True, is3d=False if a.lowres else None, hold=True)
    rc = 1
    try:
        full.until(d, lambda s: s['screen'] in ('title', 'dialog', 'menu'), 'the title', 180)
        if a.nosparkle:
            d.ok('nosparkle')
        s = d.state()
        if s['updates'] < a.play_at - 1:
            d.ok('wait %d' % (a.play_at - 1 - s['updates']))  # the original and the port then get every input at the same update
        if a.mode == 'tourload' or a.mode.startswith('tour:'):
            d.ok('wait 60')
            d.shot('title')
        if a.mode == 'tour':
            d.ok('wait 60')
            d.shot('title')
            d.click(d.state()['widgets']['play'])
            s = full.until(d, lambda s: 27 in s['dialogs'], 'the new-user dialog', 60)
            d.ok('wait 60')
            d.shot('newuser')
        full.to_menu(d) if a.mode != 'tour' else (d.type(full.NAME), d.key('Return'))
        play = next(u for u, c, *_ in d.events if c.startswith('click'))
        loaded = None
        if a.mode == 'hidden':
            start_hidden(d)
        elif a.mode == 'tour':
            toured = tour(d, a)
        elif a.mode == 'tourload':
            toured = tourload(d, a)
        elif a.mode.startswith('tour:'):
            toured = tourmode(d, a)
        elif a.mode == 'load':
            loaded, load_start = load(d, a)
        elif a.mode == 'resume':
            full.start_mode(d, 'classic')
        else:
            if a.mode in full.SECRET:
                d.ok('unlock all')
            full.start_mode(d, a.mode)
        start = d.events[-1][0]
        if a.mode == 'load':
            r = loaded
            start = load_start
        elif a.mode in ('tour', 'tourload') or a.mode.startswith('tour:'):
            r = toured
        elif a.mode == 'resume':
            r = resume(d, a)
        else:
            r = full.play(d, a.mode, a.moves, a.out)
        if a.idle_end:
            t0 = d.state()['updates']
            while d.state()['updates'] < t0 + a.idle_end and 'over' not in str(d.state().get('phase', '')):
                d.ok('wait 100')
            if 'over' in str(d.state().get('phase', '')):
                d.ok('wait 700')
            r = dict(r, phase=d.state().get('phase'))
        last = d.state()['updates']
        print(a.mode, json.dumps(r), 'play click at', play, 'mode click at', start, 'ends at', last)
        with open(os.path.join(a.out, 'events.json'), 'w') as f:
            json.dump({'play': play, 'start': start, 'last': last, 'mode': a.mode, 'events': d.events}, f)
        rc = 0
    finally:
        d.close()
    return rc


if __name__ == '__main__':
    sys.exit(main())
