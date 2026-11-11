"""Phase one of the identical-play pass (docs/COORDINATION.md): the port plays a mode by its own bot with real input, in 2D at
640x480 (what the original does on rundesk's desktop, which has no 3D), and this writes down every input with the update at
which it ran, so that the same inputs can be given to the original at the same updates (origcmp/to_origrun.py) and to the
port again. The port's -trace lines (the board's score, level, state and generator position, as origrun's --watch reads them in
the original) stay in game.log.

    python games/bejeweled2/tools/origcmp/record.py --exe <exe> --userdir <a new folder> --out <dir> --mode classic [--moves 60]

--mode is a game mode, `hidden`, `resume` (Classic left by its menu button and loaded again) or `load` (the saved game of the
player's folder, which make_save.py makes; --hyper swaps a hyper cube first).

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
    ap.add_argument('--play-at', type=int, default=345, help='the update of the title play click: the first at which the original takes one (its title has loaded at 319)')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    d = Recorder(a.exe, a.gamedir, a.userdir, a.out, True, ['-seed=%d' % a.seed, '-trace'], windowed=True, is3d=False if a.lowres else None, hold=True)
    rc = 1
    try:
        full.until(d, lambda s: s['screen'] in ('title', 'dialog', 'menu'), 'the title', 180)
        if a.nosparkle:
            d.ok('nosparkle')
        s = d.state()
        if s['updates'] < a.play_at - 1:
            d.ok('wait %d' % (a.play_at - 1 - s['updates']))  # the original and the port then get every input at the same update
        full.to_menu(d)
        play = d.events[0][0]
        loaded = None
        if a.mode == 'hidden':
            start_hidden(d)
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
