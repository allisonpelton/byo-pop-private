#!/usr/bin/env python3
"""Which functions of the port draw random numbers in a window of updates (a debugging aid, used with a temporary
log in GameRand::Get: "<update> <return address> <range>" lines and a "BASE <address of a global>" first line).

    python games/atomica/tools/identical/rngsites.py <rnglog.txt> <nm -n output> <global's nm address> <from> <to>
"""
import bisect
import collections
import sys


def main():
    log, nm, glob, lo, hi = sys.argv[1], sys.argv[2], int(sys.argv[3], 16), int(sys.argv[4]), int(sys.argv[5])
    syms = []
    for l in open(nm):
        p = l.split(None, 2)
        if len(p) == 3 and p[1] in 'tTwW':
            syms.append((int(p[0], 16), p[2].strip()))
    syms.sort()
    keys = [s[0] for s in syms]
    lines = open(log).read().split('\n')
    base = int(lines[0].split()[1], 16) - glob
    count = collections.Counter()
    for l in lines[1:]:
        w = l.split()
        if len(w) != 3:
            continue
        u = int(w[0])
        if lo <= u <= hi:
            a = int(w[1], 16) - base
            i = bisect.bisect_right(keys, a) - 1
            count[(syms[i][1][:90], int(w[2]))] += 1
    for (name, rng), n in count.most_common():
        print('%5d  range %-6d %s' % (n, rng, name))


main()
