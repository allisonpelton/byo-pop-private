"""Compares the original's board watches (origrun's "watch N: a -> b at update U" lines) with the port's -trace lines for the
same inputs (origcmp/record.py, to_origrun.py, run_original.sh). The four watches are the score (1), the level (2), the board's
state (3) and the generator's position (4).

    python games/bejeweled2/tools/origcmp/compare.py <origrun.log> <port game.log> [--delta N] [--show N]

Both sides' changes of a watch are listed in order and compared by value; the update at which a change happened is compared
after the delta (original update = port update + delta). It prints the first difference of each watch and a count of how many
changes agree, and exits 1 on any difference. The values before the first board (the "-" of no board) and the original's
watches of other objects before the game loads are skipped: a watch's list starts at its first change on a board.
"""
import argparse
import collections
import re
import sys

LINE = re.compile(r'watch (\d): (\S+) -> (\S+) at update (\d+)')


def read(path):
    out = collections.defaultdict(list)
    with open(path, 'r', errors='replace') as f:
        for l in f:
            m = LINE.search(l)
            if m:
                out[int(m.group(1))].append((int(m.group(4)), m.group(2), m.group(3)))
    return out


def norm(v):
    """The original prints a value in decimal; the port too. A hex value is read as such."""
    if v in ('-', '?'):
        return v
    try:
        return str(int(v, 0))
    except ValueError:
        return v


def trim(seq, from_update):
    # the original's watches show other memory before its board exists (the app's pointer chain is not yet filled): a list
    # starts at the first change at or after the board's creation, which the port marks with the "-" -> value of the first change
    return [c for c in seq if c[0] >= from_update]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('original')
    ap.add_argument('port')
    ap.add_argument('--delta', type=int, default=20)
    ap.add_argument('--show', type=int, default=3)
    a = ap.parse_args()
    o, p = read(a.original), read(a.port)
    bad = 0
    for w in (1, 2, 3, 4):
        # skip changes into and out of "no board": compare from the first value that is a number on both sides
        po = [(u, norm(x), norm(y)) for u, x, y in o.get(w, []) if norm(y) not in ('-', '?')]
        pp = [(u + a.delta, norm(x), norm(y)) for u, x, y in p.get(w, []) if norm(y) not in ('-', '?')]
        n = min(len(po), len(pp))
        same = 0
        first = None
        offs = collections.Counter()
        for i in range(n):
            if po[i][2] == pp[i][2]:
                same += 1
                offs[po[i][0] - pp[i][0]] += 1
            elif first is None:
                first = i
        print('watch %d: original %d changes, port %d, %d agree in value; update offsets (original - port - delta) %s' % (
            w, len(po), len(pp), same, dict(offs.most_common(6))))
        if first is None and len(po) == len(pp):
            continue
        bad += 1
        i = first if first is not None else n
        lo = max(0, i - a.show)
        print('  first difference at change %d:' % i)
        for j in range(lo, min(max(len(po), len(pp)), i + a.show + 1)):
            so = 'original %s' % (po[j],) if j < len(po) else 'original -'
            sp = 'port %s' % (pp[j],) if j < len(pp) else 'port -'
            print('   %s%d: %-44s %s' % ('>' if j == i else ' ', j, so, sp))
    print('IDENTICAL' if not bad else 'DIFFERENT (%d watches)' % bad)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
