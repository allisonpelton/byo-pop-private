# lagscan.py <tour> <orig shots dir> <out dir> [--window W] [--names s46450,s46600,...] [--prepare | --compare]
#
# Which update does the original's shot show? A shot labelled "at update N" of the original (docs/ORIGRUN.md, "Which frame the
# shot shows") can show a frame older than N, and this build of the framework has no flag that says when the frame of N is
# drawn. So the port takes a shot at every update from N-W to N of each chosen shot of the tour, and each of the original's
# shots is compared with those frames: the update whose frame matches it (the fewest pixels that differ by more than the tolerance)
# is the update the original's shot really shows, and the count at that update says whether the picture is the same.
#
#   --prepare   writes <out>/lag.txt, the port's -script for the tour with a shot per update in the windows (run it with
#               tools/identical/port.sh, then --compare)
#   --compare   reads <out>/port/f<update>.png and prints one line per chosen shot: its label, the best update, its differing pixels,
#               and the next best update with its count
import argparse, os, re, shutil, subprocess, sys
import numpy as np
from PIL import Image
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..', 'tools', 'shotdiff'))
import shotdiff

ap = argparse.ArgumentParser()
ap.add_argument('tour')
ap.add_argument('orig')
ap.add_argument('out')
ap.add_argument('--window', type=int, default=160)
ap.add_argument('--names', default='')
ap.add_argument('--tol', type=int, default=8)
ap.add_argument('--mask', action='append', default=[], help='x,y,w,h of a region left out of the count (repeatable)')
ap.add_argument('--prepare', action='store_true')
ap.add_argument('--compare', action='store_true')
ap.add_argument('--pick', default='', help='copy the best frame of each shot to <dir>/<name>.png (for shotdiff)')
ap.add_argument('--ahead', type=int, default=3, help='frames after the label to look at as well')
args = ap.parse_args()

names = [n for n in args.names.split(',') if n]
if not names:
    names = sorted(os.path.splitext(f)[0] for f in os.listdir(args.orig) if f.endswith('.png'))
labels = [int(n[1:]) for n in names]
frames = sorted(set(k for n in labels for k in range(max(1, n - args.window), n + args.ahead + 1)))

if args.prepare:
    os.makedirs(args.out, exist_ok=True)
    lines = []
    for l in open(args.tour):
        l = l.split('#')[0].strip()
        if l:
            k, cmd, *rest = l.split(None, 2)
            if cmd != 'screenshot':
                lines.append((int(k), cmd, ' '.join(rest)))
    last = max(frames)
    lines = [(k, c, a) for k, c, a in lines if k <= last + 2 or c == 'quit']
    for k in frames:
        lines.append((k, 'screenshot', 'f%d' % k))
    lines.append((last + 3, 'quit', ''))
    lines.sort(key=lambda t: t[0])
    tour = os.path.join(args.out, 'lag-tour.txt')
    with open(tour, 'w') as f:
        for k, c, a in lines:
            f.write(('%d %s %s\n' % (k, c, a)).rstrip() + '\n')
    here = os.path.dirname(os.path.abspath(__file__))
    script = subprocess.run([sys.executable, os.path.join(here, 'make.py'), tour, args.out, 'port'], capture_output=True, text=True)
    with open(os.path.join(args.out, 'lag.txt'), 'w') as f:
        f.write(script.stdout)
    print('%d shots in %d windows -> %s/lag.txt' % (len(frames), len(labels), args.out))
    sys.exit(0)


masks = [tuple(int(v) for v in m.split(',')) for m in args.mask]
cfg = {}
cache = {}


def frame(k):
    if k not in cache:
        p = os.path.join(args.out, 'port', 'f%d.png' % k)
        cache[k] = Image.open(p) if os.path.exists(p) else None
    return cache[k]


for name, n in zip(names, labels):
    oi = Image.open(os.path.join(args.orig, name + '.png'))
    res = []
    shift = None
    ign = None
    for k in range(max(1, n - args.window), n + args.ahead + 1):
        pi = frame(k)
        if pi is None:
            continue
        a, b, _, _ = shotdiff.client_areas(oi, pi, cfg)
        if a.shape != b.shape:
            continue
        if ign is None:
            ign = shotdiff.ignore_mask(a.shape[:2], masks)
        if shift is None:
            # one alignment for the whole scan: the best of the shifts at the label's own frame
            shift = shotdiff.search_shift(a, b, ign, args.tol, 8)
        res.append((shotdiff.diff_count(a, b, args.tol, shift, ign), k))
    res.sort()
    if not res:
        print('%-8s no frames' % name)
        continue
    best = res[0]
    if args.pick:
        os.makedirs(args.pick, exist_ok=True)
        shutil.copyfile(os.path.join(args.out, 'port', 'f%d.png' % best[1]), os.path.join(args.pick, name + '.png'))
    second = next((r for r in res[1:] if abs(r[1] - best[1]) > 1), None)
    print('%-8s shift %s best update %d (%+d), %d pixels differ; next %s' % (name, tuple(shift), best[1], best[1] - n, best[0], 'update %d, %d pixels' % (second[1], second[0]) if second else '-'))
