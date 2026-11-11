# softpath_table.py [folder]: the table of softpath.sh's runs (build/perf/sp by default), as Markdown.
# For each way of drawing (d, d1, s1, s; see softpath.sh), windowed and in full screen: the private bytes before the
# game loads anything, at the title, in play and at the end of the game (the memreport lines of the -fast game), the
# peak; the processor time and cycles of the -fast game and of the game in real time, with the frames the real-time game
# drew; and the time a frame takes, from each pair of runs that differ only in -drawevery: the difference in wall time
# over the difference in frames, and the same for the processor's cycles.
import json, os, sys

folder = sys.argv[1] if len(sys.argv) > 1 else 'build/perf/sp'


def load(name):
    p = os.path.join(folder, name + '.json')
    return json.load(open(p)) if os.path.exists(p) else None


def mb(n):
    return '%.1f' % (n / 1048576.0)


def frame(fast, every):
    if not fast or not every or not fast.get('closing') or not every.get('closing') or every.get('timed_out') or fast.get('timed_out'):
        return None
    frames = every['closing']['draws'] - fast['closing']['draws']
    if frames <= 0:
        return None
    return ((every['wall_s'] - fast['wall_s']) * 1000.0 / frames, (every['cycles'] - fast['cycles']) / frames / 1e6, frames)


NAMES = {'d': 'default path', 'd1': 'default path, drawn at 640 x 480', 's1': 'software, drawn at 640 x 480', 's': 'software, drawn at the window\'s scale'}
print('| drawing | window | private at start / title / play / end (MB) | peak private (MB) | -fast game: s, G cycles | real-time game: s, G cycles, frames | frame, short pair: ms, M cycles (frames) | frame, long pair: ms, M cycles (frames) |')
print('|---|---|---|---|---|---|---|---|')
for v in ('d', 'd1', 's1', 's'):
    for fs in (0, 1):
        k = '%s-fs%d-' % (v, fs)
        gf, gr = load(k + 'game-fast'), load(k + 'game-real')
        if not gf and not gr:
            continue
        mem = '-'
        if gf and gf.get('memreports'):
            r = gf['memreports']
            mem = mb(r[0]['process']['privateAtStart']) + ' / ' + ' / '.join(mb(x['process']['private']) for x in r)
        peak = mb(max(x['peak_private'] for x in (gf, gr) if x)) if (gf or gr) else '-'
        fast = '%.2f, %.2f' % (gf['cpu_s'], gf['cycles'] / 1e9) if gf else '-'
        real = '-'
        if gr:
            real = '%.2f, %.2f, %s%s' % (gr['cpu_s'], gr['cycles'] / 1e9, gr['closing']['draws'] if gr.get('closing') else '?',
                                         ' (stopped at the time limit)' if gr.get('timed_out') else (' in %.0f s' % gr['wall_s']))
        cells = []
        for pair in (('short-fast', 'short-every'), ('game-fast', 'game-every')):
            f = frame(load(k + pair[0]), load(k + pair[1]))
            cells.append('%.2f, %.1f (%d)' % f if f else '-')
        print('| %s | %s | %s | %s | %s | %s | %s | %s |' % (NAMES[v], 'full screen' if fs else 'window', mem, peak, fast, real, cells[0], cells[1]))
