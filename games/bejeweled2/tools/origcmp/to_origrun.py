"""Turns origcmp/record.py's events.json into an origrun script, so that the original gets the same inputs the port got.

    python games/bejeweled2/tools/origcmp/to_origrun.py events.json script.txt [--delta N] [--tail N]

The port's input at update U is given to the original at update U + delta (0: record.py waits for the original's first update that
takes a click, its title having loaded at 319, before it clicks play). The cursor is the
script's own (origrun --cursor scripted): a `move` before a click puts the original's mouse where the port's was.
The script ends `tail` updates after the last event with a quit.
"""
import argparse
import json


def convert(events, delta, tail, until=None, shifts=None, after=None, dialog_shift=0, remap=None):
    lines = []
    last = 0
    shifts = shifts or {}
    for u, cmd, *marks in events['events']:
        if until is not None and u > until:
            break
        at = u + delta + shifts.get(u, 0) + (after[1] if after and u > after[0] else 0) + (dialog_shift if marks and cmd != 'key Escape' else 0)
        last = max(last, at)
        w = cmd.split()
        if w[0] in ('move', 'click'):
            # "move game 320 401": game pixels are the original's client pixels at 640x480
            x, y = (remap or {}).get((int(w[2]), int(w[3])), (int(w[2]), int(w[3])))
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
    end = last + tail if tail is not None else max(last, events['last'] + delta) + 5
    lines.append('at %d mark' % end)  # origrun cannot quit at an update: wait for it, then quit
    lines.append('quit')
    return lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('events')
    ap.add_argument('script')
    ap.add_argument('--delta', type=int, default=None, help='original update - port update (default 0: record.py puts the play click at 345)')
    ap.add_argument('--shift', action='append', default=[], metavar='U=D', help='give the input recorded at port update U to the original D updates later (the board-creating click in the 3D game: the original handles it at once and the port one update later)')
    ap.add_argument('--after', default=None, metavar='U=D', help='give every input recorded after port update U to the original D updates later (the 3D game: the board is created at the same update, and its later inputs reach it one update away)')
    ap.add_argument('--dialog-shift', type=int, default=0, help='give every mouse input that reaches a dialog (marked by record.py; the key Escape that closes a hint dialog is not delayed) to the original this many updates later: -1 in the 3D game, where it takes a click on a dialog one update later than the port (the board then resumes its ticks one update late)')
    ap.add_argument('--tail', type=int, default=None, help="updates the original runs on after the last input (default: until the update at which the port's recording ended, so the endings are compared too)")
    ap.add_argument('--remap', action='append', default=[], metavar='X,Y=X2,Y2', help="a move or click at X,Y goes to the original at X2,Y2: a dialog's button where the port's 640 by 480 layout is not the original's (the yes of \"Load Game?\": 257,222 in the port, 257,203 in the original)")
    ap.add_argument('--until', type=int, default=None, help='only the inputs up to this port update')
    a = ap.parse_args()
    ev = json.load(open(a.events))
    delta = a.delta if a.delta is not None else 0
    tail = a.tail if a.tail is not None else None
    lines = ['# from %s, delta %d' % (a.events, delta)] + convert(ev, delta, tail, a.until, dict((int(x.split('=')[0]), int(x.split('=')[1])) for x in a.shift), after=(int(a.after.split('=')[0]), int(a.after.split('=')[1])) if a.after else None, dialog_shift=a.dialog_shift,
        remap=dict((tuple(int(v) for v in x.split('=')[0].split(',')), tuple(int(v) for v in x.split('=')[1].split(','))) for x in a.remap))
    with open(a.script, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print('%d lines, delta %d, last update %s' % (len(lines), delta, lines[-2].split()[1]))


if __name__ == '__main__':
    main()
