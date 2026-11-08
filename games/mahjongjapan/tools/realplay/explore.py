"""Runs a few steps against a game and prints what the game says, to look at its screens and buttons (not a test).

    build/native/rundesk.exe python games/mahjongjapan/tools/realplay/explore.py <out dir> <japan|china> step ...

A step is one of: state, move, wait N, shot NAME, or any live command line in quotes ("click game 400 300")."""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from mjgame import Game, Failed  # noqa: E402

GAMES = {'japan': ('build/native/mahjongjapan.exe', 'assets/mahjongjapan'),
         'china': ('build/native/mahjongchina.exe', 'assets/mahjongchina')}


def main():
    out, which, steps = sys.argv[1], sys.argv[2], sys.argv[3:]
    exe, gamedir = GAMES[which]
    g = Game(out, os.path.abspath(exe), gamedir)
    try:
        for step in steps:
            word = step.split()[0]
            if word == 'state':
                print(json.dumps(g.state()))
            elif word == 'move':
                print('MOVE', json.dumps(g.ask('move')))
            elif word == 'wait':
                g.wait(int(step.split()[1]))
            elif word == 'shot':
                g.shot(step.split()[1])
            elif word == 'until':
                want = step.split(None, 1)[1]
                print(json.dumps(g.until(want, lambda st: st['screen'] == want and not st['busy'], 120.0)))
            elif word == 'settle':
                print(json.dumps(g.settle()))
            else:
                print(step, '->', g.ask(step))
    except Failed as e:
        print('FAILED', e)
    finally:
        g.close()


if __name__ == '__main__':
    main()
