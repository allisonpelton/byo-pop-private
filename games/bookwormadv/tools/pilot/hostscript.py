#!/usr/bin/env python3
"""hostscript.py <orig script> <host script>: gen.py's script for the original as a host run through origrun's in-game
frame reader wants it (docs/ORIGRUN.md, "The in-game frame reader"), whose only shots are `at <N> shot` lines: the
window wait goes (the virtual clock's `at` lines wait for the game), and each `at <N> burst <name> <count> update <k>`
becomes <count> stills `at <N + i*k> shot <name>_<i>`, which run.sh gathers into the clip's folder as it does the
sandbox's burst frames (`at +N` bursts the same, counted from the last mark)."""
import sys


def main():
    src, dst = sys.argv[1], sys.argv[2]
    out = []
    for line in open(src, encoding='utf-8'):
        w = line.split()
        if w[:1] == ['waitshot']:
            continue
        if len(w) == 7 and w[0] == 'at' and w[2] == 'burst' and w[5] == 'update':
            plus = '+' if w[1].startswith('+') else ''
            at, name, count, step = int(w[1].lstrip('+')), w[3], int(w[4]), int(w[6])
            out += ['at %s%d shot %s_%d' % (plus, at + i * step, name, i) for i in range(count)]
            continue
        out.append(line.rstrip('\n'))
    # the `at` lines in order of their update (a burst's frames may pass later steps' updates), within each stretch
    # between the lines that are not `at` lines (gen.py's waitfor and mark, which `at +N` counts from, and quit)
    fixed, stretch = [], []
    for line in out:
        w = line.split()
        if w[:1] == ['at']:
            stretch.append((int(w[1].lstrip('+')), len(stretch), line))
            continue
        fixed += [l for _, _, l in sorted(stretch)]
        stretch = []
        fixed.append(line)
    fixed += [l for _, _, l in sorted(stretch)]
    open(dst, 'w', encoding='utf-8', newline='\n').write('\n'.join(fixed) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
