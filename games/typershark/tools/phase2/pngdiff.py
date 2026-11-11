#!/usr/bin/env python3
"""Compares the shots of two runs of the port, pixel for pixel: python pngdiff.py <dir A> <dir B>
Prints one summary line (the shots in both, how many differ) and one line for each shot that differs: its name, the pixels that
differ, the largest difference in a channel and the box they lie in."""
import os
import sys
import numpy as np
from PIL import Image

a, b = sys.argv[1], sys.argv[2]
names = sorted(n for n in os.listdir(a) if n.endswith(".png") and os.path.exists(os.path.join(b, n)))
only = sorted(set(os.listdir(a)) ^ set(os.listdir(b)))
rows = []
for n in names:
    x = np.asarray(Image.open(os.path.join(a, n)).convert("RGB")).astype(int)
    y = np.asarray(Image.open(os.path.join(b, n)).convert("RGB")).astype(int)
    if x.shape != y.shape:
        rows.append("%s: size %s against %s" % (n, x.shape, y.shape))
        continue
    d = np.abs(x - y).max(axis=2)
    ys, xs = np.nonzero(d)
    if len(ys):
        rows.append("%s: %d pixels, max %d, box x %d..%d y %d..%d" % (n, len(ys), d.max(), xs.min(), xs.max(), ys.min(), ys.max()))
print("%s %s: %d shots in both, %d differ%s" % (a, b, len(names), len(rows), (", only in one: %d" % len(only)) if only else ""))
for r in rows:
    print("  " + r)
