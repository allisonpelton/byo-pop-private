#!/usr/bin/env python3
"""The scenario of a death on a skull trap, for p2.sh: a saved classic game whose maze has a trap is continued, the explorer
walks onto the trap, and both programs are photographed every few updates while the block falls (or the pit opens).

  trapscn.py find <state line file> <save> <out scenario> [style] [step]
      <state line file> holds the port's STATE line of the saved board (the `state` test command: the explorer, the traps and
      the walls). Every walk of up to 6 steps from the explorer to a trap is tried in the port (the save continued, the arrow
      keys, -fast under rundesk, a few seeds), and the first one whose death is of the wanted style (0 the block, 1 the pit) is
      written as a p2 scenario with a shot every <step> updates (default 2) from just before the trap to the lost panel.
      Prints the seed to give p2.sh.

Run it from the worktree root, under rundesk: build/native/rundesk.exe python -I games/mummymaze/tools/identical/trapscn.py find ...
The moves are found by this script and checked by the port; nobody picks them.
"""
import json
import os
import re
import shutil
import subprocess
import sys

KEYS = {(-1, 0): 37, (0, -1): 38, (1, 0): 39, (0, 1): 40}
# the play link, the classic link, Continue's Yes, and the Ready button of the traps' tip (a fresh user folder shows it)
CONTINUE = ['99 title', '1000 click 185 376', '1006 click 258 291', '1100 click 395 317']
FIRST_KEY = 1400      # the continued board takes input by then
STEP = 70             # updates between two steps (a step takes 60 at the default speed)


def state_of(path):
    last = None
    for ln in open(path, errors='replace'):
        m = re.search(r'STATE (\{.*\})', ln)
        if m:
            j = json.loads(m.group(1))
            if j.get('board'):
                last = j
    return last


def walks(board, limit=6):
    """Every walk without a repeated cell from the explorer to a trap, shortest first."""
    n = board['size']
    w = board['walls']
    traps = [tuple(t) for t in board['traps'] if t[0] != 99]

    def blocked(x, y, dx, dy):
        c = int(w[y * n + x], 16)
        x2, y2 = x + dx, y + dy
        if not (0 <= x2 < n and 0 <= y2 < n):
            return True
        c2 = int(w[y2 * n + x2], 16)
        bit, back = {(0, -1): (1, 2), (0, 1): (2, 1), (1, 0): (4, 8), (-1, 0): (8, 4)}[(dx, dy)]
        return bool(c & bit) or bool(c2 & back)

    out = []

    def go(x, y, seen, path):
        if (x, y) in traps and path:
            out.append(list(path))
            return
        if len(path) >= limit:
            return
        for d in KEYS:
            if blocked(x, y, d[0], d[1]):
                continue
            p = (x + d[0], y + d[1])
            if p in seen:
                continue
            go(p[0], p[1], seen | {p}, path + [d])

    e = tuple(board['explorer'])
    go(e[0], e[1], {e}, [])
    return sorted(out, key=len)


def try_walk(root, save, walk, seed, tag):
    d = os.path.join(root, 'build', 'trap', 'try')
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    shutil.copy(save, os.path.join(d, 'classic.sav'))
    lines = ['99 click 323 470', '1000 click 185 376', '1006 click 258 291', '1100 click 395 317']
    u = FIRST_KEY
    for dxy in walk:
        lines += ['%d keydown %d' % (u, KEYS[dxy]), '%d keyup %d' % (u + 1, KEYS[dxy])]
        u += STEP
    for k in range(0, 80, 2):       # where the death starts: the first update with a death timer
        lines.append('%d state' % (u - STEP + k))
    lines.append('%d quit' % (u + 100))
    script = os.path.join(root, 'build', 'trap', 'try.txt')
    open(script, 'w', newline='\n').write('\n'.join(lines) + '\n')
    r = subprocess.run([os.path.join(root, 'build', 'native', 'mummymaze.exe'), '-userdir=' + d, '-fast', '-seed=%d' % seed,
                        '-script=' + script], capture_output=True, text=True, errors='replace', timeout=120)
    first = None
    for ln in (r.stdout + r.stderr).splitlines():
        m = re.search(r'STATE (\{.*\})', ln)
        if not m:
            continue
        j = json.loads(m.group(1))
        b = j.get('board')
        if b and b['death'][0] >= 0 and first is None:
            first = (j['update'], b['death'][0], b['death'][1])
    return first


def find(statefile, save, out, style=0, step=2):
    root = os.getcwd()
    j = state_of(statefile)
    ws = walks(j['board'])
    print('%d walks to a trap' % len(ws))
    for walk in ws:
        for seed in (21, 1, 2, 3, 4, 5, 6, 7):
            got = try_walk(root, save, walk, seed, 'w')
            if got is None:
                break               # caught on the way, or the walk does not arrive: the next walk
            u, timer, st = got
            print('walk %s seed %d: death timer %d at update %d, style %d' % (walk, seed, timer, u, st))
            if st != style:
                continue
            start = u - (timer - (0x1e if st == 0 else 0)) - 10
            lines = list(CONTINUE)
            k = FIRST_KEY
            for dxy in walk:
                lines.append('%d key %d' % (k, KEYS[dxy]))
                k += STEP
            n = 0
            for s in range(start, start + 300, step):
                lines.append('%d shot f%03d' % (s, n))
                n += 1
            lines.append('%d quit' % (start + 320))
            lines.sort(key=lambda l: int(l.split()[0]))
            open(out, 'w', newline='\n').write('\n'.join(lines) + '\n')
            print('wrote %s: %d shots from update %d, seed %d' % (out, n, start, seed))
            return 0
    print('no walk found')
    return 1


if __name__ == '__main__':
    if len(sys.argv) >= 5 and sys.argv[1] == 'find':
        sys.exit(find(sys.argv[2], sys.argv[3], sys.argv[4], int(sys.argv[5]) if len(sys.argv) > 5 else 0,
                      int(sys.argv[6]) if len(sys.argv) > 6 else 2))
    sys.exit(__doc__)
