"""A first driver for Bejeweled 2 (a soak, not the final realplay): launches the game under the commands channel with real
input, gets to a classic game through the title, the new-user entry, the welcome and how-to-play dialogs, then plays N of the
bot's moves (the test command "move") as real mouse input, reporting the level and score as it goes.

    build/native/rundesk.exe python games/bejeweled2/tools/realplay/soak.py --exe <exe> --out <dir> [--moves N] [--mode classic]
"""
import argparse
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from driver_base import Driver  # noqa: E402


def wait_screen(d, screens, timeout=120):
    deadline = time.time() + timeout
    s = d.state()
    while s['screen'] not in screens:
        if time.time() > deadline:
            raise RuntimeError('screen %s, wanted %s' % (s['screen'], '/'.join(screens)))
        time.sleep(0.02)
        s = d.state()
    return s


def wait_state(d, pred, what, timeout=120):
    deadline = time.time() + timeout
    s = d.state()
    while not pred(s):
        if time.time() > deadline:
            raise RuntimeError('timeout waiting for %s (screen %s, dialogs %s, menu %s)' % (what, s['screen'], s.get('dialogs'), s.get('menu')))
        time.sleep(0.02)
        s = d.state()
    return s


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--gamedir', default='')
    ap.add_argument('--userdir', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--moves', type=int, default=200)
    ap.add_argument('--mode', default='classic')
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--realtime', action='store_true')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    extra = ['-seed=%d' % a.seed]
    d = Driver(a.exe, a.gamedir, a.userdir, a.out, not a.realtime, extra)
    rc = 1
    try:
        wait_screen(d, ('title',), 180)
        d.ok('goto menu')  # the title screen's play link
        # the new user's name, then the welcome dialog (the menu is behind both)
        wait_state(d, lambda s: s['screen'] == 'dialog' and 27 in s['dialogs'], 'the new-user dialog')
        d.type('Ali')
        d.key('Return')
        wait_state(d, lambda s: s['screen'] == 'dialog' and 36 in s['dialogs'], 'the welcome dialog')
        d.click((510, 270))
        wait_state(d, lambda s: s['screen'] == 'menu' and s.get('menu') == 1 and not s['dialogs'], 'the menu', 60)
        d.ok('unlock all')
        d.ok('goto ' + a.mode)
        s = wait_state(d, lambda s: s['screen'] in ('game', 'dialog'), 'the game')
        if s['screen'] == 'dialog':
            d.click((740, 678))  # How To Play: Continue
        s = wait_state(d, lambda s: s['screen'] == 'game' and not s['busy'], 'the first move', 120)
        d.shot('start')
        made = nulls = 0
        movelog = open(os.path.join(a.out, 'moves.log'), 'w')
        levels = set()
        t0 = time.time()
        while made < a.moves:
            s = d.state()
            if s['screen'] == 'dialog' and s.get('buttons'):
                # a one-time hint ("Power Gem" and the like): its first button, by a click
                b = s['buttons']
                d.click(b.get('yes') or b.get('no'))
                time.sleep(0.05)
                continue
            if s['screen'] != 'game':
                break
            levels.add(s['level'])
            m = d.bot_move()
            if m is None:
                nulls += 1
                if nulls > 300:
                    raise RuntimeError('no move for a long time: ' + json.dumps({k: s.get(k) for k in ('phase', 'busy', 'screen')}))
                time.sleep(0.01)
                continue
            nulls = 0
            d.play_input(m['input'])
            if m['kind'] == 'wait':
                continue
            made += 1
            movelog.write('%d level %d score %d phase %s tick %d\n' % (made, s['level'], s['score'], s['phase'], s['tick']))
            movelog.flush()
            if made % 50 == 0:
                print('moves %d level %d score %d phase %s' % (made, s['level'], s['score'], s['phase']))
                sys.stdout.flush()
        s = d.state()
        d.shot('end')
        print('soak: %d moves in %.0f s, levels %s, final screen %s score %s level %s' % (
            made, time.time() - t0, sorted(levels), s['screen'], s.get('score'), s.get('level')))
        rc = 0
    finally:
        d.close()
    return rc


if __name__ == '__main__':
    sys.exit(main())
