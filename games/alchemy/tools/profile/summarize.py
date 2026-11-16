"""summarize.py <runs.txt> [--tol N]: the figures of pairs.sh's runs. For each round, the leading marks at which the original's
board (level, score) was where the port's was (the same level and a score within --tol, default 25: the marks are read a
few updates apart) say how long the original kept up with the bot's script on the real clock; the segments inside that
are the stretch both played the same game in, and the cycles of every run in the round are summed over those segments.
Runs marked "invalid" are left out. Prints, per configuration, the cycles for 1000 updates and the draws for 1000 updates
(medians over the rounds, the values beside them), the whole minute's cycles of the port with and without its mixer
thread, and the peak private bytes at the end of the stretch (medians; the whole run, whatever the original's play)."""
import json
import statistics
import sys

path = sys.argv[1]
tol = int(sys.argv[sys.argv.index('--tol') + 1]) if '--tol' in sys.argv else 25
runs = []                      # (round, config, result)
rnd, seen = 0, {}
for line in open(path):
    if line.startswith(('start', 'end', 'stopped')):
        continue
    cfg, _, js = line.partition(' ')
    try:
        r = json.loads(js)
    except ValueError:
        continue
    if r.get('invalid') or 'segments' not in r:
        continue
    seen[cfg] = seen.get(cfg, 0) + 1
    runs.append((seen[cfg], cfg, r))        # the n-th valid run of a configuration: its round, as the discards are redone


def tracks(a, b):
    n = 0
    for x, y in zip(a, b):
        if x[0] is None or x[0] != y[0] or abs(x[1] - y[1]) > tol:
            break
        n += 1
    return n


rounds = sorted({n for n, _, _ in runs})
table = {}
for n in rounds:
    here = {c: r for m, c, r in runs if m == n}
    if 'orig' not in here or 'port1x' not in here:
        continue
    marks = tracks(here['orig']['level_score'], here['port1x']['level_score'])
    segs = max(0, marks - 1)                # marks 0..marks-1 agree: the segments between them
    if segs == 0:
        print('round %d: the original had no segment in step with the port (marks %s against %s)'
              % (n, here['orig']['level_score'][:3], here['port1x']['level_score'][:3]))
        continue
    print('round %d: the original kept up for %d segments of 500 updates (to update %d)' % (n, segs, 600 + 500 * segs))
    for c, r in here.items():
        g = sum(s['g'] for s in r['segments'][:segs])
        u = sum(s['upd'] for s in r['segments'][:segs])
        d = sum(s['draws'] for s in r['segments'][:segs])
        mixer = sum(t[1] for t in r['threads'] if t[0].startswith('SDLAudio'))
        share = mixer / r['gcycles'] if r['gcycles'] else 0
        row = table.setdefault(c, {'g': [], 'gm': [], 'd': [], 'peak': [], 'segs': [], 'whole': []})
        row['g'].append(g * 1000.0 / u)
        row['gm'].append(g * (1 - share) * 1000.0 / u)
        row['d'].append(d * 1000.0 / u)
        row['peak'].append(r['peak_private_mb'])
        row['segs'].append(segs)
        row['whole'].append(r['gcycles'] * 6000.0 / r['updates'])
for c, row in table.items():
    med = lambda k: round(statistics.median(row[k]), 3)
    print('%-10s rounds %d  G/1000 upd %s (median %.3f; without the mixer %.3f)  draws/1000 upd median %.0f  peak MB %s (median %.1f)'
          '  whole stretch G per 6000 upd %s'
          % (c, len(row['g']), [round(x, 3) for x in row['g']], med('g'), med('gm'), med('d'),
             row['peak'], statistics.median(row['peak']), [round(x, 2) for x in row['whole']]))
orig = table.get('orig')
for c, row in table.items():
    if c != 'orig' and orig:
        pairs = list(zip(row['g'], orig['g'][:len(row['g'])]))
        print('%-10s over the original, per round: %s' % (c, [round(a / b, 3) for a, b in pairs]))
