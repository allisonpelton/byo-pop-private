"""How long the port takes from its launch to its main menu, in real time (not -fast): the process is started, and the
test command `state` asks until the menu is drawn and settled. The port loads at the machine's speed (a few files a frame
only where the window must keep drawing), so this is the loading the player waits for. N runs, medians printed.

    build/native/rundesk.exe python games/mahjongjapan/tools/realplay/perfload.py <out dir> --game japan|china [--runs 5] [--mods] [--logos]"""
import os
import statistics
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mjgame import Game  # noqa: E402
import drive  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))


def main():
    args = sys.argv[1:]
    out, which, runs, mods, logos = None, 'japan', 5, False, False
    while args:
        a = args.pop(0)
        if a == '--game':
            which = args.pop(0)
        elif a == '--runs':
            runs = int(args.pop(0))
        elif a == '--mods':
            mods = True    # the fixes mod on (steadyClock and seededDeals; the loading is the same)
        elif a == '--logos':
            logos = True   # the original's two 2 s logo screens kept
        else:
            out = a
    if out is None:
        sys.exit(__doc__)
    exe, gamedir = drive.GAMES[which]
    seconds = []
    for i in range(runs):
        g = Game(os.path.join(out, 'run%d' % i), os.path.join(ROOT, exe), os.path.join(ROOT, gamedir), False,
                 (['-modsdir=' + os.path.join(ROOT, 'games', 'mahjong' + which, 'mods')] if mods else ['-nomods']) + (['-logos'] if logos else []))
        t0 = time.time()
        try:
            st = g.until('the main menu', lambda s: s['screen'] == 'menu' and s['dialog'] is None and not s['busy'], 120, 0.01)
            took = time.time() - t0
            seconds.append(took)
            print('run %d: the menu after %.2f s (update %d)' % (i, took, st['updates']), flush=True)
        finally:
            g.close()
    print('median %.2f s (%.2f to %.2f) of %d runs' % (statistics.median(seconds), min(seconds), max(seconds), len(seconds)))


if __name__ == '__main__':
    main()
