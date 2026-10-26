# The fun lab's report (games/bejeweled/docs/GAME_MODES.md): sums up bejeweled_verify funlab's "GAME {...}" lines per
# file (a candidate mode), game mode and bot, and marks each measure against its band: "<" below it, ">" above.
#
#   python games/bejeweled/tools/funlab/report.py <label>.txt... [--json <file>]
import argparse, json, os, statistics, sys

# The bands: what a mode that plays well shows for the skilled bot (the novice's own rows say only how long a new
# player's games last, and its scores give the skill ratio). Minutes are game time at the runner's look per move. The
# rewards' bands are centred on the original rules' measures (cascade % 41, big/100 9 for the skilled bot at 20 seeds),
# since they are what Bejeweled feels like; tension and fairness are design targets, where the original's Normal mode
# is weak (2.5 % danger, a third of its game overs unwarned).
BANDS = {
    'minutes':       {'normal': (8, 45), 'timed': (4, 15)},
    'novice min':    {'normal': (3, None), 'timed': (2, None)},
    'moves/level':   {'normal': (15, 60), 'timed': (6, 30)},
    'choices':       (4, None),
    'forced %':      (None, 10),
    'cascade %':     (25, 55),
    'big/100':       (4, 14),
    'danger %':      (2, 35),
    'close calls':   {'normal': (1, None), 'timed': (0.5, None)},
    'warned %':      (60, None),
    'skill x':       (1.5, None),
}


def median(xs):
    return statistics.median(xs) if xs else 0


def mark(value, band):
    if value is None or band is None:
        return ' '
    lo, hi = band
    if lo is not None and value < lo:
        return '<'
    if hi is not None and value > hi:
        return '>'
    return ' '


def summary(games):
    moves = sum(g['moves'] for g in games) or 1
    decisions = sum(g['decisions'] for g in games) or 1
    minutes = sum(g['minutes'] for g in games) or 1e-9
    overs = [g for g in games if g['end'] != 'capped']
    ends = {}
    for g in games:
        ends[g['end']] = ends.get(g['end'], 0) + 1
    levels = [m for g in games for m in g['levelMoves']]
    return {
        'games': len(games),
        'minutes': median([g['minutes'] for g in games]),
        'moves': median([g['moves'] for g in games]),
        'level': median([g['level'] for g in games]),
        'score': median([g['score'] for g in games]),
        'ends': ', '.join('%s %d%%' % (k, round(100.0 * v / len(games))) for k, v in sorted(ends.items(), key=lambda kv: -kv[1])),
        'moves/level': median(levels) if levels else None,
        'choices': median([g['choiceMedian'] for g in games]),
        'forced %': 100.0 * sum(g['forced'] for g in games) / decisions,
        'cascade %': 100.0 * sum(g['cascades'] for g in games) / moves,
        'big cascade %': 100.0 * sum(g['bigCascades'] for g in games) / moves,
        'big/100': 100.0 * sum(g['big'] for g in games) / moves,
        'events/min': sum(g['events'] for g in games) / minutes,
        'danger %': 100.0 * sum(g['danger'] for g in games) / decisions,
        'close calls': statistics.mean([g['closeCalls'] for g in games]) if games else 0,
        'warned %': (100.0 * sum(1 for g in overs if g['warned']) / len(overs)) if overs else None,
    }


def band(name, mode):
    b = BANDS.get(name)
    return b.get(mode) if isinstance(b, dict) else b


def fmt(v):
    if v is None:
        return '-'
    if isinstance(v, float):
        return ('%.1f' % v) if abs(v) < 100 else ('%.0f' % v)
    return str(v)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('files', nargs='+')
    ap.add_argument('--json', default='')
    a = ap.parse_args()
    out = {}
    for path in a.files:
        label = os.path.splitext(os.path.basename(path))[0]
        games = []
        for line in open(path, encoding='utf-8', errors='replace'):
            if line.startswith('GAME '):
                games.append(json.loads(line[5:]))
        for mode in ('normal', 'timed'):
            rows = {p: [g for g in games if g['mode'] == mode and g['policy'] == p] for p in ('skilled', 'novice')}
            if not rows['skilled'] and not rows['novice']:
                continue
            s = summary(rows['skilled']) if rows['skilled'] else None
            n = summary(rows['novice']) if rows['novice'] else None
            if s and n:
                s['skill x'] = (s['score'] / n['score']) if n['score'] > 0 else None
                s['novice min'] = n['minutes']
            measures = ['minutes', 'novice min', 'moves/level', 'choices', 'forced %', 'cascade %', 'big/100', 'danger %',
                        'close calls', 'warned %', 'skill x']
            if mode == 'timed':
                measures.remove('forced %')
            r = s or n
            inband = sum(1 for m in measures if r.get(m) is not None and mark(r.get(m), band(m, mode)) == ' ')
            counted = sum(1 for m in measures if r.get(m) is not None)
            print('%s, %s: %d+%d games; skilled: %s min, %s moves, level %s, score %s; ends %s; in band %d/%d' % (
                label, mode, len(rows['skilled']), len(rows['novice']), fmt(r['minutes']), fmt(r['moves']), fmt(r['level']),
                fmt(r['score']), r['ends'], inband, counted))
            print('    ' + '  '.join('%s %s%s' % (m, fmt(r.get(m)), mark(r.get(m), band(m, mode))) for m in measures) +
                  '  events/min %s  big cascade %% %s' % (fmt(r['events/min']), fmt(r['big cascade %'])))
            if n:
                print('    novice: %s min, %s moves, level %s, score %s; ends %s; cascade %% %s, danger %% %s' % (
                    fmt(n['minutes']), fmt(n['moves']), fmt(n['level']), fmt(n['score']), n['ends'], fmt(n['cascade %']),
                    fmt(n['danger %'])))
            out['%s/%s' % (label, mode)] = {'skilled': s, 'novice': n, 'inBand': inband, 'counted': counted}
    if a.json:
        with open(a.json, 'w', newline='\n') as f:
            json.dump(out, f, indent=1)


main()
