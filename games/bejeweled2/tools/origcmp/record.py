"""Phase one of the identical-play pass (docs/COORDINATION.md): the port plays a mode by its own bot with real input, in 2D at
640x480 (what the original does on rundesk's desktop, which has no 3D), and this writes down every input with the update at
which it ran, so that the same inputs can be given to the original at the same updates (origcmp/to_origrun.py) and to the
port again. The port's -trace lines (the board's score, level, state and generator position, as origrun's --watch reads them in
the original) stay in game.log.

    python games/bejeweled2/tools/origcmp/record.py --exe <exe> --userdir <a new folder> --out <dir> --mode classic [--moves 60]

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

INPUT_VERBS = ('move', 'click', 'down', 'up', 'key', 'type')


class Recorder(Driver):
    """A Driver that notes the update at which each input command runs (a command sent after a state reply runs one update
    later: -hold lets one update run for each new line)."""

    def __init__(self, *a, **k):
        super().__init__(*a, **k)
        self.events = []

    def ok(self, line):
        if line.split()[0] in INPUT_VERBS:
            u = self.state()['updates']
            self.events.append([u + 1, line])
        super().ok(line)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--gamedir', default='')
    ap.add_argument('--userdir', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--mode', default='classic')
    ap.add_argument('--moves', type=int, default=60)
    ap.add_argument('--seed', type=int, default=1)
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    d = Recorder(a.exe, a.gamedir, a.userdir, a.out, True, ['-seed=%d' % a.seed, '-trace'], windowed=True, is3d=False, hold=True)
    rc = 1
    try:
        full.until(d, lambda s: s['screen'] in ('title', 'dialog', 'menu'), 'the title', 180)
        full.to_menu(d)
        play = d.events[0][0]
        if a.mode in full.SECRET:
            d.ok('unlock all')
        full.start_mode(d, a.mode)
        start = d.events[-1][0]
        r = full.play(d, a.mode, a.moves, a.out)
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
