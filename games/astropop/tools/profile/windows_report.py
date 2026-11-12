"""Summarises the runs of windows.sh (one set directory: <out>/2d-total, 3d-split and the like):

    python -I games/astropop/tools/profile/windows_report.py [--busy-max 0.6] <set dir> [<set dir> ...]

For each kind of run (orig, port-w1: the port at the game's own size, port-w2: its default window, twice the size) the stretch's
processor time in G cycles from the runs' JSON lines (runs.txt), per update beside it (the stretch is 2,000 to 2,030 updates; the
original's and the port's are counted by their own update counters), the audio thread apart, the median, the range and the
window state and renderer each run reported. For a set run with SPLIT=1 the cycle split by tools/cyclesplit.py, read from each run's
split.jsonl (the port) or counts.jsonl (the original) between its snapshots a and b: M cycles an update for update, picture (draw,
present and the driver's threads), audio, loop and the rest, as medians of the runs.
"""
import glob
import json
import os
import re
import statistics
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CYCLESPLIT = os.path.join(HERE, '..', '..', '..', '..', 'tools', 'cyclesplit.py')
HZ = 2112000000      # the nominal clock, as the notes' figures (the split's shares of a core are not used here)


def runs(d):
    out = []
    for line in open(os.path.join(d, 'runs.txt')):
        if not line.strip() or line.startswith(('start', 'end')):
            continue
        kind, js = line.split(' ', 1)
        try:
            out.append((kind, json.loads(js)))
        except ValueError:
            out.append((kind, None))
    return out


def med(v):
    return statistics.median(v) if v else None


def fmt(v, n=2):
    return '-' if v is None else ('%.*f' % (n, v))


def split_of(path):
    r = subprocess.run([sys.executable, '-I', CYCLESPLIT, path, '--from', 'a', '--to', 'b', '--hz', str(HZ), '--json'], capture_output=True, text=True)
    if r.returncode:
        return None
    return json.loads(r.stdout)[0]


def report(d, busy_max=9.0):
    print('== %s%s' % (d, ('  (only runs with the machine at most %.2f busy)' % busy_max) if busy_max < 9 else ''))
    rs = runs(d)
    kinds = []
    for k, _ in rs:
        if k not in kinds:
            kinds.append(k)
    print('%-9s %4s  %-34s %-26s %-9s' % ('kind', 'runs', 'stretch G cycles', 'per update (M cycles)', 'audio G'))
    for k in kinds:
        js = [j for kk, j in rs if kk == k and j and j['machine_busy'] <= busy_max]
        st = [j['scripted_stretch'] for j in js]
        g = [s['cycles_g'] for s in st]
        up = [j.get('updates_in_stretch') or 2030 for j in js]
        pu = [s['cycles_g'] * 1000.0 / u for s, u in zip(st, up)]
        a = [s.get('audio_g', 0) for s in st]
        print('%-9s %4d  %-34s %-26s %-9s' % (k, len(js), ' '.join('%.2f' % x for x in g) + ' (med %.2f)' % med(g),
                                              'med %.3f (%.3f to %.3f)' % (med(pu), min(pu), max(pu)), fmt(med(a))))
        shown = set()
        for j in js:
            for key in ('display', 'graphics_modules', 'flag3d'):
                if j.get(key) is not None and (key, str(j[key])) not in shown:
                    shown.add((key, str(j[key])))
                    print('            %s: %s' % (key, re.sub(r'^INFO: ', '', str(j[key]))))
    # the split, from the files of each run
    names = {'orig': ('orig*', 'counts.jsonl'), 'port-w1': ('port-w1-*', 'split.jsonl'), 'port-w2': ('port-w2-*', 'split.jsonl')}
    rows = {}
    for k in kinds:
        pat, fn = names.get(k, (None, None))
        if not pat:
            continue
        for f in glob.glob(os.path.join(d, pat, '**', fn), recursive=True):
            s = split_of(f)
            try:
                busy = json.load(open(os.path.join(os.path.dirname(f), 'perf.json')))['result']['machine_busy']
            except (OSError, ValueError, KeyError):
                busy = 0
            if s and s['updates'] > 0 and busy <= busy_max:
                n = s['updates'] * 1e6
                pic = s['draw'] + s['present'] + s['driver']
                rows.setdefault(k, []).append({'update': s['update'] / n, 'draw': s['draw'] / n, 'present': s['present'] / n, 'driver': s['driver'] / n,
                                               'picture': pic / n, 'audio': s['audio'] / n, 'loop': s['loop'] / n, 'other': s['other'] / n,
                                               'total': s['process'] / n, 'draw_calls': s['draw_calls'] / s['updates'], 'present_calls': s['present_calls'] / s['updates'],
                                               'readings': s['counter_calls'] * s['counter_each'] / max(s['process'], 1) * 100.0, 'misses': s['misses'],
                                               'threads': [(c / n, kind, label) for c, kind, label in s['threads']]})
    if rows:
        cols = ['update', 'draw', 'present', 'driver', 'picture', 'audio', 'loop', 'other', 'total']
        print('split, M cycles an update, median of the runs (draw, present: the engine\'s own functions; picture = draw + present + driver threads):')
        print('%-9s %4s  ' % ('kind', 'runs') + ' '.join('%8s' % c for c in cols) + '   draws/upd  presents/upd  counting%  misses')
        for k in kinds:
            if k in rows:
                r = rows[k]
                print('%-9s %4d  ' % (k, len(r)) + ' '.join('%8.3f' % med([x[c] for x in r]) for c in cols) +
                      '   %7.2f    %9.2f   %7.2f  %d' % (med([x['draw_calls'] for x in r]), med([x['present_calls'] for x in r]), med([x['readings'] for x in r]),
                                                          sum(x['misses'] for x in r)))
                labels = {}
                for x in r:
                    for c, kind, label in x['threads']:
                        labels.setdefault((kind, label), []).append(c)
                for (kind, label), v in sorted(labels.items(), key=lambda kv: -sum(kv[1]))[:4]:
                    if sum(v) / len(r) > 0.005:
                        print('          thread %-7s %6.3f M an update (mean of the runs)  %s' % (kind, sum(v) / len(r), label))
                print('          each run\'s total: ' + ' '.join('%.3f' % x['total'] for x in r) + '; picture: ' + ' '.join('%.3f' % x['picture'] for x in r))


if __name__ == '__main__':
    args = sys.argv[1:]
    busy_max = 9.0
    if args[:1] == ['--busy-max']:      # leave out the runs taken on a machine busier than this (the fraction the whole machine was busy)
        busy_max = float(args[1])
        args = args[2:]
    for d in args:
        report(d, busy_max)
        print()
