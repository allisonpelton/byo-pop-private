"""pairsum.py <out dir>: pairs.sh's runs summed up: each side's medians and ranges of the processor time over the cycle window
(G cycles, and the port's without SDL's audio thread), the private bytes at the hold and the peak private bytes."""
import glob, json, os, statistics, sys

out = sys.argv[1]


def runs(prefix):
    rows = []
    for d in sorted(glob.glob(os.path.join(out, prefix + '[0-9]*'))):
        f = os.path.join(d, 'summary.json')
        if os.path.isdir(d) and os.path.exists(f):
            s = json.load(open(f))
            if 'cycles' in s and s.get('atHold'):
                c = s['cycles']
                rows.append((c['process'] / 1e9, (c['process'] - c['audio']) / 1e9, c['to'] - c['from'],
                             s['atHold']['private'] / 1048576.0, s['peakPrivate'] / 1048576.0))
    return rows


def line(name, rows):
    if not rows:
        return '%s: no runs' % name
    col = lambda i: [r[i] for r in rows]
    fmt = lambda v: '%.2f (%.2f to %.2f)' % (statistics.median(v), min(v), max(v))
    return ('%s, %d runs: %s G cycles in %.0f s, %s without the audio thread; private at the hold %s MB, peak %s MB'
            % (name, len(rows), fmt(col(0)), statistics.median(col(2)), fmt(col(1)), fmt(col(3)), fmt(col(4))))


print(line('port', runs('p')))
print(line('original', runs('o')))
