"""Plays a recording of origcmp/record.py again on the port (the same flags, so the same game) and prints the port's board
(gem types and flame gems) at the updates asked for, for looking at the board of a difference that the original's watches show only
as numbers.

    python games/bejeweled2/tools/origcmp/replay.py --exe build/native/bejeweled2.exe --events <dir>/events.json --userdir <new folder>
        [--lowres] [--nosparkle] [--profile <make_profile.py's folder>] --dump 2580,2627
"""
import argparse
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'realplay'))
import full  # noqa: E402
from driver_base import Driver  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--events', required=True)
    ap.add_argument('--userdir', required=True)
    ap.add_argument('--out', default=None)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--lowres', action='store_true')
    ap.add_argument('--nosparkle', action='store_true')
    ap.add_argument('--profile', default=None)
    ap.add_argument('--dump', default='')
    a = ap.parse_args()
    ev = json.load(open(a.events))
    out = a.out or a.userdir + '.out'
    os.makedirs(out, exist_ok=True)
    shutil.rmtree(a.userdir, ignore_errors=True)
    if a.profile:
        os.makedirs(a.userdir)
        shutil.copytree(os.path.join(a.profile, 'users'), os.path.join(a.userdir, 'users'))
    dumps = sorted(int(x) for x in a.dump.split(',') if x)
    d = Driver(a.exe, '', a.userdir, out, True, ['-seed=%d' % a.seed, '-trace'], windowed=True, is3d=False if a.lowres else None, hold=True)
    try:
        full.until(d, lambda s: s['screen'] in ('title', 'dialog', 'menu'), 'the title', 180)
        if a.nosparkle:
            d.ok('nosparkle')
        # every line the game is sent lets one update run it (-hold): a command that the recording has at update U is the line that
        # follows a state reply of U - 1; so this asks for the state until it says U - 1 (a long gap is cut short by a wait)
        todo = [(u, 'cmd', c) for u, c, *_ in ev['events']] + [(u, 'dump', None) for u in dumps]
        todo.sort(key=lambda t: (t[0], 0 if t[1] == 'dump' else 1))
        for u, kind, c in todo:
            want = u - 1 if kind == 'cmd' else u
            while True:
                s = d.state()
                now = s['updates']
                if now == want:
                    break
                if now > want:
                    raise SystemExit('missed update %d (at %d)' % (want, now))
                if want - now > 6:
                    d.ok('wait %d' % (want - now - 5))
            if kind == 'cmd':
                d.ok(c)
            else:
                print('update', s['updates'], 'screen', s.get('screen'), 'dialogs', s.get('dialogs'), 'buttons', s.get('buttons'), 'state', s.get('phase'), 'score', s.get('score'), 'level', s.get('level'))
                for r, row in enumerate(s.get('board', [])):
                    print('   ', row, s.get('flames', [''] * 8)[r])
    finally:
        d.close()


if __name__ == '__main__':
    main()
