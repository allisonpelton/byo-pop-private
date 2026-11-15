"""The intervals between the presents of runs that wrote a present log (-presentlog; docs/TEST_COMMANDS.md, "The present log"):

    python -I games/astropop/tools/profile/present_intervals.py <set dir> [<set dir> ...]

For every run folder holding presents.txt (the port's runs of vsync.sh: port-vsync-N, port-pace-N): the header (renderer, vsync asked
and granted, the pace in ms; pace_ms 0 means the renderer's own vertical blank holds the presents), then over the stretch of the run
that the loop was playing (from 20 s after the first present on, so that the load and the title are out; the whole log is 60 s) the
number of presents, the median interval between their starts, the 5th and 95th percentile and the largest, the share of intervals
within a quarter of the median (an even cadence: 1.0), the share of intervals that are a multiple of the median's frame (a missed
beat), and the median and mean length of the SDL_RenderPresent call. Per kind it gives the medians over the runs.
"""
import glob
import os
import statistics
import sys


def read(path):
    header, rows = '', []
    for line in open(path):
        if line.startswith('# renderer'):
            header = line[2:].strip()
        elif line.startswith('#') or not line.strip():
            continue
        else:
            a, b = line.split()
            rows.append((float(a), float(b)))
    return header, rows


def pct(v, p):
    v = sorted(v)
    return v[min(len(v) - 1, int(p * len(v)))]


def analyse(path, skip_us=20e6):
    header, rows = read(path)
    rows = [r for r in rows if r[0] >= skip_us]
    if len(rows) < 20:
        return header, None
    starts = [r[0] for r in rows]
    iv = [(b - a) / 1000.0 for a, b in zip(starts, starts[1:])]      # ms
    med = statistics.median(iv)
    even = sum(1 for x in iv if abs(x - med) <= 0.25 * med) / len(iv)
    missed = sum(1 for x in iv if x > 1.5 * med) / len(iv)
    dur = [r[1] / 1000.0 for r in rows]
    span = (starts[-1] - starts[0]) / 1e6
    return header, {'n': len(rows), 'per_s': len(rows) / span if span else 0, 'median': med, 'p5': pct(iv, 0.05), 'p95': pct(iv, 0.95),
                    'max': max(iv), 'even': even, 'missed': missed, 'call_med': statistics.median(dur), 'call_mean': sum(dur) / len(dur)}


def main():
    for d in sys.argv[1:]:
        print('== %s' % d)
        kinds = {}
        for f in sorted(glob.glob(os.path.join(d, 'port-*', 'presents.txt'))):
            header, a = analyse(f)
            kind = os.path.basename(os.path.dirname(f)).rsplit('-', 1)[0]
            print('%-16s %s' % (os.path.basename(os.path.dirname(f)), header))
            if a:
                print('    %d presents (%.1f a second): interval ms median %.2f, 5th %.2f, 95th %.2f, largest %.1f; %.0f%% within a quarter of the median, %.1f%% over one and a half; call ms median %.2f, mean %.2f' %
                      (a['n'], a['per_s'], a['median'], a['p5'], a['p95'], a['max'], 100 * a['even'], 100 * a['missed'], a['call_med'], a['call_mean']))
                kinds.setdefault(kind, []).append(a)
        for kind, v in kinds.items():
            print('%s, median of %d runs: %.1f presents a second, interval median %.2f ms (5th %.2f, 95th %.2f), %.0f%% even, %.1f%% missed beats, call mean %.2f ms' %
                  (kind, len(v), statistics.median(x['per_s'] for x in v), statistics.median(x['median'] for x in v), statistics.median(x['p5'] for x in v),
                   statistics.median(x['p95'] for x in v), 100 * statistics.median(x['even'] for x in v), 100 * statistics.median(x['missed'] for x in v),
                   statistics.median(x['call_mean'] for x in v)))
        print()


if __name__ == '__main__':
    main()
