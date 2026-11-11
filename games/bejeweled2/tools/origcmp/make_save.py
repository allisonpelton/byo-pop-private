"""Makes the player "Ali" with a saved Classic game at a chosen level, flame gems and hyper cubes on its board, as a folder
of the game's own save files (users/Ali/profile.dat and normal.sav), so that the original and the port can both load it and
the scores of a level that no bot game reaches are compared (record.py --mode load, with PROFILE=<folder> on both sides).

    python games/bejeweled2/tools/origcmp/make_save.py --exe build/native/bejeweled2.exe --out build/logs/bej2/save10 --level 10

The port plays to the menu with real input, starts Classic, goes to the level with the test command `level <n>`, and leaves
by the game's own menu button, which saves the game. The save is then edited: --flames and --hypers name squares as
"col,row col,row ..." (the flame flag is the byte at +0x70 of a gem's 0xe0 bytes, a hyper cube is type 9); by default four
pairs of neighbouring flame gems and three hyper cubes. Writes <out>/users/... and prints the board. Runs under rundesk (the
Driver does that), in 2D at 640x480 like the other recordings.
"""
import argparse
import os
import shutil
import struct
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'realplay'))
import full  # noqa: E402
from driver_base import Driver  # noqa: E402

PIECE = 0xe0


def cells(text):
    return [tuple(int(v) for v in one.split(',')) for one in text.split()]


def edit(path, flames, hypers):
    """The saved game: the version (4), the generator's state as a string (a short length and 0x9c0 bytes), the backdrop,
    the level and the next level, then 64 squares in rows: a byte that says whether a gem is there and its 0xe0 bytes."""
    data = bytearray(open(path, 'rb').read())
    version, length = struct.unpack_from('<IH', data, 0)
    if version != 4 or length != 0x9c0:
        raise SystemExit('not a version 4 saved game: %s' % path)
    at = 6 + length + 12
    where = {}
    for row in range(8):
        for col in range(8):
            has = data[at]
            at += 1
            if has:
                where[(col, row)] = at
                at += PIECE
    for c in flames:
        data[where[c] + 0x70] = 1
    for c in hypers:
        struct.pack_into('<i', data, where[c], 9)
        data[where[c] + 0x70] = 0
    open(path, 'wb').write(bytes(data))
    rows = []
    for row in range(8):
        line = ''
        for col in range(8):
            if (col, row) not in where:
                line += '.'
                continue
            t = struct.unpack_from('<i', data, where[(col, row)])[0]
            line += ('f' if data[where[(col, row)] + 0x70] else '') + ('%x' % t if t < 16 else '?') + ' '
        rows.append(line)
    return struct.unpack_from('<i', data, 6 + length + 4)[0], rows


def settle(d, pred, what, timeout):
    """Waits for a state of the game, answering the messages that come up on the way."""
    deadline = time.time() + timeout
    while True:
        s = d.state()
        if s['dialogs']:
            full.handle_dialog(d, s)
        elif pred(s):
            return s
        else:
            d.idle()
        if time.time() > deadline:
            raise SystemExit('timeout waiting for %s: %s' % (what, full.describe(s)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--level', type=int, default=10)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--flames', default='1,1 2,1 5,2 5,3 2,5 3,5 6,6 6,7')
    ap.add_argument('--hypers', default='0,4 4,0 7,3')
    a = ap.parse_args()
    user = a.out + '.user'
    shutil.rmtree(user, ignore_errors=True)
    os.makedirs(a.out, exist_ok=True)
    d = Driver(a.exe, '', user, a.out, True, ['-seed=%d' % a.seed], windowed=True, is3d=False, hold=True)
    try:
        full.until(d, lambda s: s['screen'] in ('title', 'dialog', 'menu'), 'the title', 180)
        full.to_menu(d)
        full.start_mode(d, 'classic')
        s = settle(d, lambda s: s['screen'] == 'game' and s.get('phase') == 'idle', 'the board', 120)
        if a.level > 1:
            d.ok('level %d' % a.level)
            s = settle(d, lambda s: s.get('level') == a.level and s.get('phase') == 'idle', 'level %d' % a.level, 240)
        print('level', s['level'], 'score', s['score'])
        d.click(s['widgets']['menu'])
        s = full.until(d, lambda s: 2 in s['dialogs'] and 'mainmenu' in (s.get('buttons') or {}), 'the options dialog', 60)
        d.click(s['buttons']['mainmenu'])
        full.to_menu(d)
    finally:
        d.close()
    src = os.path.join(user, 'users')
    dst = os.path.join(a.out, 'users')
    shutil.rmtree(dst, ignore_errors=True)
    shutil.copytree(src, dst)
    saves = [os.path.join(r, f) for r, _, fs in os.walk(dst) for f in fs if f == 'normal.sav']
    if len(saves) != 1:
        raise SystemExit('no saved game was written: %s' % saves)
    level, rows = edit(saves[0], cells(a.flames), cells(a.hypers))
    print('saved game at level index %d (level %d):' % (level, level + 1))
    for r in rows:
        print('  ' + r)
    return 0


if __name__ == '__main__':
    sys.exit(main())
