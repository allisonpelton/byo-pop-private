"""addshots.py <steps in> <steps out> <prefix> [every]: a shot before each step of a list made by the port's bot
(tools/coverage/play.py) that answers something on the screen, so that a long part of the screenshot pilot pictures
what it plays through without hand-timed shots.

A shot named <prefix>_<n>_<kind> goes before each click whose comment names a dialog, a tutorial's tile, an arrow's
hint, a lost turn, a treasure, the treasure room's Continue, the map's Enter! or the conversation moving on, and before
each idle click (the click at 400,150 play.py makes when nothing asks: Level Up's "Click to continue", a word scroll).
Every <every>th Attack (default 4) gets one too, 2 s after it (the attack under way). The shots already in the list stay.
"""
import sys

KINDS = (('# dialog', 'dialog'), ('# tutorial', 'tutorial'), ('# hint', 'hint'), ('# lostturn', 'lostturn'),
         ('# a treasure', 'treasure'), ('# Continue', 'continue'), ('# Enter!', 'enter'),
         ('# the conversation on', 'talk'))


def main():
    src, dst, prefix = sys.argv[1:4]
    every = int(sys.argv[4]) if len(sys.argv) > 4 else 4
    out, n, attacks, pending = [], 0, 0, False
    for line in open(src):
        s = line.rstrip('\n')
        w = s.split('#', 1)[0].split()
        kind = next((k for c, k in KINDS if c in s), None)
        if w[:1] == ['click'] and kind is None and w[1:3] == ['400', '150']:
            kind = 'idle'
        if kind:
            n += 1
            out.append('shot %s_%02d_%s' % (prefix, n, kind))
        out.append(s)
        if w[:1] == ['click'] and '# Attack' in s:
            attacks += 1
            pending = attacks % every == 0
        elif pending and w[:1] == ['wait']:
            # 2 s into the wait after the Attack (play.py waits 2500 there)
            ms = int(w[1])
            if ms > 2000:
                n += 1
                out[-1] = 'wait 2000'
                out += ['shot %s_%02d_attack' % (prefix, n), 'wait %d' % (ms - 2000)]
            pending = False
    open(dst, 'w', newline='\n').write('\n'.join(out) + '\n')
    print('%s: %d shots added' % (dst, n))


main()
