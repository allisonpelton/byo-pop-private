# runmedians.py <dir> [game]: the runs <dir>/{port,orig}N/perf.json of perfrun.sh with TOOL=threadprof.py: per run and the median, in G cycles
# for the window of 30 s: the game's main thread, the other threads (the renderer's driver thread) and the mixer thread
import json, glob, os, statistics, sys
d = sys.argv[1]
game = sys.argv[2] if len(sys.argv) > 2 else os.path.basename(d.rstrip('/')).split('-')[0]
for kind in ('port', 'orig'):
    rows = []
    for f in sorted(glob.glob(os.path.join(d, kind + '[0-9]*', 'perf.json'))):
        r = json.load(open(f))
        if abs(r['wall'] - 30.1) > 0.6:
            print('skip', f, 'wall', r['wall'])
            continue
        main = sum(t['cycles'] for t in r['threads'] if t['start'].lower().startswith(game) or t['start'].lower().startswith('mahjongescape'))
        audio = sum(t['cycles'] for t in r['threads'] if t['start'].lower().startswith('sdl2'))
        total = sum(t['cycles'] for t in r['threads'])
        rows.append((main / 1e9, (total - main - audio) / 1e9, audio / 1e9, total / 1e9, os.path.basename(os.path.dirname(f))))
    for x in rows:
        print('  %-5s %-6s main %.3f  other %.3f  mixer %.3f  total %.3f  (without mixer %.3f)' % (kind, x[4], x[0], x[1], x[2], x[3], x[3] - x[2]))
    if rows:
        print('  %s median: total %.3f, without the mixer %.3f (n=%d)' % (kind, statistics.median(x[3] for x in rows),
              statistics.median(x[3] - x[2] for x in rows), len(rows)))
