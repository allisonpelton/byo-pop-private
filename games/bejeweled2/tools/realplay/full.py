"""The real-input driver of Bejeweled 2 (CLAUDE.md, "Done means identical": code plays, never the session): from launch through
the title's play link, the new player's name, the welcome message, the menu's mode buttons, the tutorial of the puzzles and
the one-time hints, through the bot's moves of each mode, to the game's end, the high scores and back to the menu. Every
step is real input (clicks and keys through -realinput, one burst of move, press and release for a click); the test commands
only read the state ("state"), ask the bot for its move ("move"), open every mode ("unlock all") and end a game ("lose", the
game's own ending; the puzzle modes end by "goto menu"). The Driver starts the game under rundesk.

    python games/bejeweled2/tools/realplay/full.py --exe <exe> --userdir <a new folder> --out <dir>
        [--modes classic,action,puzzle,endless,twilight,hyper,cognito,finity] [--moves N] [--fullscreen] [--no3d] [--3d]

Prints one line for each mode and "full: ok" (exit status 0) when every mode went through.
"""
import argparse
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from driver_base import Driver  # noqa: E402

SECRET = {'twilight': 'classic', 'hyper': 'action', 'cognito': 'puzzle', 'finity': 'endless'}
PUZZLE_MODES = ('puzzle', 'cognito')
NAME = 'Ali'
TEXT_DIALOGS = (10, 27, 41)  # a high score's name, a new player's name, "Enter Name"


class Failure(Exception):
    pass


def describe(s):
    return json.dumps({k: s.get(k) for k in ('screen', 'dialogs', 'menu', 'mode', 'phase', 'level', 'score', 'tutorial', 'busy')})


def until(d, pred, what, timeout=120, step=0.02):
    deadline = time.time() + timeout
    s = d.state()
    while not pred(s):
        if time.time() > deadline:
            raise Failure('timeout waiting for %s: %s' % (what, describe(s)))
        d.idle(step)
        s = d.state()
    return s


_last_button = [None]
KEY_HINTS = False  # set by origcmp/record.py: the one-time hint dialogs (25, 26, 38, 39) are closed with Escape, not by a click on a button
HINT_DIALOGS = (25, 26, 38, 39)


def handle_dialog(d, s):
    """Answers the dialog on top by real input: a name by typing it, any other by the bot's click on its button (the bot
    answers only the messages in front of a game; one in front of the menu, such as the welcome, is answered by a click on its
    yes button, or its no button when it has no other, once the button has stood still for two questions)."""
    top = max(s['dialogs'])
    if top in TEXT_DIALOGS:
        d.type(NAME)
        d.key('Return')
    else:
        m = d.bot_move()
        if m is not None and m.get('kind') == 'dialog' and KEY_HINTS and top in HINT_DIALOGS:
            d.key('Escape')  # origcmp: the one-time hints are closed by the key, since the port at 640 by 480 lays them out as the 1024 layout scaled
        elif m is not None and m.get('kind') in ('dialog', 'wait'):
            d.play_input(m['input'])  # the click on its button, or a wait while it slides in or out
        elif m is None and s.get('buttons'):
            b = s['buttons'].get('yes') or s['buttons'].get('no')
            if _last_button[0] == (top, tuple(b)):
                d.click(b)
                _last_button[0] = None
            else:
                _last_button[0] = (top, tuple(b))
                d.idle(0.05)
                return
        else:
            d.idle(0.05)
            return
    d.idle(0.05)


def menu_ready(s):
    return (s['screen'] == 'menu' and s.get('menu') == 1 and not s['dialogs'] and s.get('user') and s.get('widgets', {}).get('classic')
            and s['widgets'].get('classic') != [0, 0])


def to_menu(d, timeout=180):
    """From wherever the game is (the title, a dialog, the high scores) to the menu, by clicks."""
    deadline = time.time() + timeout
    while True:
        s = d.state()
        if menu_ready(s):
            return s
        if time.time() > deadline:
            raise Failure('timeout waiting for the menu: ' + describe(s))
        w = s.get('widgets', {})
        if s['dialogs']:
            handle_dialog(d, s)
        elif s['screen'] == 'title' and w.get('play'):
            d.click(w['play'])
            d.idle(0.2)
        elif s['screen'] == 'highscores' and w.get('back'):
            d.click(w['back'])
            d.idle(0.2)
        else:
            d.idle()


