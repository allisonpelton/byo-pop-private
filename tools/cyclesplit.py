"""The cycle split of a run, or of a port and its original side by side (docs/TEST_COMMANDS.md, "The cycle split"; docs/ORIGRUN.md,
"Counting cycles inside the original's functions").

    python -I tools/cyclesplit.py <file> [<file of the original>] [--from LABEL] [--to LABEL] [--hz N] [--split]

A file is what the port's -cyclesplit=<file> writes or origrun's --count writes (build/origrun/<run>/counts.jsonl): one JSON line a
snapshot, taken by the script's `cyclesplit <label>` / `at N count <label>` lines and once more at exit (label "exit"). The stretch is
the difference of two snapshots, --from (default: the first) and --to (default: the last); with two files both are cut at the same
labels. A share of a core is the cycles over --hz (the cycle counter's rate, measured by a spin of a quarter second when not given)
and the stretch's wall time.

The parts, all in cycles of the threads that ran (never time spent waiting):
  update    the game's update function, on the loop's thread
  draw      the game's draw function less its present: the game's own drawing
  present   the present call: the upload and the swap in the port, the Blt or Flip in the original
  driver    threads started in a graphics driver or runtime module (the GPU driver's own thread, Direct3D, DXGI)
  audio     threads of the mixer: SDL's audio thread, BASS, FMOD, DirectSound
  loop      the loop thread's remainder: events, waking from Sleep, snapshots, the cycle readings
  other     every other thread, and the threads that ended before the last snapshot (the process total less the rest)
The parts add up to the process's cycles by construction; "unaccounted" is loop + other.

THE LIMIT, which holds wherever a split is shown: a GPU renderer batches its draw calls until the present, and DirectDraw can blit
asynchronously, so a side that draws through one has its drawing partly in `present` and in `driver`. Such a side is reported as
"picture" (draw + present + driver) and not split; --split shows the three apart, which is only meaningful for a side that draws in
software (the 2D software renderer, the original's own blitters).
"""
import argparse
import json
import re
import sys
import time

AUDIO = re.compile(r'sdlaudio|bass\.dll|fmod|dsound|xaudio|wasapi|audiodg|openal', re.I)
DRIVER = re.compile(r'nvwgf2um|nvldumd|nvd3dum|nvoglv|igd\d*|ig\d+icd|igdumd|atidxx|aticfx|amdxx|amdvlk|d3d9\.|d3d11|d3d12|dxgi|ddraw|opengl32|vulkan|gdi32|win32u', re.I)


def read(path):
    snaps = []
    for line in open(path):
        line = line.strip()
        if line.startswith('{'):
            snaps.append(json.loads(line))
    if not snaps:
        sys.exit('%s: no snapshots' % path)
    return snaps


def pick(snaps, label, default):
    if label is None:
        return snaps[default]
    for s in snaps:
        if s['label'] == label:
            return s
    sys.exit('no snapshot labelled %r (labels: %s)' % (label, ', '.join(s['label'] for s in snaps)))


def tsc_hz():
    """The rate the cycle counters tick at: a thread's cycles over the wall time of a quarter second's spin."""
    import ctypes
    import ctypes.wintypes as W
    k32 = ctypes.WinDLL('kernel32')
    k32.GetCurrentThread.restype = ctypes.c_void_p
    k32.QueryThreadCycleTime.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint64)]
    h = ctypes.c_void_p(k32.GetCurrentThread())
    c0, c1 = ctypes.c_uint64(), ctypes.c_uint64()
    k32.QueryThreadCycleTime(h, ctypes.byref(c0))
    t0 = time.perf_counter()
    while time.perf_counter() - t0 < 0.25:
        pass
    k32.QueryThreadCycleTime(h, ctypes.byref(c1))
    return (c1.value - c0.value) / (time.perf_counter() - t0)


