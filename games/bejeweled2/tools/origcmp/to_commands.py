"""Turns origcmp/record.py's events.json into a -commands file, so that the port (native, or the web build under Node by
wasmrun.js) plays a recording again without a driver:

    python games/bejeweled2/tools/origcmp/to_commands.py events.json commands.txt [--delta 1] [--nosparkle]
    bejeweled2.exe -gamedir= -userdir=<a new folder with registry.ini> -commands=<file> -realinput=<an empty file> -hold -fast -seed=1 -trace

A file is read whole: the lines up to a `wait N` run in one update and the lines after it in the update at which the N are up,
so the file has a `wait` wherever the recording has a gap. The framework runs a line given by the driver one update before it
runs a line of a whole file, so `--delta 1` (every line one update earlier) makes the replay's -trace lines the recording's,
every one (the game is held, as in the recording, and the global generator's draws agree).
"""
import argparse
import json


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('events')
    ap.add_argument('out')
    ap.add_argument('--until', type=int, default=None, help='only the inputs up to this update')
    ap.add_argument('--delta', type=int, default=1, help='run the lines this many updates earlier (calibration)')
    ap.add_argument('--nosparkle', action='store_true')
    ap.add_argument('--first', type=int, default=1, help='the update at which the first lines of the file run')
    ap.add_argument('--shots', default=None, help="a folder (relative to the game's working directory): each `shot <name>` of the recording becomes `screenshot <folder>/<name>.png`, run at the update after the recording's one, as the driver's state reply and then its screenshot command are")
    a = ap.parse_args()
    ev = json.load(open(a.events))
    lines = []
    if a.nosparkle:
        lines.append('nosparkle')
    # a file is read whole: the lines up to a `wait N` run in one update, and the lines after it in the update at which the N are up
    cur = a.first  # the update at which the first lines run
    for u, cmd, *_ in ev['events']:
        if a.until is not None and u > a.until:
            break
        shot = cmd.startswith('shot ')
        if shot and a.shots is None:
            continue
        want = u - a.delta + (1 if shot else 0)
        if want > cur:
            lines.append('wait %d' % (want - cur))
            cur = want
        lines.append('screenshot %s/%s.png' % (a.shots, cmd[5:]) if shot else cmd)
    lines.append('quit')
    with open(a.out, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    print('%d lines' % len(lines))


if __name__ == '__main__':
    main()
