"""Turns origcmp/record.py's events.json into an origrun script, so that the original gets the same inputs the port got.

    python games/astropop/tools/origcmp/to_origrun.py events.json script.txt [--delta N] [--tail N] [--until N]

The port's input at update U is given to the original at update U + delta (0 by default: record.py puts the title click where the
original takes one). The mouse is the script's own (origrun --cursor scripted): a `move` before a click puts the original's
mouse where the port's was. The script ends `tail` updates after the last event with a quit.
"""
import argparse
import json


def convert(events, delta, tail, until=None):
    lines = []
    last = 0
    i = 0
    while i < len(events):
        u, cmd = events[i]
        i += 1
        if until is not None and u > until:
            break
        at = u + delta
        last = max(last, at)
        w = cmd.split()
        if w[0] == 'move':
            lines.append('at %d move %d %d' % (at, int(w[2]), int(w[3])))
        elif w[0] == 'click':
            lines.append('at %d %s %d %d' % (at, 'rclick' if len(w) > 4 and w[4] == 'right' else 'click', int(w[2]), int(w[3])))
        elif w[0] == 'type':
            chars = w[1]
            while i < len(events) and events[i][0] == u and events[i][1].split()[0] == 'type':
                chars += events[i][1].split()[1]
                i += 1
            lines.append('at %d text %s' % (at, chars))
        elif w[0] == 'key':
            vk = {'Return': 13, 'Escape': 27, 'Space': 32, 'Tab': 9}.get(w[1])
            if vk is None:
                raise SystemExit('no virtual key for ' + cmd)
            lines.append('at %d key %d' % (at, vk))
        elif w[0] in ('down', 'up'):
            lines.append('at %d %s %d %d' % (at, w[0], int(w[2]), int(w[3])))
        else:
            raise SystemExit('cannot give the original ' + cmd)
    lines.append('at %d mark' % (last + tail))  # origrun cannot quit at an update: wait for it, then quit
    lines.append('quit')
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('events')
    ap.add_argument('script')
    ap.add_argument('--delta', type=int, default=0)
    ap.add_argument('--tail', type=int, default=60)
    ap.add_argument('--until', type=int, default=None, help='only the inputs up to this port update')
    a = ap.parse_args()
    ev = json.load(open(a.events))
    lines = ['# from %s, delta %d' % (a.events, a.delta)] + convert(ev['events'], a.delta, a.tail, a.until)
    with open(a.script, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print('%d lines, delta %d, last update %d' % (len(lines), a.delta, ev['last'] + a.delta))


if __name__ == '__main__':
    main()
