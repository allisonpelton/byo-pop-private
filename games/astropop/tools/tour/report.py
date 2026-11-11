"""One line per pair of a tour: shotdiff's verdict, its visible boxes, and the raw differences it hides as faint.

    python games/astropop/tools/tour/report.py <name> [--min N]

Reads build/shotreview/astropop/<name>/summary.json (shotdiff's) and the two folders of pictures of build/logs/ap/tour/<name>/.
A faint difference is below what shotdiff shows a reviewer (a 3x3 blur and 32 levels), so this also counts the raw pixels that
differ by more than 8 in some channel and says where they are (the box that holds them) when there are at least --min of them
(default 200): a brightness difference of 20 levels over a whole strip is faint to shotdiff and still a difference. The original's capture is
first lifted by 2 levels, the offset every origrun capture has.
"""
import argparse
import json
import os

import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))


def load(path, crop):
    im = Image.open(path).convert('RGB')
    if crop:
        im = im.crop((3, 26, 643, 506))
    return np.asarray(im).astype(int)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('name')
    ap.add_argument('--min', type=int, default=200)
    a = ap.parse_args()
    d = os.path.join(ROOT, 'build', 'logs', 'ap', 'tour', a.name)
    s = json.load(open(os.path.join(ROOT, 'build', 'shotreview', 'astropop', a.name, 'summary.json')))
    for pr in sorted(s['pairs'], key=lambda p: p['name']):
        n = pr['name']
        o = os.path.join(d, 'orig', n)
        p = os.path.join(d, 'port', n)
        raw = ''
        if os.path.exists(o) and os.path.exists(p):
            x, y = load(o, True), load(p, False)
            if x.shape == y.shape:
                # origrun's captures are 2 levels of 255 darker than the port's picture on every channel, black staying 0
                # (Typer Shark's notes found it first): lift the original's by 2 before counting
                x = np.where(x == 0, np.minimum(y, 2), x + 2)
                df = abs(x - y).max(axis=2)
                m = df > 8
                k = int(m.sum())
                if k >= a.min:
                    ys, xs = np.nonzero(m)
                    raw = 'raw %d px in (%d,%d)-(%d,%d), max %d' % (k, xs.min(), ys.min(), xs.max(), ys.max(), df.max())
        vis = ['(%d,%d %dx%d %dpx)' % (b['x'], b['y'], b['w'], b['h'], b['pixels']) for b in pr.get('boxes', [])]
        print('%-34s %-7s %s %s' % (n, pr['status'], ' '.join(vis), raw))
    print('only original:', s.get('orig_only'), 'only port:', s.get('port_only'))


if __name__ == '__main__':
    main()
