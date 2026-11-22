#!/usr/bin/env python3
"""respell.py <part> <profile.bwa> [port options]: a part's hand-picked words placed again on the boards the port deals
under the given options (by default the pilot's, -holdloadhints), for steps written against other boards.

A word is a run of `click x y  # L ...` lines whose comment starts with one capital letter (wait lines between them
are kept). For each word in order, the port plays the part's steps up to the word's first click (coverage/port.sh,
-fast), the last `board:` line of its board log gives each tile's letter and place, and the word's clicks are moved
onto tiles with its letters (a tile it already clicks first, then the first free one by rows); the first click's
comment gets the board's rows. A word whose letters the board does not hold stops the run. The part is rewritten in
place; its other lines are untouched.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..', '..'))
LETTER = re.compile(r'^click\s+(\d+)\s+(\d+)\s+#\s*([A-Z])(\s|$)')


def words(lines):
    """[(first index, [click line indexes])] for each run of letter clicks."""
    out, cur = [], None
    for i, line in enumerate(lines):
        if LETTER.match(line):
            if cur is None:
                cur = (i, [])
                out.append(cur)
            cur[1].append(i)
        elif line.split()[:1] not in (['wait'], []):
            cur = None
    return out


def board(prefix, profile, args, tag):
    path = os.path.join(ROOT, 'build', 'scratch', 'pilot', 'respell-%s.txt' % tag)
    open(path, 'w', encoding='utf-8', newline='\n').write(''.join(prefix) + 'wait 1000\n')
    env = dict(os.environ, PORT_ARGS=args)
    subprocess.run(['bash', 'games/bookwormadv/tools/coverage/port.sh', 'respell-' + tag, path, profile], cwd=ROOT,
                   env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    log = os.path.join(ROOT, 'build', 'coverage', 'bookwormadv', 'port', 'respell-' + tag, 'board.log')
    last = None
    for line in open(log, encoding='utf-8', errors='replace'):
        if line.startswith('board:'):
            last = line
    if last is None or ' letters ' not in last:
        sys.exit('respell: no board before the word (%s)' % log)
    tiles = []
    for t in last.split(' letters ', 1)[1].split()[0].split(';'):
        if '@' in t:
            ch, xy = t.split('@')
            x, y = xy.split(',')
            tiles.append((ch, int(x), int(y)))
    # "board: <n> <WORD|-> x,y ... attack ...": the longest word and its tiles, in order
    f = last.split()
    best = None
    if f[2] != '-':
        places = [tuple(int(v) for v in p.split(',')) for p in f[3:3 + len(f[2])]]
        best = (f[2], [(ch, x, y) for ch, (x, y) in zip(f[2], places)])
    return tiles, best


def rows(tiles):
    ys = sorted({y for _, _, y in tiles})
    return ' / '.join(''.join(ch for ch, _, ty in sorted(tiles, key=lambda t: t[1]) if ty == y) for y in ys)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    part, profile = sys.argv[1], sys.argv[2]
    args = sys.argv[3] if len(sys.argv) > 3 else '-holdloadhints -popcaplogo -drawevery -renderscale=1'
    lines = open(part, encoding='utf-8').readlines()
    n = 0
    while True:
        found = words(lines)
        if n >= len(found):
            break
        first, clicks = found[n]
        tiles, best = board(lines[:first], profile, args, '%s-%d' % (os.path.basename(part)[:-4], n))
        word = ''.join(LETTER.match(lines[i]).group(3) for i in clicks)
        free, picks = list(tiles), []
        for i in clicks:
            m = LETTER.match(lines[i])
            x, y, ch = int(m.group(1)), int(m.group(2)), m.group(3)
            pick = next((t for t in free if t[0] == ch and abs(t[1] - x) <= 2 and abs(t[2] - y) <= 2), None)
            pick = pick or next((t for t in free if t[0] == ch), None)
            if pick is None:
                break
            free.remove(pick)
            picks.append(pick)
        if len(picks) == len(clicks):
            for k, (i, pick) in enumerate(zip(clicks, picks)):
                note = '# %s' % pick[0] + ('  (by rows: %s)' % rows(tiles) if k == 0 else '')
                lines[i] = '%-24s %s\n' % ('click %d %d' % (pick[1], pick[2]), note)
            print('%s: %s on %s' % (os.path.basename(part), word, rows(tiles)))
        else:
            # the board lacks a letter: its own longest word in the word's place, a click and a wait for each tile
            if not best:
                sys.exit('respell: %s does not fit the board (%s), which spells no word' % (word, rows(tiles)))
            new = []
            for k, (ch, x, y) in enumerate(best[1]):
                note = '# %s' % ch + ('  (%s in place of %s, by rows: %s)' % (best[0], word, rows(tiles)) if k == 0 else '')
                new.append('%-24s %s\n' % ('click %d %d' % (x, y), note))
                new.append('wait 400\n')
            new[-1] = 'wait 800\n'
            end = clicks[-1] + 1
            while end < len(lines) and lines[end].split()[:1] == ['wait']:
                end += 1
            lines[first:end] = new
            print('%s: %s in place of %s, on %s: rename any shot that names %s' % (os.path.basename(part), best[0], word,
                                                                                   rows(tiles), word))
        n += 1
    open(part, 'w', encoding='utf-8', newline='\n').writelines(lines)
    return 0


if __name__ == '__main__':
    sys.exit(main())
