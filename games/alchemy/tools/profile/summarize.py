"""summarize.py <runs.txt> [--ref ref.json] [--tol N] [--minn N]: the figures of pairs.sh's runs, segment by segment.

The original plays the bot's script on the real clock and falls out of step with it for good after a while (docs: GAME_NOTES.md,
"The software screen at 1x"); the port plays it exactly, every run. A segment of 500 updates is therefore compared only where the
original's board (level, score) at both of its ends was where the port's was (the same level and a score within --tol, default 25:
the marks are read a few updates apart). For each configuration and each such segment the cycles for 1000 updates of every run that
covers it are collected (the port's runs all cover every segment; the original's, those that were still in step); the figures are
the median and the minimum over those runs, averaged over the leading segments that at least --minn (default 3) original runs
covered. The minimum is what the processor did when the machine left it alone (a busy sibling thread, a build or another game
inflates a run's cycles by a third or more, and the runs differ that much); the median is the typical run. The port's mixer thread
(SDLAudio*) is taken off by its share of the whole run's cycles. Memory is the peak private bytes at the end of each whole run
(whatever the original's play); draws are the median draws for 1000 updates."""
import json
import os
import statistics
import sys

path = sys.argv[1]
arg = lambda name, default: sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default
tol = int(arg('--tol', 25))
minn = int(arg('--minn', 3))
ref_path = arg('--ref', os.path.join(os.path.dirname(path), 'ref.json'))
ref = json.load(open(ref_path))
runs = []                      # (config, result)
for line in open(path):
    if line.startswith(('start', 'end', 'stopped')):
        continue
    cfg, _, js = line.partition(' ')
    try:
        r = json.loads(js)
    except ValueError:
        continue
    if 'segments' not in r:
        continue
    runs.append((cfg, r))


def tracked_marks(r):
    n = 0
    for x, y in zip(r.get('level_score', []), ref):
        if x[0] is None or x[0] != y[0] or abs(x[1] - y[1]) > tol:
            break
        n += 1
    return n


def covered(cfg, r):
    """how many leading segments the run covers: all for a port (it plays the script exactly), for the original the segments
    between marks it was in step at (an original run the script's own checks marked invalid still counts for what it covered)"""
    if cfg != 'orig':
        return 0 if r.get('invalid') else len(r['segments'])
    return max(0, tracked_marks(r) - 1)


per = {}                       # config -> segment -> [(value, mixerless value, draws)]
whole = {}
for cfg, r in runs:
    if r.get('invalid') and cfg != 'orig':
        continue
    mixer = sum(t[1] for t in r.get('threads', []) if t[0].startswith('SDLAudio'))
    share = mixer / r['gcycles'] if r.get('gcycles') else 0
    for i, s in enumerate(r['segments'][:covered(cfg, r)]):
        if s['upd'] <= 0:
            continue
        v = s['g'] * 1000.0 / s['upd']
        per.setdefault(cfg, {}).setdefault(i, []).append((v, v * (1 - share), s['draws'] * 1000.0 / s['upd']))
    if r.get('peak_private_mb'):
        whole.setdefault(cfg, []).append(r['peak_private_mb'])

orig = per.get('orig', {})
counts = [len(orig.get(i, [])) for i in range(max(orig) + 1)] if orig else []
K = 0
while K < len(counts) and counts[K] >= minn:
    K += 1
print('original runs covering each segment of 500 updates from update 600: %s (of %d original runs)'
      % (counts, sum(1 for c, _ in runs if c == 'orig')))
print('segments compared: the first %d (updates 600 to %d)' % (K, 600 + 500 * K))
if K == 0:
    sys.exit(1)
res = {}
for cfg in per:
    segs = [per[cfg][i] for i in range(K) if i in per[cfg]]
    med = statistics.mean(statistics.median(x[0] for x in s) for s in segs)
    medm = statistics.mean(statistics.median(x[1] for x in s) for s in segs)
    low = statistics.mean(min(x[0] for x in s) for s in segs)
    lowm = statistics.mean(min(x[1] for x in s) for s in segs)
    dr = statistics.mean(statistics.median(x[2] for x in s) for s in segs)
    res[cfg] = (med, medm, low, lowm)
    pk = whole.get(cfg, [])
    print('%-10s runs %2d  G/1000 upd: median %.3f (without mixer %.3f), best %.3f (without mixer %.3f)  draws/1000 upd %.0f'
          '  peak MB median %.1f (min %.1f, max %.1f)'
          % (cfg, len(pk), med, medm, low, lowm, dr, statistics.median(pk) if pk else 0, min(pk) if pk else 0, max(pk) if pk else 0))
if 'orig' in res:
    for cfg, v in res.items():
        if cfg != 'orig':
            o = res['orig']
            print('%-10s over the original: median %.2f (without mixer %.2f), best %.2f (without mixer %.2f)'
                  % (cfg, v[0] / o[0], v[1] / o[0], v[2] / o[2], v[3] / o[2]))
