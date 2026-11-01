#!/usr/bin/env python3
"""Compare a screenshot of the original Bonnie's Bookstore (tools/origrun, window frame included) with the port's.

origrun's captures of the original's SDL 1.2 window come out resampled by a constant sub-pixel offset: each pixel is the
bilinear mix of the game's pixel with its left and upper neighbours (0.2 of a pixel across, 0.4 down; fitted on the
PopCap logo, where the port's frame then matches to within 2 levels). This applies the same mix to the port's frame,
cuts the original's frame away and reports the pixels still differing by more than the tolerance, with a mask.

    python games/bonnies/tools/origcmp.py <original.png> <port.png> [--tol 6] [--mask out.png]
"""
import argparse
import sys

import numpy as np
from PIL import Image

FX, FY = 0.2, 0.4


def load(path):
    return np.asarray(Image.open(path).convert('RGB')).astype(float)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('orig')
    ap.add_argument('port')
    ap.add_argument('--tol', type=float, default=6)
    ap.add_argument('--mask')
    args = ap.parse_args()
    o = load(args.orig)
    p = load(args.port)
    if p.shape[:2] != (600, 800):
        sys.exit('the port frame is not 800x600')
    if o.shape[:2] != (600, 800):
        # origrun's window: a 3-pixel border and the caption above the 800x600 client area
        bx = (o.shape[1] - 800) // 2
        o = o[o.shape[0] - 600 - bx:o.shape[0] - bx, bx:bx + 800]
    up = np.roll(p, 1, 0)
    q = (1 - FY) * ((1 - FX) * p + FX * np.roll(p, 1, 1)) + FY * ((1 - FX) * up + FX * np.roll(up, 1, 1))
    d = np.abs(q - o).max(axis=2)
    d[0, :] = 0
    d[:, 0] = 0
    bad = d > args.tol
    print(f'{bad.mean() * 100:.3f}% of pixels differ by more than {args.tol:g} (max {d.max():.0f})')
    if bad.any():
        ys, xs = np.nonzero(bad)
        print(f'box x {xs.min()}-{xs.max()} y {ys.min()}-{ys.max()}')
    if args.mask:
        Image.fromarray((bad * 255).astype('uint8')).save(args.mask)


if __name__ == '__main__':
    main()
