# showmem.py <dir>: the medians of altmem.sh's runs: per game and configuration the peak private bytes and the peak working set of
# the whole scripted play (perfmin.py's "peak private MB" and "peak working set MB"). An original whose origrun.log says it found no window
# in the first 20 s is skipped and named.
import glob, json, os, statistics, sys
d = sys.argv[1]
for game in ('mahjongjapan', 'mahjongchina'):
    print(game)
    for cfg, pat in (('o', 'o*/orig1'), ('p1', 'p1_*/port1'), ('p2', 'p2_*/port1'), ('pf', 'pf_*/port1')):
        rows = []
        for run in sorted(glob.glob(os.path.join(d, game, pat))):
            f = os.path.join(run, 'perf.json')
            if not os.path.exists(f):
                continue
            r = json.load(open(f))
            lg = os.path.join(run, 'origrun.log')
            if cfg == 'o' and os.path.exists(lg) and 'no window in the first' in open(lg, errors='replace').read():
                print('  skip', run, 'origrun: no window in the first 20 s'); continue
            rows.append((r['peak private MB'], r['peak working set MB'], max(s[4] for s in r['samples'])))
        if rows:
            print('  %-2s n=%d  peak private %s MB (median %.1f)  peak working set %s MB (median %.1f)  threads %s' % (
                cfg, len(rows), '/'.join('%.1f' % x[0] for x in rows), statistics.median(x[0] for x in rows),
                '/'.join('%.1f' % x[1] for x in rows), statistics.median(x[1] for x in rows), '/'.join(str(x[2]) for x in rows)))
