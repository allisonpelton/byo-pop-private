"""Matches the original's shots of the 3D screen wipe with the port's shots of every update (GAME_NOTES section 10):

    python -I wipematch.py <dir> [--mask x0,y0,x1,y1] [--window 12] [--pairs <out dir>]

<dir> holds orig/ (the original's shots w000.png ... and steps.tsv, from `wipeshots.py orig`) and port-all/ (the port's shots <update>.png,
one for every update of the wipe: wipeshots.py port <dir> ... then a script with a shot a line, as run.sh makes it). An original's picture
shows the frame that was on the screen when the game was held at update C, which on the real clock is mostly the frame of C or the one
before it, and now and then one of eight or nine updates before (the draw counter says the update was drawn, not that the compositor has
shown it), so each is compared with the port's pictures of the updates C-<window> to C+<window> and takes the one that differs least; the wipe is monotonic, so that is the frame of the same state. What
counts is how small the difference is at the best match and that the offset does not drift over the wipe (a wipe that ran at another speed
would). The window's bottom corners are left out (Windows 11 rounds them). The mask is the right-hand column of the main menu, where the port has one button less than the original (its "More Games" button is
taken out, GAME_NOTES section 3), live behind the wipe. A pixel differs when a channel differs by more than 8 (shotdiff's tolerance).
Prints one line a frame and a summary line. --pairs copies each original picture and its best match, under the same name, to <out>/orig and
<out>/port, for tools/shotdiff (docs/SHOTDIFF.md) to judge the boxes that differ.
"""
import os
import shutil
import sys

import numpy as np
from PIL import Image


def load(path):
    return np.asarray(Image.open(path).convert("RGB"), dtype=np.int16)


def main():
    a = sys.argv[1:]
    mask, window = (430, 100, 640, 460), 12
    if "--mask" in a:
        i = a.index("--mask")
        mask = tuple(int(v) for v in a[i + 1].split(","))
        del a[i:i + 2]
    if "--window" in a:
        i = a.index("--window")
        window = int(a[i + 1])
        del a[i:i + 2]
    pairs = None
    if "--pairs" in a:
        i = a.index("--pairs")
        pairs = a[i + 1]
        del a[i:i + 2]
        os.makedirs(os.path.join(pairs, "orig"), exist_ok=True)
        os.makedirs(os.path.join(pairs, "port"), exist_ok=True)
    d = a[0]
    port = {}
    for f in os.listdir(os.path.join(d, "port-all")):
        if f.endswith(".png"):
            port[int(f[:-4])] = load(os.path.join(d, "port-all", f))
    rows = []
    print("shot   landed best-port-update lag  px>8  px>24  maxdelta")
    for line in open(os.path.join(d, "orig", "steps.tsv")):
        n, c, cmd, name = line.rstrip("\n").split("\t")[:4]
        if cmd != "shot" or not name.startswith("w"):
            continue
        c = int(c)
        o = load(os.path.join(d, "orig", name + ".png"))
        best = None
        for k in range(c - window, c + window + 1):
            if k not in port:
                continue
            diff = np.abs(port[k] - o).max(axis=2)
            diff[mask[1]:mask[3], mask[0]:mask[2]] = 0
            diff[-8:, :8] = 0          # the bottom corners of the window (Windows 11 rounds them: the screen behind shows through)
            diff[-8:, -8:] = 0
            r = (int((diff > 8).sum()), k, int((diff > 24).sum()), int(diff.max()))
            if best is None or r < best:
                best = r
        if best is None:
            continue
        rows.append((name, c, best[1], best[1] - c, best[0], best[2], best[3]))
        if pairs:
            shutil.copyfile(os.path.join(d, "orig", name + ".png"), os.path.join(pairs, "orig", name + ".png"))
            shutil.copyfile(os.path.join(d, "port-all", "%d.png" % best[1]), os.path.join(pairs, "port", name + ".png"))
        print("%-6s %5d  %5d  %+3d  %6d  %5d  %4d" % rows[-1])
    if rows:
        lags = [r[3] for r in rows]
        late = ["%s (%+d)" % (r[0], r[3]) for r in rows if r[3] < -1 and r[0] != "w000"]
        print("offset 0 or -1 in %d of %d frames; older (the screen showed an earlier frame): %s" % (
            sum(1 for r in rows if r[3] in (0, -1)), len(rows), ", ".join(late) or "none"))
        print("summary: %d frames, lag %+d to %+d, px>8 at the best match: median %d, max %d; px>24: max %d; max delta: %d" % (
            len(rows), min(lags), max(lags), sorted(r[4] for r in rows)[len(rows) // 2], max(r[4] for r in rows),
            max(r[5] for r in rows), max(r[6] for r in rows)))


if __name__ == "__main__":
    main()
