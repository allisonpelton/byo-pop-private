#!/usr/bin/env python3
"""The pairs of pairs.sh: process cycles an update of the port and of the original between the snapshots a and b, medians.

    python games/hammerheads/tools/perf/pairsum.py <mode>        # mode: total or split
"""
import glob
import json
import os
import statistics
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))


def stretch(path):
    snaps = {}
    for line in open(path):
        line = line.strip()
        if line.startswith('{'):
            j = json.loads(line)
            snaps[j['label']] = j
    a, b = snaps['a'], snaps.get('b') or snaps['exit']   # a paused port takes no snapshot b: its exit one, ten updates later, stands in
    updates = b['update'] - a['update']
    parts = {k: (b['parts'][k]['cycles'] - a['parts'][k]['cycles']) / updates / 1e6 for k in ('update', 'draw', 'present')}
    return (b['process_cycles'] - a['process_cycles']) / updates / 1e6, parts


def main():
    mode = sys.argv[1]
    base = os.path.join(ROOT, 'build', 'perf', 'hammerheads')
    rows = []
    for n in range(1, 99):
        p, o = (os.path.join(base, '%s%d-%s' % (mode, n, side), side + '.json') for side in ('port', 'orig'))
        if os.path.exists(p) and os.path.exists(o):
            rows.append((n, stretch(p), stretch(o)))
    for n, (pt, pp), (ot, op) in rows:
        print('pair %d: port %.3f, original %.3f M cycles an update (%.2f)' % (n, pt, ot, pt / ot))
    if rows:
        pm, om = statistics.median(r[1][0] for r in rows), statistics.median(r[2][0] for r in rows)
        print('median of %d: port %.3f, original %.3f M cycles an update, ratio %.2f' % (len(rows), pm, om, pm / om))
        if mode == 'split':
            for k in ('update', 'draw', 'present'):
                print('  %s: port %.3f, original %.3f' % (k, statistics.median(r[1][1][k] for r in rows), statistics.median(r[2][1][k] for r in rows)))


if __name__ == '__main__':
    main()
