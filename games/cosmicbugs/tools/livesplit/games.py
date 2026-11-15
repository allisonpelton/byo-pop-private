#!/usr/bin/env python3
"""Games for the autosplit block's tests (run_tests.sh): the exe under rundesk at -fast with the "livesplit" mod on, played
through real input events (tools/play/drive.py's Game: -realinput and the live command channel), each game's events read from the
exe's own log lines ("livesplit: board 1 GO at tick ...") and from -livesplitlog's block lines.

    build/native/rundesk.exe python games/cosmicbugs/tools/livesplit/games.py <out dir> <mods dir> <run>...

Runs (<out dir>/<run>/block.log is the -livesplitlog, game.log the exe's log):
  game     a new profile: Start Game, the instructions page, Play, board 1 won, board 2 won, board 3 lost (out of lives)
  mission  a new profile at the instructions page, board 577 (the last) started with the "level" test command and won: the
           mission-complete screen
  left     a game left with Esc and Esc on its first board, then a second game from the instructions page, lost
  resume   a game left on its second board, then continued with the resume question's Yes: no GO, RESUMED
  select   a game lost after one board was won, then a board chosen on the level selection: no GO, RESUMED
"""
import json
import os
import re
import shutil
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'play'))
import drive
from drive import Game, Failed, TITLE, PLAY

HALL_BACK = drive.HALL_BACK
# read off the screens: the resume question's OK, the level selection's first world, the mission-complete screen's "click here to
# continue" and the second selection's Play
RESUME_YES = (340, 330)
SELECT_WORLD = (293, 105)
CONTINUE = (400, 520)
SELECT2_PLAY = (400, 560)


def ready(g):
    """the board's ship under control: the pointer is on the board, which a game that has just begun needs"""
    g.until('a board', lambda s: s['screen'] == 'board', 120)
    g.wait(20)
    g.move(300, 250)
    g.until('the ship under control', lambda s: s['screen'] == 'board' and s['playing'], 120)


def new_game(g):
    """from the title of a profile that has reached no board: Start Game, the instructions page, Play"""
    g.until('the title', lambda s: s['screen'] == 'title', 240)
    g.wait(30)
    g.click(*TITLE['start'])
    g.until('the instructions', lambda s: s['screen'] == 'instructions')
    g.wait(30)
    g.click(*PLAY)
    ready(g)


def clear_board(g):
    """the board's own ending (the fill reaches its quota), then the clicks through its texts to the next screen"""
    level = g.state()['level']
    g.ask('win')
    g.until('the board won', lambda s: s.get('won') or s['screen'] != 'board', 120)
    for _ in range(80):
        st = g.state()
        if st['screen'] != 'board' or st.get('level') != level:
            break
        g.click(400, 300)
        g.wait(60)
    else:
        raise Failed('the won board %d did not end' % level)


def lose(g):
    """the game over of the last life, the click that leaves it, the hall of fame and its Back button"""
    ready(g)
    g.ask('lose')
    g.until('the game over', lambda s: s['screen'] == 'board' and s.get('over'), 60)
    g.wait(60)
    g.click(400, 300)
    g.until('the hall of fame', lambda s: s['screen'] in ('highscores', 'highscore_entry'), 60)
    g.wait(30)
    g.click(*HALL_BACK)
    g.until('the title', lambda s: s['screen'] == 'title', 60)


def leave(g):
    """Esc pauses the board, Esc again goes to the title (the game is saved there)"""
    g.key('Escape')
    g.until('paused', lambda s: s.get('paused'))
    g.key('Escape')
    g.until('the title', lambda s: s['screen'] == 'title', 60)
    g.wait(30)


def run_game(g):
    new_game(g)
    clear_board(g)
    ready(g)
    clear_board(g)
    lose(g)


def run_mission(g):
    g.until('the title', lambda s: s['screen'] == 'title', 240)
    g.wait(30)
    g.ask('goto instructions')
    g.until('the instructions', lambda s: s['screen'] == 'instructions')
    g.wait(30)
    g.ask('level 577')
    ready(g)
    g.ask('win')
    g.until('the board won', lambda s: s.get('won') or s['screen'] != 'board', 120)
    for _ in range(80):
        st = g.state()
        if st['screen'] == 'mission_complete':
            break
        g.click(400, 300)
        g.wait(60)
    else:
        raise Failed('the mission-complete screen did not come')
    g.wait(100)
    g.shot('mission_complete')
    g.click(*CONTINUE)
    g.until('a screen after it', lambda s: s['screen'] != 'mission_complete', 60)


def run_left(g):
    new_game(g)
    leave(g)
    g.click(*TITLE['start'])
    g.until('the instructions', lambda s: s['screen'] == 'instructions')
    g.wait(30)
    g.click(*PLAY)
    ready(g)
    lose(g)


def run_resume(g):
    new_game(g)
    clear_board(g)
    ready(g)
    leave(g)
    g.click(*TITLE['start'])
    g.until('the resume question', lambda s: s['screen'] == 'title' and s.get('requester'), 60)
    g.wait(30)
    g.shot('resume_question')
    g.click(*RESUME_YES)
    ready(g)
    g.wait(100)


def run_select(g):
    new_game(g)
    clear_board(g)
    lose(g)
    g.click(*TITLE['start'])
    g.until('the level selection', lambda s: s['screen'] == 'levelselect', 60)
    g.wait(60)
    g.shot('level_selection')
    g.click(*SELECT_WORLD)
    g.until('the second level selection', lambda s: s['screen'] == 'levelselect2', 60)
    g.wait(60)
    g.shot('level_selection2')
    g.click(*SELECT2_PLAY)
    ready(g)
    g.wait(100)


RUNS = {'game': run_game, 'mission': run_mission, 'left': run_left, 'resume': run_resume, 'select': run_select}


def main():
    out, mods = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
    ok = True
    for name in sys.argv[3:]:
        d = os.path.join(out, name)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        g = Game(d, os.path.abspath('build/native/cosmicbugs.exe'), True, 'win1x',
                 extra_args=['-modsdir=' + mods, '-livesplitlog=' + os.path.join(d, 'block.log')])
        result = 'ok'
        try:
            RUNS[name](g)
        except Failed as e:
            result = 'FAILED: %s' % e
            try:
                g.shot('failed')
            except Failed:
                pass
        finally:
            g.close()
        g.say('RESULT ' + result)
        print('%s: RESULT %s' % (name, result), flush=True)
        ok = ok and result == 'ok'
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
