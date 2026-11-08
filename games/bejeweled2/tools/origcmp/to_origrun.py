"""Turns origcmp/record.py's events.json into an origrun script, so that the original gets the same inputs the port got.

    python games/bejeweled2/tools/origcmp/to_origrun.py events.json script.txt [--delta N] [--tail N]

The port's input at update U is given to the original at update U + delta, where delta puts the title's play click at the
original's first update that takes a click (its title loads at update 319; the port's was recorded at 325). The cursor is the
script's own (origrun --cursor scripted): a `move` before a click puts the original's mouse where the port's was.
The script ends `tail` updates after the last event with a quit.
"""
import argparse
import json


def convert(events, delta, tail):
    lines = []
    last = 0
    for u, cmd in events['events']:
        at = u + delta
        last = max(last, at)
        w = cmd.split()
        if w[0] in ('move', 'click'):
            # "move game 320 401": game pixels are the original's client pixels at 640x480
            x, y = int(w[2]), int(w[3])
            right = len(w) > 4 and w[4] == 'right'
            if w[0] == 'move':
                lines.append('at %d move %d %d' % (at, x, y))
            else:
                lines.append('at %d %s %d %d' % (at, 'rclick' if right else 'click', x, y))
        elif w[0] == 'type':
            lines.append('at %d text %s' % (at, ' '.join(w[1:])))
        elif w[0] == 'key':
            vk = {'Return': 13, 'Escape': 27, 'space': 32, 'Space': 32, 'Tab': 9}.get(w[1])
            if vk is None:
                raise SystemExit('no virtual key for ' + cmd)
            lines.append('at %d key %d' % (at, vk))
        elif w[0] in ('down', 'up'):
            lines.append('at %d %s %d %d' % (at, 'down' if w[0] == 'down' else 'up', int(w[2]), int(w[3])))
        else:
            raise SystemExit('cannot give the original ' + cmd)
    lines.append('at %d mark' % (last + tail))  # origrun cannot quit at an update: wait for it, then quit
    lines.append('quit')
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('events')
    ap.add_argument('script')
    ap.add_argument('--delta', type=int, default=None, help='original update - port update (default: the play click at 345)')
    ap.add_argument('--tail', type=int, default=60)
    a = ap.parse_args()
    ev = json.load(open(a.events))
    delta = a.delta if a.delta is not None else 345 - ev['play']
    lines = ['# from %s, delta %d' % (a.events, delta)] + convert(ev, delta, a.tail)
    with open(a.script, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print('%d lines, delta %d, last update %d' % (len(lines), delta, ev['last'] + delta))


if __name__ == '__main__':
    main()
