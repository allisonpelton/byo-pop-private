"""summarize.py <runs.txt>: the figures of pairs.sh's runs (GAME_NOTES.md, "Performance"), one line a kind: the median of its runs
with the range, and the other game processes that ran meanwhile.

Processor time is a cycle count over the machine's clock (perf.py: QueryProcessCycleTime, so the scheduler's tick never enters).
Idle kinds (*-idle): the share of one core over the last 20 s of the idle board in real time; the port's also less its SDL audio
thread (the framework's mixer: the original under origrun mixes nothing). Work kinds (*-work): processor seconds after the title for
the recorded game's 6,400 updates (the 3D game's: 7,900) without waiting; a run of the original counts only if it played the recorded
game (final score 580, 3D 1,475 or 1,490), and the port's is given less the commands file's poll (the test channel the original has no
counterpart of). Load kinds (*-load): seconds from the start of the process to the title taking input. Memory: private bytes at the
title (the load runs') and the peak private bytes of the idle run."""
import json
import statistics
import sys


def lines(path):
    out = {}
    for ln in open(path, errors='replace'):
        if ' {' not in ln:
            continue
        kind, js = ln.split(' ', 1)
        try:
            out.setdefault(kind, []).append(json.loads(js))
        except ValueError:
            pass
    return out


def med(xs, digits=2):
    xs = [x for x in xs if x is not None]
    if not xs:
        return 'none'
    f = '%%.%df' % digits
    return (f + ' (' + f + ' to ' + f + ')') % (statistics.median(xs), min(xs), max(xs)) if len(xs) > 1 else f % xs[0]


def stretch(r):
    s = r.get('scripted_stretch')
    return s if s and s.get('wall_s') else None


def share(r):
    s = stretch(r)
    return 100.0 * s['processor_s'] / s['wall_s'] if s else None


def share_less_audio(r):
    s = stretch(r)
    return 100.0 * (s['processor_s'] - s.get('audio_s', 0.0)) / s['wall_s'] if s else None


def after_title(r, orig):
    if orig:
        w = r.get('whole_run')
        return w['processor_s'] - r['processor_s_at_title'] if w and r.get('processor_s_at_title') is not None else None
    return r.get('play_processor_s')


def recorded_game(r, kind):
    """a run of the original that played the recorded game (the port's own state says so in its final_state)"""
    if not kind.startswith('orig'):
        return True
    return r.get('game_started') and r.get('final_score') in (('580', '1475', '1490') if '3d' in kind else ('580',))


runs = lines(sys.argv[1])
for kind in sorted(runs):
    rs = [r for r in runs[kind] if 'error' not in r]
    if not rs:
        print(kind, 'no run')
        continue
    others = '%s others at the start, %s at the end' % (sorted({r.get('others_start') for r in rs}), sorted({r.get('others_end') for r in rs}))
    orig = kind.startswith('orig')
    if kind.endswith('-load'):
        t = [r.get('load_s') if 'load_s' in r else r.get('median_s') for r in rs]
        mem = [r.get('private_mb_at_title') for r in rs]
        print('%-16s load s %s | private MB at the title %s | %d runs | %s' % (kind, med(t), med(mem, 1), len(rs), others))
    elif kind.endswith('-idle'):
        less = '' if orig else ' | less its audio thread %s' % med([share_less_audio(r) for r in rs])
        print('%-16s idle board, %% of a core %s%s | audio s in the 20 s %s | peak private MB %s | %d runs | %s' % (
            kind, med([share(r) for r in rs]), less, med([(stretch(r) or {}).get('audio_s') for r in rs], 3),
            med([(r.get('whole_run') or {}).get('peak_private_mb') for r in rs], 1), len(rs), others))
        print('                 threads of run 1: %s' % '; '.join('%s %.1f s (%d%%)' % (t['thread'], t['cpu_s'], 100 * t['share']) for t in rs[0].get('threads', [])))
        if not orig:
            fs = rs[0].get('final_state') or {}
            print('                 state at the end: %s' % {k: fs.get(k) for k in ('screen', 'dialogs', 'phase', 'score', 'level') if k in fs})
        else:
            print('                 original at the end: score %s, state %s (1 is idle)' % (rs[0].get('final_score'), rs[0].get('final_state')))
    else:
        good = [r for r in rs if recorded_game(r, kind)]
        if orig:
            print('%-16s after the title s %s | %d of %d runs played the recorded game | %s' % (kind, med([after_title(r, True) for r in good]), len(good), len(rs), others))
        else:
            print('%-16s after the title s %s | less the poll %s (the poll %s) | updates %s, draws %s | %d runs | %s' % (
                kind, med([after_title(r, False) for r in good]), med([after_title(r, False) - r.get('poll_s', 0.0) for r in good if after_title(r, False) is not None]),
                med([r.get('poll_s') for r in good], 3), sorted({r.get('updates') for r in good} - {None}), sorted({r.get('draws') for r in good} - {None}),
                len(good), others))
