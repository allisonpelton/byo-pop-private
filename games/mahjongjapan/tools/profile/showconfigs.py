# showconfigs.py <dir>: the medians of altconfigs.sh's runs: per game and configuration (o, p1, p2, pf) the main thread, the
# audio thread (SDL's mixer), the video driver and other threads, and the total, in billions of cycles over the same 30 s. A run
# whose window is not 30.1 s long, or an original whose origrun.log says it found no window in the first 20 s (its first click
# was lost and it idled), is skipped and named.
import glob, json, os, statistics, sys
d = sys.argv[1]
for game in ('mahjongjapan', 'mahjongchina'):
    print(game)
    base = None
    for cfg, pat in (('o', 'o*/orig1'), ('p1', 'p1_*/port1'), ('p2', 'p2_*/port1'), ('pf', 'pf_*/port1')):
        rows = []
        for run in sorted(glob.glob(os.path.join(d, game, pat))):
            f = os.path.join(run, 'perf.json')
            if not os.path.exists(f):
                continue
            r = json.load(open(f))
            if abs(r['wall'] - 30.1) > 0.6:
                print('  skip', run, 'wall', r['wall']); continue
            lg = os.path.join(run, 'origrun.log')
            if cfg == 'o' and os.path.exists(lg) and 'no window in the first' in open(lg, errors='replace').read():
                print('  skip', run, 'origrun: no window in the first 20 s'); continue
            main = sum(t['cycles'] for t in r['threads'] if t['start'].lower().startswith(game) or t['start'].lower().startswith('mahjongescape'))
            audio = sum(t['cycles'] for t in r['threads'] if t['start'].lower().startswith('sdl2'))
            total = sum(t['cycles'] for t in r['threads'])
            rows.append((main / 1e9, (total - main - audio) / 1e9, audio / 1e9, total / 1e9, os.path.basename(os.path.dirname(run))))
        if not rows:
            continue
        for x in rows:
            print('  %-2s %-6s main %.2f  other %.2f  audio %.2f  total %.2f' % (cfg, x[4], x[0], x[1], x[2], x[3]))
        med = lambda i: statistics.median(x[i] for x in rows)
        print('  %-2s MEDIAN main %.2f  other %.2f  audio %.2f  total %.2f (range %.2f-%.2f, n=%d); without audio %.2f%s' % (
            cfg, med(0), med(1), med(2), med(3), min(x[3] for x in rows), max(x[3] for x in rows), len(rows), med(3) - med(2),
            '' if base is None else '; of the original %.2f (%.2f without audio)' % (med(3) / base[0], (med(3) - med(2)) / base[1])))
        if cfg == 'o':
            base = (med(3), med(3) - med(2))
