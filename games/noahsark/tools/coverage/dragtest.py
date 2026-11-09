"""The port's puzzle drag, by real input (the coverage check, GAME_NOTES.md "Coverage check"): origrun has no drag command, so
the original's Board::MouseDrag (0x40f710) is checked by reading it, and the port's by this run.

    build/native/rundesk.exe python games/noahsark/tools/coverage/dragtest.py --exe <abs exe> --gamedir <abs> --userdir <abs> --out <dir>

It plays the port's menu with the bot's `move` (tools/realplay/drive.py's Game) until a puzzle game is on its board and playing, then
presses on one animal, moves to a neighbour of another kind and releases (down, move, up: `down game X Y`...). The drag must leave
the board in the swap phase with the two animals' kinds traded. A drag onto a cell two away must drop the pick and swap nothing; a
press, a move to the neighbour and a release in action mode is not tried here (the link drag has its own run). Prints one JSON line
and exits 0 when both held.
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'realplay'))
import drive  # noqa: E402


def cell(cx, cy):
    # the board's cells are 50 pixels, the first at (178, 16) (Board::CellLeft, CellTop); the middle of the cell
    return 178 + cx * 50 + 25, 16 + cy * 50 + 25


def animal(ch):
    return ch.isdigit() or ch in 'ABC'


def find_pair(board, dx, dy):
    for y in range(9):
        for x in range(9):
            x2, y2 = x + dx, y + dy
            if x2 > 8 or y2 > 8:
                continue
            a, b = board[y][x], board[y2][x2]
            if animal(a) and animal(b) and a != b:
                return (x, y), (x2, y2), a, b
    return None


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--exe', required=True)
    p.add_argument('--gamedir', required=True)
    p.add_argument('--userdir', required=True)
    p.add_argument('--out', required=True)
    a = p.parse_args()
    a.fast = False                                  # real time: with -fast a few thousand updates pass between two lines of mine
    a.exe_args = []
    g = drive.Game(a)
    result = {'checks': []}

    def check(name, ok, detail=''):
        result['checks'].append({'name': name, 'ok': bool(ok), 'detail': detail})

    try:
        for line in ('set mode puzzle', 'set difficulty easy'):
            g.cmd(line)
        s = None
        for _ in range(400):
            s = g.state()
            if s['screen'] == 'game' and s.get('phase') == 'playing' and not s.get('busy') and not s.get('paused'):
                break
            kind, rest = g.cmd('move')
            if kind == 'MOVE' and rest.strip() != 'null':
                for step in json.loads(rest).get('input', []):
                    drive.send_input(g, step)
                g.wait_updates(max(1, json.loads(rest).get('updates', 1)))
            else:
                g.wait_updates(30)
        check('on the board', s['screen'] == 'game' and s.get('phase') == 'playing', json.dumps({k: s.get(k) for k in ('screen', 'phase', 'mode')}))

        # 1. a drag onto the neighbour on the right: the two kinds trade places and the board is in the swap phase
        pair = find_pair(s['board'], 1, 0)
        (x1, y1), (x2, y2), k1, k2 = pair
        px1, py1 = cell(x1, y1)
        px2, py2 = cell(x2, y2)
        for line in ('move game %d %d' % (px1, py1), 'down game %d %d' % (px1, py1), 'move game %d %d' % (px2, py2), 'up game %d %d' % (px2, py2)):
            kind, rest = g.cmd(line)
            if kind == 'ERROR':
                raise RuntimeError(line + ' -> ' + rest)
        t = g.state()
        check('the drag swapped the kinds', t['board'][y1][x1] == k2 and t['board'][y2][x2] == k1,
              '%s%s -> %s%s' % (k1, k2, t['board'][y1][x1], t['board'][y2][x2]))
        check('the board is in the swap phase', t.get('phase') in ('swap', 'swapback', 'settle'), str(t.get('phase')))
        g.wait_updates(150)

        # 2. a drag onto a cell two away drops the pick and swaps nothing
        s = g.state()
        for _ in range(100):
            if s.get('phase') == 'playing' and not s.get('busy'):
                break
            g.wait_updates(20)
            s = g.state()
        pair = None
        for y in range(9):
            for x in range(7):
                if animal(s['board'][y][x]) and animal(s['board'][y][x + 1]) and animal(s['board'][y][x + 2]):
                    pair = ((x, y), (x + 2, y))
                    break
            if pair:
                break
        (x1, y1), (x2, y2) = pair
        before = s['board']
        px1, py1 = cell(x1, y1)
        px2, py2 = cell(x2, y2)
        for line in ('move game %d %d' % (px1, py1), 'down game %d %d' % (px1, py1), 'move game %d %d' % (px2, py2), 'up game %d %d' % (px2, py2)):
            g.cmd(line)
        g.wait_updates(5)
        t = g.state()
        check('a drag two cells away swaps nothing', t['board'] == before and t.get('phase') == 'playing', str(t.get('phase')))
    except Exception as e:
        check('no error', False, str(e))
    result['errors'] = g.errors[:5]
    g.close()
    print(json.dumps(result))
    return 0 if all(c['ok'] for c in result['checks']) and not g.errors else 1


if __name__ == '__main__':
    sys.exit(main())
