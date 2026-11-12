"""Short scripted games for the autosplit block's replay logs (-livesplitlog): the port's own bot plays by real input through
tools/playbymove's loop, and a `lose` ends the game.

    python -I games/astropop/tools/livesplit/scripted.py <kind> <out dir>

kinds:
  classic    a classic game from level 1 with Sprocket (pilot 1): the bot plays until two levels are cleared (level 3 on the screen)
             or 1500 moves, then the game is lost (`lose`: a game over of kind 3, "other", the test command's)
  survival   a survival game with Turbot (pilot 2): 80 moves, then `lose`
  supa       a classic game with Vector (pilot 0) whose weapon starts charged (`set chargestart 18000`): the bot starts the SUPA
             weapon at once; 60 moves, then `lose`
  levels     a classic game with Vector (pilot 0) whose levels are cleared by the test command `win`, two bot moves into each, and
             the bot takes the warp screen's gate and the boxes on the way: until level 33 is on the screen (32 level clears, the
             Any% category's length) or 20000 moves, then `lose`
The run folder holds game.log (the observer's `livesplit:` lines among the game's), block.log (-livesplitlog: a line each time the
block changes but for the clocks and the scores) and driver.txt (every command sent and its answer).
"""
import json
import os
import sys
import types

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'playbymove'))
import playbymove as pbm          # noqa: E402

KINDS = {
    #            first commands                                    moves  stop at level  end with `lose`  `win` after n moves on a level
    'classic':  (['goto classic 1 1'],                              1500,  3,             True,            None),
    'survival': (['goto survival 2'],                               80,    None,          True,            None),
    'supa':     (['set chargestart 18000', 'goto classic 1 0'],     60,    None,          True,            None),
    'levels':   (['goto classic 1 0'],                              20000, 33,            True,            2),
}


def play(kind, out, exe):
    firsts, limit, stop_level, lose, win_after = KINDS[kind]
    os.makedirs(out, exist_ok=True)
    args = types.SimpleNamespace(dir=out, exe=exe, timeout=120, realtime=False,
                                 game_args=['-seed=1', '-userdir=' + os.path.join(out, 'user'),
                                            '-livesplitlog=' + os.path.join(out, 'block.log')])
    game = pbm.Game(args)
    moves, state, stop = 0, None, None
    level, on_level, won, want_win = 1, 0, False, False
    try:
        for line in firsts:
            game.send(line)
        batch = []
        nulls = 0
        while True:
            if want_win:
                # `win` is answered with an ERROR unless the level is in play (a level with no brick taken yet, a level-up, a bonus
                # round...): then it is asked again after the next shot
                want_win = False
                try:
                    game.send(*(batch + ['win']))
                    won = True
                    batch = ['wait 20']
                except pbm.Stop as e:
                    if not str(e).startswith('error: ERROR win'):
                        raise
                    batch = []
            answers = game.send(*(batch + ['state', 'move']))
            state = answers[-2][len('STATE '):]
            st = json.loads(state)
            if st.get('over') is True:
                stop = 'over'
                break
            if moves >= limit:
                stop = 'limit'
                break
            if stop_level is not None and st.get('level', 0) >= stop_level:
                stop = 'level %d' % st['level']
                break
            if st.get('level', 0) != level:
                level, on_level, won, want_win = st.get('level', 0), 0, False, False
            m = json.loads(answers[-1][len('MOVE '):])
            if m is None:
                nulls += 1
                if nulls >= 20:
                    stop = 'no move'
                    break
                batch = ['wait 30']
                continue
            nulls = 0
            moves += 1
            batch = []
            for line in pbm.move_lines(m):
                if batch and not batch[-1].startswith('wait ') and not line.startswith('wait '):
                    batch.append('wait 1')
                batch.append(line)
            updates = m.get('updates', 1)
            batch.append('wait %d' % (updates if isinstance(updates, int) and updates > 0 else 1))
            on_level += 1
            if win_after is not None and not won and on_level >= win_after and m.get('kind') == 'shoot':
                want_win = True
        if lose and stop != 'over':
            game.send('lose')
            game.send('wait 10', 'state')
    except pbm.Stop as e:
        stop = str(e)
    finally:
        game.finish(None)
    print('%s: %d moves, stop %s; STATE %s' % (kind, moves, stop, state))
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in KINDS:
        print(__doc__)
        sys.exit(2)
    sys.exit(play(sys.argv[1], sys.argv[2], os.path.join(ROOT, 'build', 'native', 'astropop.exe')))