def split(a, b):
    """The stretch between snapshots a and b: wall seconds, updates and the parts in cycles."""
    d = {'wall_s': b['wall_s'] - a['wall_s'], 'updates': b['update'] - a['update']}
    d['process'] = b['process_cycles'] - a['process_cycles']
    d['main'] = b['main_thread_cycles'] - a['main_thread_cycles']
    for p in ('update', 'draw', 'present'):
        d[p] = b['parts'][p]['cycles'] - a['parts'][p]['cycles']
        d[p + '_calls'] = b['parts'][p]['calls'] - a['parts'][p]['calls']
    d['counter_calls'] = b['counter']['calls'] - a['counter']['calls']
    d['counter_each'] = b['counter']['cycles_each']
    before = {t['tid']: t['cycles'] for t in a['threads']}
    d['audio'] = d['driver'] = d['other_live'] = 0
    d['threads'] = []
    for t in b['threads']:
        c = t['cycles'] - before.get(t['tid'], 0)
        label = (t['name'] + ' ' + t['start']).strip()
        if AUDIO.search(label):
            d['audio'] += c
            kind = 'audio'
        elif DRIVER.search(label):
            d['driver'] += c
            kind = 'driver'
        else:
            d['other_live'] += c
            kind = 'other'
        d['threads'].append((c, kind, label))
    d['loop'] = d['main'] - d['update'] - d['draw'] - d['present']
    d['other'] = d['process'] - d['main'] - d['audio'] - d['driver']      # the live other threads and the ended ones
    d['misses'] = b['misses'] - a['misses']
    d['misses_total'] = b['misses']
    return d


def rows(d, split_picture):
    if split_picture:
        r = [('update', d['update']), ('draw', d['draw']), ('present', d['present']), ('driver threads', d['driver'])]
    else:
        r = [('update', d['update']), ('picture (draw, present, driver threads)', d['draw'] + d['present'] + d['driver'])]
    r += [('audio', d['audio']), ('loop (events, sleeps, readings)', d['loop']), ('other threads', d['other'])]
    return r


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('files', nargs='+')
    ap.add_argument('--from', dest='frm')
    ap.add_argument('--to')
    ap.add_argument('--hz', type=float)
    ap.add_argument('--split', action='store_true', help='show draw, present and the driver threads apart')
    ap.add_argument('--json', action='store_true')
    args = ap.parse_args()
    hz = args.hz or tsc_hz()
    results = []
    for path in args.files[:2]:
        s = read(path)
        results.append((path, split(pick(s, args.frm, 0), pick(s, args.to, -1))))
    if args.json:
        print(json.dumps([dict(r, file=p) for p, r in results]))
        return
    for path, d in results:
        n = max(d['updates'], 1)
        print('== %s: %d updates in %.2f s (%.1f a second), the cycle counter taken as %.0f MHz' % (path, d['updates'], d['wall_s'], d['updates'] / d['wall_s'], hz / 1e6))
        print('   %-42s %14s %12s %8s' % ('', 'M cycles/update', 'share of core', 'of total'))
        for name, c in rows(d, args.split) + [('total (process cycles)', d['process'])]:
            print('   %-42s %14.3f %11.2f%% %7.1f%%' % (name, c / n / 1e6, 100.0 * c / (hz * d['wall_s']), 100.0 * c / d['process'] if d['process'] else 0))
        unaccounted = d['loop'] + d['other']
        print('   counted parts %.1f%% of the total, unaccounted (loop + other threads) %.1f%%; %d cycle readings at %d cycles each = %.2f%% of the total; %d misses'
              % (100.0 * (d['update'] + d['draw'] + d['present'] + d['driver'] + d['audio']) / d['process'], 100.0 * unaccounted / d['process'],
                 d['counter_calls'], d['counter_each'], 100.0 * d['counter_calls'] * d['counter_each'] / d['process'], d['misses']))
        for c, kind, label in sorted(d['threads'], reverse=True)[:5]:
            print('   thread %-8s %6.2f%%  %s' % (kind, 100.0 * c / (hz * d['wall_s']), label))
    if len(results) == 2:
        (pa, a), (pb, b) = results
        print('== %s against %s, M cycles an update' % (pa, pb))
        ra, rb = rows(a, args.split), rows(b, args.split)
        # the two sides name their parts alike; the rows pair up by position
        for (name, ca), (_, cb) in zip(ra + [('total', a['process'])], rb + [('total', b['process'])]):
            xa, xb = ca / max(a['updates'], 1) / 1e6, cb / max(b['updates'], 1) / 1e6
            print('   %-42s %10.3f %10.3f   %s' % (name, xa, xb, ('%.2f' % (xa / xb)) if xb else '-'))
    if not args.split:
        print('Where a side draws through a GPU renderer or DirectDraw its drawing is spread over draw, present and the driver\'s threads (the draw '
              'calls are batched until the present, DirectDraw can blit asynchronously), so only "picture" is meaningful; --split is for a software drawer.')


main()
