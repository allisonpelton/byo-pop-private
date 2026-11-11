"""Turns a phase-one recording (origcmp/record.py's events.json: the inputs of one game the port's bot played, each with the update
it ran at) into a tour file for tour.sh, with a shot every so many updates:

    python games/astropop/tools/tour/gametour.py <events.json> <tour.txt> [--every N] [--first U] [--last U] [--at U,U,...]

The same inputs go to the original at the same updates (phase one played both through games that came out identical), so the
shots of the two sides are of the same moments of the same game. A shot is named g<update>.
"""
import argparse
import json


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('events')
    ap.add_argument('tour')
    ap.add_argument('--every', type=int, default=150, help='a shot every N updates from --first to --last')
    ap.add_argument('--first', type=int, default=None, help="default: the first input's update")
    ap.add_argument('--last', type=int, default=None, help="default: the last input's update")
    ap.add_argument('--at', default='', help='more shots at these updates')
    ap.add_argument('--charge', type=int, default=None, help="the port's weapon charge for every new board and ResetState (the port-only test command `set chargestart`): the original's patch list stores it in the same two places")
    a = ap.parse_args()
    ev = json.load(open(a.events))
    events = ev['events']
    lines = []
    i = 0
    while i < len(events):
        u, cmd = events[i]
        i += 1
        w = cmd.split()
        if w[0] == 'move':
            lines.append((u, 'move %s %s' % (w[2], w[3])))
        elif w[0] == 'click':
            lines.append((u, '%s %s %s' % ('rclick' if len(w) > 4 and w[4] == 'right' else 'click', w[2], w[3])))
        elif w[0] == 'type':
            chars = w[1]
            while i < len(events) and events[i][0] == u and events[i][1].split()[0] == 'type':
                chars += events[i][1].split()[1]
                i += 1
            lines.append((u, 'type ' + chars))
        elif w[0] == 'key':
            lines.append((u, 'key ' + w[1]))
        elif w[0] in ('down', 'up'):
            lines.append((u, '%s %s %s' % (w[0], w[2], w[3])))
        else:
            raise SystemExit('cannot give a tour ' + cmd)
    if a.charge is not None:
        # the original's patch list stores it in the constructor of the board and in ResetState (every new board, game and level):
        # the port's test command `set chargestart` does the same from the first update on
        lines.append((5, 'portcmd set chargestart %d' % a.charge))
    first = a.first if a.first is not None else events[0][0]
    last = a.last if a.last is not None else min(ev['last'], events[-1][0] + 60)
    shots = set(range(first, last + 1, a.every))
    for x in a.at.split(','):
        if x.strip():
            shots.add(int(x))
    # a shot at the update of an input shows something else on the two sides (the port's inputs are stamped one update earlier, so
    # its shot already sees what the input did): such a shot moves back to the nearest update with no input
    busy = set(u for u, _ in lines)
    moved = set()
    for u in sorted(shots):
        v = u
        while v in busy or v in moved:
            v -= 1
        moved.add(v)
    shots = moved
    # at one update a shot comes before the inputs, as in the original's script (the port's inputs are stamped one update earlier,
    # which tour.py does: a shot at an input's update shows the state the input finds)
    out = [(u, 1, c) for u, c in lines] + [(u, 0, 'shot g%d' % u) for u in sorted(shots)]
    out.sort(key=lambda t: (t[0], t[1]))
    with open(a.tour, 'w') as f:
        f.write('# from %s: %d inputs, %d shots\n' % (a.events, len(lines), len(shots)))
        for u, _, c in out:
            f.write('%d %s\n' % (u, c))


if __name__ == '__main__':
    main()
