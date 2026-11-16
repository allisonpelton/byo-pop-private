#!/usr/bin/env python3
"""The sums of screens.sh: per way, the port's process cycles an update against the round's original (o<n>-orig), medians, and the peak
private bytes.   python games/hammerheads/tools/perf/screensum.py"""
import os
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pairsum  # noqa: E402

base = os.path.join(pairsum.ROOT, 'build', 'perf', 'hammerheads')
orig = {}
for n in range(1, 99):
    path = os.path.join(base, 'o%d-orig' % n, 'orig.json')
    if os.path.exists(path):
        orig[n] = pairsum.stretch(path)[0]
print('original: %s, median %.3f' % (' '.join('%.2f' % v for v in orig.values()), statistics.median(orig.values())))
for way in ('g3off', 'g3on', 'g3onnv', 's3off'):
    ratios, cycles, peaks = [], [], []
    for n in orig:
        path = os.path.join(base, '%s%d-port' % (way, n), 'port.json')
        if os.path.exists(path):
            c = pairsum.stretch(path)[0]
            cycles.append(c)
            ratios.append(c / orig[n])
            peaks.append(float(open(os.path.join(base, '%s%d-port' % (way, n), 'peak.txt')).read()))
    print('%-7s cycles %s median %.3f; ratios %s median %.2f; peak MB median %.1f (%s)' % (
        way, ' '.join('%.2f' % v for v in cycles), statistics.median(cycles), ' '.join('%.2f' % v for v in ratios),
        statistics.median(ratios), statistics.median(peaks), ' '.join('%.0f' % v for v in peaks)))
