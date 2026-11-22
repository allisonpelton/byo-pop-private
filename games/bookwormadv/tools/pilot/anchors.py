#!/usr/bin/env python3
"""anchors.py marks <origrun or visrun log> > <marks> | anchors.py fill <port script> <marks> <out>: the port's side of
gen.py's `loaded` steps.

marks: the update counter at each of the original's marks, comma-separated: origrun's "mark: the counter reads V" lines,
or for visrun, which does not log its marks, the update at which each load's second waitfor (the screen gone) held,
where visrun's mark reads the counter of the game it then holds.

fill: each port line "@<k>+<N> <command>" becomes "<V_k + N> <command>", the k-th mark's update plus N. The port's
loads last as long as the original's (run.sh gives it -chapterloadupdates from the same run), so it reaches the end of
each load at the original's update and the lines after it line up with the original's `at +N`. A script with more
marks than the log fails."""
import re
import sys

LOADSCREEN = re.escape('[0x8259e4]+0x83c')


def marks(log):
    got, vis = [], []
    for line in open(log, errors='replace'):
        m = re.search(r'mark: the counter reads (\d+)', line)
        if m:
            got.append(int(m.group(1)))
        m = re.search(r"'%s == 0' true at update (\d+)" % LOADSCREEN, line)
        if m:
            vis.append(int(m.group(1)))
    return got or vis


def fill(script, mk, out):
    vals = [int(v) for v in open(mk).read().strip().split(',') if v]
    lines = []
    for line in open(script, encoding='utf-8'):
        m = re.match(r'@(\d+)\+(\d+) (.*)', line.rstrip('\n'))
        if m:
            k = int(m.group(1))
            if k > len(vals):
                sys.exit('anchors: the script needs mark %d, the original logged %d (%s)' % (k, len(vals), mk))
            line = '%d %s\n' % (vals[k - 1] + int(m.group(2)), m.group(3))
        lines.append(line)
    open(out, 'w', encoding='utf-8', newline='\n').write(''.join(lines))


def main():
    if len(sys.argv) == 3 and sys.argv[1] == 'marks':
        print(','.join(str(v) for v in marks(sys.argv[2])))
    elif len(sys.argv) == 5 and sys.argv[1] == 'fill':
        fill(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
