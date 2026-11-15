"""One line a pair of a shotdiff run (build/shotreview/<game>/<run>/summary.json): its status, how much differs and where.

    python games/bejeweled2/tools/origcmp/shotsum.py <run> [--all]

Prints the pairs that differ (or all of them with --all) as: name, status, visible pixels, then each visible box as x,y wxh (pixels).
The menu's two buttons the port leaves out by decision (the "?" of the secret modes: x 430 to 600, y 530 to 700; More Games: x 600 to
790, y 660 to 760, in the 3D game's 1024 by 768) are marked with `*` on a box that lies wholly inside them.
"""
import json
import sys

KNOWN = [(430, 530, 600, 700), (600, 660, 790, 760), (372, 531, 780, 753)]


def known(b):
    x0, y0, x1, y1 = b['x'], b['y'], b['x'] + b['w'], b['y'] + b['h']
    return any(x0 >= k[0] and y0 >= k[1] and x1 <= k[2] and y1 <= k[3] for k in KNOWN)


def main():
    run = sys.argv[1]
    every = '--all' in sys.argv
    d = json.load(open('build/shotreview/bejeweled2/%s/summary.json' % run))
    n = {'match': 0, 'faint': 0, 'differ': 0, 'error': 0}
    for p in sorted(d['pairs'], key=lambda p: p['name']):
        n[p['status']] = n.get(p['status'], 0) + 1
        if p['status'] == 'match' or (p['status'] == 'faint' and not every):
            continue
        boxes = ' '.join('%s%d,%d %dx%d(%d)' % ('*' if known(b) else '', b['x'], b['y'], b['w'], b['h'], b['visible_pixels'])
                         for b in p.get('boxes', []))
        print('%-18s %-6s %6.2f%% %6d visible  %s' % (p['name'], p['status'], p.get('diff_pct', 0), p.get('visible_pixels', 0), boxes))
    print('pairs: %s; only in the original: %d, only in the port: %d' % (
        ', '.join('%d %s' % (v, k) for k, v in n.items() if v), len(d.get('orig_only', [])), len(d.get('port_only', []))))


main()
