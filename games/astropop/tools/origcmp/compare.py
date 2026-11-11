"""Compares the original's watches (origrun's "watch N: a -> b at update U" lines) with the port's -trace lines for the
same inputs (origcmp/record.py, to_origrun.py, run_original.sh). The four watches are the board's score (1), its level (2),
the game generator's position (3) and Rand()'s position (4).

    python games/astropop/tools/origcmp/compare.py <origrun.log> <port game.log> [--delta N] [--last U] [--show N]

Both sides' changes of a watch are listed in order and compared by value and by the update at which each happened (after the
delta: original update = port update + delta). It prints the first difference of each watch and a count of how many
changes agree, and exits 1 on any difference. A log in which origrun cut a watch at its cap ("N changes logged; no more") is
not compared at all (exit 2): the rest of that game was never recorded. The first reading of a watch ("?" -> value) and the values of no board (the "-")
are skipped. --last ignores the changes after that update (the end of the port's recording: the original's script runs on a
little longer).
"""
import argparse
import collections
import re
import sys

LINE = re.compile(r'watch (\d): (\S+) -> (\S+) at update (\d+)')
CAPPED = re.compile(r'watch (\d): (\d+) changes logged; no more')
UNREACHED = re.compile(r'counter (\d+) not reached within \d+ ?ms on the virtual clock \(last (\d+)\)')


def read(path):
    out = collections.defaultdict(list)
    capped = {}
    with open(path, 'r', errors='replace') as f:
        for l in f:
            m = LINE.search(l)
            if m:
                out[int(m.group(1))].append((int(m.group(4)), m.group(2), m.group(3)))
                continue
            m = CAPPED.search(l)
            if m:
                capped[int(m.group(1))] = int(m.group(2))
    return out, capped


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


def settle(seq):
    """The original's watch is read inside its update as well as between updates, so it can see a value that the update changes again
    (a state that goes 0, 1, 16 within one update); the port's trace sees only the end of an update. Keep the last value of each update,
    and leave out a change that ends where it began."""
    out = []
    for u, x, y in seq:
        if out and out[-1][0] == u:
            out[-1] = (u, out[-1][1], y)
        else:
            out.append((u, x, y))
    return [c for c in out if c[1] != c[2]]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('original')
    ap.add_argument('port')
    ap.add_argument('--delta', type=int, default=0)
    ap.add_argument('--show', type=int, default=3)
    ap.add_argument('--last', type=int, default=None, help='ignore the changes after this update (the end of the port recording: the original script runs on a little longer)')
    ap.add_argument('--pair', default='', help="w=p: the original's watch w is the port's watch p (2=5: the global generator's index, W2=0x67d910)")
    a = ap.parse_args()
    (o, capped), (p, _) = read(a.original), read(a.port)
    if capped:
        # origrun stops logging a watch at its cap (--watch-max): the rest of the game was not recorded, which is no
        # difference and no agreement (it was read as "the generator stops" once: classic, seed 13)
        for w in sorted(capped):
            print("the original's watch %d was cut at origrun's cap of %d changes: nothing to compare after it" % (w, capped[w]))
        print('NO COMPARISON (raise --watch-max)')
        return 2
    # the script ends with a mark at the port's last update; when the port's bot stood on a screen for a long time, the
    # original does not get there within origrun's 30 s for a step and is stopped at the update it had reached: the
    # two are compared up to that update, and the verdict says so
    short = None
    with open(a.original, 'r', errors='replace') as f:
        for l in f:
            m = UNREACHED.search(l)
            if m and a.last is not None and int(m.group(1)) == a.last and int(m.group(2)) < a.last:
                short = int(m.group(2))
    if short is not None:
        print("the original was stopped at update %d of %d (the script's end mark was out of reach): compared up to there" % (short, a.last))
        full, a.last = a.last, short
    pair = {1: 1, 2: 2, 3: 3, 4: 4}
    if a.pair:
        pair[int(a.pair.split('=')[0])] = int(a.pair.split('=')[1])
    bad = 0
    for w in (1, 2, 3, 4):
        # skip changes into and out of "no board": compare from the first value that is a number on both sides
        po = [(u, norm(x), norm(y)) for u, x, y in o.get(w, []) if 1 <= u <= (a.last or 1 << 30) and norm(y) not in ('-', '?') and norm(x) != '?']
        pp = [(u + a.delta, norm(x), norm(y)) for u, x, y in p.get(pair[w], []) if 1 <= u <= (a.last or 1 << 30) - a.delta and norm(y) not in ('-', '?') and norm(x) != '?']
        po, pp = settle(po), settle(pp)
        n = min(len(po), len(pp))
        same = 0
        first = None
        offs = collections.Counter()
        for i in range(n):
            if po[i][2] == pp[i][2] and (i > 0 or po[i][1] in ('-', pp[i][1]) or pp[i][1] == '-'):
                same += 1
                offs[po[i][0] - pp[i][0]] += 1
            elif first is None:
                first = i
        print('watch %d: original %d changes, port %d, %d agree in value; update offsets (original - port - delta) %s' % (
            w, len(po), len(pp), same, dict(offs.most_common(6))))
        if first is None and len(po) == len(pp):
            # the same values: every change must also come at the same update
            late = [i for i in range(n) if po[i][0] != pp[i][0]]
            if not late:
                continue
            first = late[0]
            print('  the values agree, but %d changes come at another update' % len(late))
        bad += 1
        i = first if first is not None else n
        lo = max(0, i - a.show)
        print('  first difference at change %d:' % i)
        for j in range(lo, min(max(len(po), len(pp)), i + a.show + 1)):
            so = 'original %s' % (po[j],) if j < len(po) else 'original -'
            sp = 'port %s' % (pp[j],) if j < len(pp) else 'port -'
            print('   %s%d: %-44s %s' % ('>' if j == i else ' ', j, so, sp))
    if bad:
        print('DIFFERENT (%d watches)' % bad)
    elif short is not None:
        print('IDENTICAL to update %d of %d' % (short, full))
    else:
        print('IDENTICAL')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