def start_mode(d, mode):
    """Clicks the mode's button (the secret button first for a secret mode) until the game of the mode is on."""
    base = SECRET.get(mode, mode)
    deadline = time.time() + 90
    idle = 0
    while True:
        s = d.state()
        if s.get('mode') == mode and s['screen'] in ('game', 'dialog'):
            return s
        if time.time() > deadline:
            raise Failure('timeout waiting for the game of %s: %s' % (mode, describe(s)))
        if s['dialogs'] and s.get('mode') is None:
            handle_dialog(d, s)  # a message that came after the menu (the welcome)
            idle = 0
        elif menu_ready(s) and idle == 0:
            if (mode in SECRET) != bool(s.get('secret')):
                d.click(s['widgets']['secret'])
            else:
                d.click(s['widgets'][base])
            idle = 1
        else:
            d.idle()
            idle = idle + 1 if idle < 40 else 0  # a click that did nothing is made again after a while


def play(d, mode, moves, out):
    """The bot's moves as real input until `moves` swaps (or clicks of the tutorial) were made; returns what was seen."""
    made = nulls = 0
    levels = set()
    log = open(os.path.join(out, 'moves-%s.log' % mode), 'w')
    t0 = time.time()
    while made < moves:
        s = d.state()
        if s['screen'] == 'menu' and s.get('mode') == mode:
            d.idle()  # the board is still sliding in (or the menu sliding out)
            continue
        if s['screen'] not in ('game', 'dialog'):
            print('  %s: left the game early: %s' % (mode, describe(s)))
            break
        if s['dialogs']:
            handle_dialog(d, s)
            continue
        if s.get('over'):
            print('  %s: the game is over: %s' % (mode, describe(s)))
            break
        levels.add(s['level'])
        m = d.bot_move()
        if m is None:
            nulls += 1
            if nulls > 400:
                raise Failure('no move for a long time: ' + describe(s))
            d.idle(0.01)
            continue
        nulls = 0
        d.play_input(m['input'])
        if m['kind'] == 'wait':
            continue
        made += 1
        log.write('%d %s level %d score %d phase %s tick %d\n' % (made, m['kind'], s['level'], s['score'], s['phase'], s['tick']))
        log.flush()
    s = d.state()
    log.close()
    return {'moves': made, 'levels': sorted(levels), 'score': s.get('score'), 'level': s.get('level'), 'seconds': round(time.time() - t0, 1)}


def end_game(d, mode):
    """The game's end by its own code ("lose"; a puzzle mode has none and goes to the menu), then the high scores and back."""
    s = d.state()
    ended = s['screen'] != 'game' or s.get('over')  # an action game ends by itself when its time is up
    if not ended:
        if mode in PUZZLE_MODES:
            d.ok('goto menu')
        else:
            d.ok('lose')
    to_menu(d)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--gamedir', default='')
    ap.add_argument('--userdir', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--modes', default='classic,action,puzzle,endless,twilight,hyper,cognito,finity')
    ap.add_argument('--moves', type=int, default=120)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--fullscreen', action='store_true')
    ap.add_argument('--3d', dest='three_d', action='store_true', help='the 3D setting on (the saved "Is3D"); the game default otherwise')
    ap.add_argument('--no3d', action='store_true')
    ap.add_argument('--realtime', action='store_true', help='the game on its own clock (100 updates a second, no -fast, no -hold)')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    is3d = False if a.no3d else (True if a.three_d else None)
    d = Driver(a.exe, a.gamedir, a.userdir, a.out, not a.realtime, ['-seed=%d' % a.seed], windowed=not a.fullscreen, is3d=is3d,
               hold=not a.realtime)
    rc = 1
    try:
        until(d, lambda s: s['screen'] in ('title', 'dialog', 'menu'), 'the title', 180)
        s = to_menu(d)
        d.ok('unlock all')
        print('menu reached, windowed %s' % s.get('windowed'))
        for mode in a.modes.split(','):
            start_mode(d, mode)
            r = play(d, mode, a.moves, a.out)
            d.shot('end-' + mode)
            end_game(d, mode)
            print('%-9s %s' % (mode, json.dumps(r)))
            sys.stdout.flush()
        print('full: ok')
        rc = 0
    except Failure as e:
        print('full: FAILED: %s' % e)
        try:
            d.shot('failure')
        except Exception:
            pass
    finally:
        d.close()
    return rc


if __name__ == '__main__':
    sys.exit(main())
