# showmedians.py <dir>: per game, the cycles of the alternating runs under <dir>/<game>/{p,o}N/*/perf.json
import json, glob, os, statistics, sys
d = sys.argv[1]
for game in ('mahjongjapan', 'mahjongchina'):
    rows = {'port': [], 'orig': []}
    for kind, pat in (('port', 'p*/port1/perf.json'), ('orig', 'o*/orig1/perf.json')):
        for f in sorted(glob.glob(os.path.join(d, game, pat))):
            r = json.load(open(f))
            if abs(r['wall'] - 30.1) > 0.6:
                print('skip', f, 'wall', r['wall'])
                continue
            main = sum(t['cycles'] for t in r['threads'] if t['start'].lower().startswith(game) or t['start'].lower().startswith('mahjongescape'))
            audio = sum(t['cycles'] for t in r['threads'] if t['start'].lower().startswith('sdl2'))
            total = sum(t['cycles'] for t in r['threads'])
            rows[kind].append((main / 1e9, (total - main - audio) / 1e9, audio / 1e9, total / 1e9, os.path.basename(os.path.dirname(os.path.dirname(f)))))
    print(game)
    for kind in ('port', 'orig'):
        for x in rows[kind]:
            print('  %-4s %s main %.3f  driver+other %.3f  audio %.3f  total %.3f' % (kind, x[4], x[0], x[1], x[2], x[3]))
    for kind in ('port', 'orig'):
        if rows[kind]:
            med = lambda i: statistics.median(x[i] for x in rows[kind])
            lo = min(x[3] for x in rows[kind]); hi = max(x[3] for x in rows[kind])
            print('  %-4s median: main %.3f driver+other %.3f audio %.3f total %.3f (range %.2f-%.2f, n=%d); without the audio thread %.3f' % (
                kind, med(0), med(1), med(2), med(3), lo, hi, len(rows[kind]), med(3) - med(2)))
    if rows['port'] and rows['orig']:
        p = statistics.median(x[3] - x[2] for x in rows['port']); o = statistics.median(x[3] for x in rows['orig'])
        print('  port without its mixer / original: %.3f' % (p / o))
