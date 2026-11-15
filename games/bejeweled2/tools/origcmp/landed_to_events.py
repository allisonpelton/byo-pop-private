"""Turns a visrun.py run (the original on the user's desktop, on the virtual clock) into a recording for the port: each input and
shot of origcmp/record.py's events.json goes to the update at which the original really took it (landed.tsv), so that the port
plays the same inputs at the same updates (to_commands.py --shots, then the port natively) and its shots can be compared with
the original's by tools/shotdiff.

    python games/bejeweled2/tools/origcmp/landed_to_events.py events.json landed.tsv out-events.json [--shot-delta N] [--shift FROM TO DELTA]... [--leave-after N]

With --leave-after N the port's mouse leaves the window N updates after every click: the original's mouse position is the real
cursor's, which the visible-desktop runs never move into the game's window, so the original's board gets the posted move and click
and the leave at once, and no hover (the glow of the gem under the mouse, the highlight of a button) lasts, and the manager's last
mouse position is the cursor's, outside the window (the board's own MouseMove call from its idle state reads it); the port's
replayed mouse stays where the last move put it. The port's mouse goes to a point outside the window and leaves it.

A shot is taken at the update it landed at, an input at the update its hold found (the counter when it was posted; the original
reads it in the next update, as the port reads a command given at that update). The input that reaches a dialog needs no shift
of its own here: to_origrun.py's --dialog-shift -1, which the sandbox runs of the original needed, gives the 3D original that
input one update earlier, and the landed update is that one (the whirlpool after the "Load Game?" dialog's Yes ran one update
late in the port when the landed update was moved on by one). The gate of visrun.py moves the commands after it on when the
title's loading took longer than the recording's, so the landed updates are the original's, not the recording's.
"""
import json
import sys


def main():
    a = sys.argv[1:]
    shot_delta = 0
    if '--shot-delta' in a:
        i = a.index('--shot-delta')
        shot_delta = int(a[i + 1])
        del a[i:i + 2]
    shifts = []   # --shift FROM TO DELTA: the inputs landed at updates FROM..TO go DELTA updates later (earlier when negative)
    while '--shift' in a:
        i = a.index('--shift')
        shifts.append((int(a[i + 1]), int(a[i + 2]), int(a[i + 3])))
        del a[i:i + 4]
    leave_after = None   # --leave-after N: the mouse leaves the window N updates after each click (see below)
    if '--leave-after' in a:
        i = a.index('--leave-after')
        leave_after = int(a[i + 1])
        del a[i:i + 2]
    src, landed, out = a
    ev = json.load(open(src))
    rows = [l.rstrip('\n').split('\t') for l in open(landed) if l.strip()]
    if len(rows) != len(ev['events']):
        raise SystemExit('%d landed lines for %d events' % (len(rows), len(ev['events'])))
    new = []
    for (u, cmd, *marks), row in zip(ev['events'], rows):
        shot = cmd.startswith('shot ')
        if shot != (row[0] == 'shot') or (shot and row[1] != cmd[5:]):
            raise SystemExit('event %s does not match landed line %s' % (cmd, row))
        c = int(row[2])
        if shot:
            c += shot_delta
        else:
            c += sum(d for lo, hi, d in shifts if lo <= c <= hi)
        new.append([c, cmd] + marks)
        if leave_after is not None and cmd.startswith('click '):
            new.append([c + leave_after, 'move game -9 -9'] + marks)
            new.append([c + leave_after, 'leave'] + marks)
    # the same update twice is fine in a commands file (the lines up to a wait run in one update)
    new.sort(key=lambda e: e[0])
    ev['events'] = new
    ev['last'] = new[-1][0] + 2
    json.dump(ev, open(out, 'w'))
    print('%d events, last update %d' % (len(new), ev['last']))


if __name__ == '__main__':
    main()
