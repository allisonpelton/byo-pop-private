#!/usr/bin/env python3
"""anchors.py marks <origrun or visrun log> > <marks> | anchors.py fill <port script> <marks> <out> |
anchors.py ends <port board log> <k> <marks> <port script>: the port's side of gen.py's `loaded` steps.

marks: the update counter at each of the original's marks, comma-separated: origrun's "mark: the counter reads V" lines,
or for visrun, which does not log its marks, the update at which each load's last waitfor (the curtain open) held,
where visrun's mark reads the counter of the game it then holds.

fill: each port line "@<k>+<N> <command>" becomes "<V_k + N> <command>", V_k the k-th mark (N already holds gen.py's
one update of the port's lead). run.sh fills the port's script with the port's own load ends (ends, below), so each
side counts from the end of its own load. A script with more marks than the file fails.

ends: the port's own load ends, from its -boardlog's "opened: <n> <update>" lines, each the update the curtain over a
load has opened (the update after the one the original's mark reads), or for a @<k>l mark its "loadend:" line, put in place of the first k marks of <marks>: run.sh's passes, each a -fast port run whose k-th
load starts where the real run's will, so that each side counts from the end of its own load (the two start a load a
few updates apart after some screens, the arena's treasure room)."""
import re
import sys

TRANSITION = re.escape('[0x8259e4]+0x8ec')


def marks(log):
    got, vis = [], []
    for line in open(log, errors='replace'):
        m = re.search(r'mark: the counter reads (\d+)', line)
        if m:
            got.append(int(m.group(1)))
        m = re.search(r"'%s == 0' true at update (\d+)" % TRANSITION, line)
        if m:
            vis.append(int(m.group(1)))
    return got or vis


def fill(script, mk, out):
    vals = [int(v) for v in open(mk).read().strip().split(',') if v]
    lines = []
    for line in open(script, encoding='utf-8'):
        m = re.match(r'@(\d+)[gl]?\+(\d+) (.*)', line.rstrip('\n'))
        if m:
            k = int(m.group(1))
            if k > len(vals):
                sys.exit('anchors: the script needs mark %d, the original logged %d (%s)' % (k, len(vals), mk))
            line = '%d %s\n' % (vals[k - 1] + int(m.group(2)), m.group(3))
        lines.append(line)
    open(out, 'w', encoding='utf-8', newline='\n').write(''.join(lines))


def ends(log, k, mk, script):
    # a `loaded <ms> gone` mark (@<k>g) keeps the original's: its load may have been over when the original reached
    # the line, which then marked the update it saw it gone at, not the load's end
    # a `loaded <ms> load` mark (@<k>l) takes the k-th load's end ("loadend:") instead of its curtain's opening
    text = open(script, encoding='utf-8').read()
    keep = {int(m.group(1)) for m in re.finditer(r'^@(\d+)g\+', text, re.M)}
    atend = {int(m.group(1)) for m in re.finditer(r'^@(\d+)l\+', text, re.M)}
    board = open(log, errors='replace').read()
    opened, loadend = ([int(m.group(1)) - 1 for m in re.finditer(r'^%s: \d+ (\d+)$' % w, board, re.M)]
                       for w in ('opened', 'loadend'))
    got = [(loadend if i + 1 in atend else opened)[i] for i in range(min(len(opened), len(loadend)))]
    old = [v for v in open(mk).read().strip().split(',') if v]
    if len(got) < k:
        sys.exit('anchors: the port logged %d load ends, the pass wants %d (%s)' % (len(got), k, log))
    new = [old[i] if i + 1 in keep else str(v) for i, v in enumerate(got[:k])] + old[k:]
    open(mk, 'w').write(','.join(new) + '\n')


def main():
    if len(sys.argv) == 3 and sys.argv[1] == 'marks':
        print(','.join(str(v) for v in marks(sys.argv[2])))
    elif len(sys.argv) == 6 and sys.argv[1] == 'ends':
        ends(sys.argv[2], int(sys.argv[3]), sys.argv[4], sys.argv[5])
    elif len(sys.argv) == 5 and sys.argv[1] == 'fill':
        fill(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
