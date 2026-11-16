"""The lite mod's measurement in the game itself (docs/LITE_MOD.md): escape stage 9 played by the port's bot (`move`) until a
quarter (--played, in percent) of its tiles have gone, then the Shuffle button clicked N times, each shuffle's cost counted on its own. Run it under rundesk, one run each of the original path and of the lite mod's, in turn:

    build/native/rundesk.exe python games/mahjongjapan/tools/realplay/shuffletest.py <out dir> --game japan|china
        [--lite] [--level N] [--shuffles N]
        [--played PERCENT]

It prints "SHUFFLE <json>": for each of N shuffles in a row on the same board, the process cycles, the wall time and the
updates from the click to the board at rest again (-cyclesplit's snapshots; the live channel runs on both sides alike), the
tiles before and after, and the option as the game's log read it. A shuffle redeals the same tiles; its search is up to a
second of the machine's clock in the original and a fixed number of steps with the lite mod's quickRedeal.
"""
import json
import os
import shutil
import sys
import time

sys.path.insert(0, os.path.dirname(__file__))
from mjgame import Failed, Game  # noqa: E402

GAMES = {'japan': ('build/native/mahjongjapan.exe', 'assets/mahjongjapan'),
         'china': ('build/native/mahjongchina.exe', 'assets/mahjongchina')}
BOARD_SHUFFLE = (0x94, 0x10c)  # the escape board's, the classic board's


def main():
    out = sys.argv[1]
    game = sys.argv[sys.argv.index('--game') + 1] if '--game' in sys.argv else 'japan'
    level = int(sys.argv[sys.argv.index('--level') + 1]) if '--level' in sys.argv else 9
    lite = '--lite' in sys.argv
    played = int(sys.argv[sys.argv.index('--played') + 1]) if '--played' in sys.argv else 25  # percent of the tiles played first
    shuffles = int(sys.argv[sys.argv.index('--shuffles') + 1]) if '--shuffles' in sys.argv else 12
    exe, gamedir = GAMES[game]
    shutil.rmtree(os.path.join(out, 'user'), ignore_errors=True)  # `set player` needs a user folder with no player yet
    os.makedirs(out, exist_ok=True)
    split = os.path.abspath(os.path.join(out, 'split.jsonl'))
    g = Game(out, os.path.abspath(exe), gamedir, fast=True, extra=['-cyclesplit=' + split] + (['-lite=quickRedeal'] if lite else []))
    try:
        g.ask('set player Lite')
        g.ask('level %d' % level)
        st = g.until('the board', lambda s: s['screen'] == 'escape' and not s['busy'] and s['board'] and s['board']['started'] and s['tiles'] > 0)
        goal = st['tiles'] * (100 - played) // 100
        stuck = 0
        while st['tiles'] > goal:
            if st['busy']:
                g.wait(3)
            else:
                mv = g.ask('move')
                if mv is None:
                    g.wait(3)
                else:
                    g.send(mv)
                    g.wait(5)
            st = g.state()
            stuck += 1
            if stuck > 4000:
                raise Failed('the bot did not play down to %d tiles' % goal)
        st = g.settle('the board at rest')
        before = {'tiles': st['tiles'], 'moves': st['moves']}
        rounds = []
        for i in range(shuffles):
            st = g.settle('the board at rest')
            button = [b for b in st['buttons'] if b['cmd'] in BOARD_SHUFFLE][0]
            g.ask('cyclesplit b%d' % i)
            x, y = button['x'], button['y']  # the state lists a button's centre
            g.hover(x, y)  # a button takes a click only when the pointer was over it
            g.click(x, y)
            g.wait(3)
            g.settle('the shuffle over')
            g.ask('cyclesplit a%d' % i)
        st = g.state()
        after = {'tiles': st['tiles'], 'moves': st['moves']}
        g.ask('quit', 10)
        time.sleep(1.0)
        snaps = {}
        for l in open(split):
            d = json.loads(l)
            snaps[d['label']] = d
        for i in range(shuffles):
            b, a = snaps['b%d' % i], snaps['a%d' % i]
            rounds.append({'cycles_G': round((a['process_cycles'] - b['process_cycles']) / 1e9, 3),
                           'wall_ms': round((a['wall_s'] - b['wall_s']) * 1000), 'updates': a['update'] - b['update']})
        log = open(os.path.join(out, 'game.log'), errors='replace').read()
        print('SHUFFLE ' + json.dumps({'lite': lite, 'level': level, 'before': before, 'after': after, 'log_option': 'quickRedeal' in log,
                                       'shuffles': rounds}))
    finally:
        g.close()


if __name__ == '__main__':
    main()
