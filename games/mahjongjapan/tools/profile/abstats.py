# The medians of interleaved runs (chain-style folders <root>/<kind><n>/port1/perf.json): python abstats.py <root> before after
import glob
import json
import statistics
import sys

root = sys.argv[1]
for kind in sys.argv[2:]:
    runs = []
    for f in sorted(glob.glob('%s/%s*/port1/perf.json' % (root, kind))):
        d = json.load(open(f))
        runs.append(d)
    if not runs:
        print(kind, 'no runs')
        continue

    def med(key):
        values = [r[key] for r in runs if key in r]
        return statistics.median(values), min(values), max(values)

    wc = med('window cpu percent of one core')
    cs = med('cpu seconds, whole run')
    wm = med('window peak private MB')
    pk = med('peak working set MB')
    print('%-7s %d runs: window cpu %% of one core median %.1f (%.1f to %.1f); whole run %.1f s processor (%.1f to %.1f); '
          'window peak private %.0f MB, peak working set %.0f MB' % (kind, len(runs), wc[0], wc[1], wc[2], cs[0], cs[1], cs[2], wm[0], pk[0]))
