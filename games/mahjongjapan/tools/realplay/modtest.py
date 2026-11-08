"""Tests of the fixes mod of Mahjong Escape: Ancient Japan and Ancient China (games/<game>/mods/fixes), by the shipped exe
with real input and the test commands (docs/TEST_COMMANDS.md). One script for both games.

    build/native/rundesk.exe python games/mahjongjapan/tools/realplay/modtest.py <out dir> --game japan|china

logos         updates from launch to the main menu, as the port starts (the logo screens skipped) and with -logos (the
              original's two seconds each); not a mod, the loading rule of the port
seededDeals   the first moves of a classic deal and of its shuffle: the same under one seed whatever the start time,
              another under another seed; without the mod the clock seeds them, so a start delayed by some updates deals
              another board even under the same seed
steadyClock   in real time, the clock on the panel after N updates is N * 5 ms: the timer shows the updates
Prints one PASS or FAIL line for each check and the RESULT line; the exit code is 0 when all pass."""
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mjgame import Failed, Game  # noqa: E402
import drive  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))


class Run:
    """One launch of the game with its own user folder; `mods` is the fixes mod's parent folder or None for -nomods"""

    def __init__(self, out, which, tag, mods, seed=None, fast=True, flags=()):
        exe, gamedir = drive.GAMES[which]
        extra = ['-modsdir=' + os.path.join(ROOT, 'games', 'mahjong' + which, 'mods')] if mods else ['-nomods']
        if seed is not None:
            extra.append('-gameseed=%d' % seed)
        extra.extend(flags)
        self.g = Game(os.path.join(out, tag), os.path.join(ROOT, exe), os.path.join(ROOT, gamedir), fast, extra)

    def to_menu(self):
        """The updates the game took to reach its main menu"""
        st = self.g.until('the main menu', lambda s: s['screen'] == 'menu' and s['dialog'] is None and not s['busy'], 120)
        return st['updates']

    def classic(self, delay=0):
        g = self.g
        self.to_menu()
        drive.new_player(g, 'Bot')
        g.ask('goto menu')
        st = drive.to_screen(g, 'menu')
        drive.press(g, st, drive.MENU_CLASSIC)
        drive.to_screen(g, 'classic list')
        g.hover(*drive.SLOTS[0])
        g.click(*drive.SLOTS[0])
        g.wait(10 + delay)
        drive.press(g, g.state(), drive.CL_PLAY)
        drive.board_ready(g, 'classic')
        g.wait(200)    # the game's tip opens a moment after the board is first ready
        return drive.board_ready(g, 'classic')

    def moves(self, n=8):
        """The bot's first n pairs of the board in front of it, played: their kinds and where they were clicked"""
        g = self.g
        fp = []
        for _ in range(n):
            drive.board_ready(g, 'classic')
            mv = g.ask('move')
            if mv is None or mv['kind'] == 'shuffle':
                break
            fp.append([mv['tiles'], mv['input']])
            g.send(mv)
            g.wait(40)
        return fp

    def close(self):
        self.g.close()


def deal_prints(out, which, tag, mods, seed, delay):
    r = Run(out, which, tag, mods, seed)
    try:
        r.classic(delay)
        before = r.moves()
        g = r.g
        drive.board_ready(g, 'classic')
        drive.press(g, g.state(), drive.BOARD_SHUFFLE)
        g.wait(120)
        drive.board_ready(g, 'classic')
        after = r.moves()
        return before, after
    finally:
        r.close()


def main():
    args = sys.argv[1:]
    out, which = None, 'japan'
    while args:
        a = args.pop(0)
        if a == '--game':
            which = args.pop(0)
        else:
            out = a
    if out is None:
        sys.exit(__doc__)
    results = []

    def check(name, ok, detail):
        results.append(ok)
        print('%s %s: %s' % ('PASS' if ok else 'FAIL', name, detail), flush=True)

    # the logos: skipped by the port itself, with the mod or without
    r = Run(out, which, 'logos-skipped', False)
    try:
        on = r.to_menu()
    finally:
        r.close()
    r = Run(out, which, 'logos-kept', False, flags=['-logos'])
    try:
        off = r.to_menu()
    finally:
        r.close()
    check('logos', 700 <= off - on, 'the menu at update %d as the port starts, %d with -logos (the two logos take 4 s, 800 updates)' % (on, off))

    # seededDeals
    a1 = deal_prints(out, which, 'seed7-a', True, 7, 0)
    a2 = deal_prints(out, which, 'seed7-b', True, 7, 37)
    a3 = deal_prints(out, which, 'seed8', True, 8, 0)
    check('seededDeals: the same seed, another start time', a1 == a2 and len(a1[0]) == 8 and len(a1[1]) == 8,
          'deal %d moves equal %s, shuffle %d moves equal %s' % (len(a1[0]), a1[0] == a2[0], len(a1[1]), a1[1] == a2[1]))
    check('seededDeals: another seed', a1[0] != a3[0] and a1[1] != a3[1],
          'deal differs: %s, shuffle differs: %s' % (a1[0] != a3[0], a1[1] != a3[1]))
    o1 = deal_prints(out, which, 'clock-a', False, 7, 0)
    o2 = deal_prints(out, which, 'clock-b', False, 7, 37)
    check('seededDeals: off, the clock seeds', o1[0] != o2[0],
          'without the mod a start 37 updates later deals another board: %s' % (o1[0] != o2[0]))

    # steadyClock, in real time: the clock starts on the first hint and then the timer shows the updates
    r = Run(out, which, 'steady', True, 7, False)
    try:
        g = r.g
        st = r.classic(0)
        drive.press(g, st, drive.BOARD_HINT)
        u0 = g.state()['updates']
        g.wait(1)
        rows = []
        for span in (400, 800, 1200):
            g.until('updates', lambda s: s['updates'] >= u0 + span, 60, 0.05)
            s = g.state()
            rows.append((s['updates'] - u0, s['timer']))
        ok = all(abs(t - (u * 5) // 1000) <= 1 for u, t in rows)
        check('steadyClock', ok, 'timer after (updates, seconds): %s, expected updates * 5 ms' % json.dumps(rows))
    finally:
        r.close()

    result = 'ok' if all(results) else 'FAILED'
    print('RESULT ' + result)
    return 0 if result == 'ok' else 1


if __name__ == '__main__':
    sys.exit(main())
