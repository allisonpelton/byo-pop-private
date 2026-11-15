"""lag3d.py <tour> prepare|compare [--lo N] [--hi N] [--names a,b,...] [--shift N]: which update does the original's 3D shot show?

The original's 3D run (visrun3d.py) is on the wall clock. The game is held to read its counter C and released for the picture, and until the
picture is drawn it runs on: the shot shows the state of an update after C (docs/IDENTICAL_PLAY.md, "3D screens"). So the port takes a shot at every update C+lo ..
C+hi of each chosen shot (default -1 .. +12), and each of the original's shots is compared with those frames; the frame with the fewest pixels that
differ by more than 8 is the one the original's shot shows, whose offset says how far the game ran on. A frame is also tried moved sideways by up to
SHAKE columns (the board's shake, which moves the whole picture by a random amount, docs/IDENTICAL_PLAY.md): the shift that fits is printed as dx and the
frame that is copied is moved by it.
  prepare  writes build/ph2/<tour>/lag3d/port3d.txt, the port's script of the run (mkport3d.py's lines, the shots of the window added; --shift N
           lets every input of the run reach the port N updates later, to test whether the original read its inputs late)
  compare  reads <tour>/lag3d/port/<name>_<offset>.png, prints one line a shot (its best offset and differing pixels, and the next best),
           copies each best frame (moved by its dx) to <tour>/lag3d/best/<name>.png and writes <tour>/lag3d/best.tsv (name, C, offset, pixels,
           dx); shotdiff then compares best/ with orig3d/."""
import os
import shutil
import sys

import numpy as np
from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
a = sys.argv[1:]
lo, hi, only, shift = -1, 12, None, 0
for opt in ("--lo", "--hi", "--names", "--shift"):
    if opt in a:
        i = a.index(opt)
        if opt == "--names":
            only = a[i + 1].split(",")
        elif opt == "--lo":
            lo = int(a[i + 1])
        elif opt == "--shift":
            shift = int(a[i + 1])
        else:
            hi = int(a[i + 1])
        del a[i:i + 2]
tour, mode = a[0], a[1]
D = os.path.join(ROOT, "build", "ph2", tour)
L = os.path.join(D, os.environ.get("LAG_DIR", "lag3d"))
landed = {}
for line in open(os.path.join(D, "orig3d", "landed.tsv")):
    name, c, _ = line.rstrip("\n").split("\t")
    landed[name] = int(c)
names = only or sorted(landed, key=lambda n: landed[n])


def port_tour(name):
    """The port's own tour file when the tour has one (o2o -> o2p.txt): the same steps as the original's tour with the coordinates of the
    port's layout, for a dialog that has one row less (the removed More Games row). Returns the commands (not the shots) in order, or None."""
    if not name.endswith("o"):
        return None
    path = os.path.join(ROOT, "games", "rocketmania", "tools", "shots", name[:-1] + "p.txt")
    if not os.path.exists(path):
        return None
    out = []
    for line in open(path):
        w = line.split(None, 2)
        if len(w) >= 2 and w[1] not in ("screenshot", "quit"):
            out.append((w[1], w[2].strip() if len(w) > 2 else ""))
    return out



def regions(mask, cell=16, least=150):
    """Bounding boxes (x0, y0, x1, y1) of the groups of touching 16x16 cells that hold differing pixels, with at least `least` differing pixels."""
    h, w = mask.shape
    gh, gw = (h + cell - 1) // cell, (w + cell - 1) // cell
    cnt = np.zeros((gh, gw), int)
    for gy in range(gh):
        for gx in range(gw):
            cnt[gy, gx] = mask[gy * cell:(gy + 1) * cell, gx * cell:(gx + 1) * cell].sum()
    seen = np.zeros((gh, gw), bool)
    out = []
    for gy in range(gh):
        for gx in range(gw):
            if cnt[gy, gx] > 8 and not seen[gy, gx]:
                stack, cells = [(gy, gx)], []
                seen[gy, gx] = True
                while stack:
                    cy, cx = stack.pop()
                    cells.append((cy, cx))
                    for dy in (-1, 0, 1):
                        for dx in (-1, 0, 1):
                            ny, nx = cy + dy, cx + dx
                            if 0 <= ny < gh and 0 <= nx < gw and cnt[ny, nx] > 8 and not seen[ny, nx]:
                                seen[ny, nx] = True
                                stack.append((ny, nx))
                if sum(cnt[c] for c in cells) >= least:
                    out.append((min(c[1] for c in cells) * cell, min(c[0] for c in cells) * cell,
                                min(w, (max(c[1] for c in cells) + 1) * cell), min(h, (max(c[0] for c in cells) + 1) * cell)))
    return out


def box_blur(a, k=7):
    """The mean over a k x k box (edges padded by repeating), of an H x W x 3 array."""
    r = k // 2
    pad = np.pad(a.astype(float), ((r, r), (r, r), (0, 0)), mode="edge")
    c = np.cumsum(np.cumsum(pad, axis=0), axis=1)
    c = np.pad(c, ((1, 0), (1, 0), (0, 0)))
    h, w = a.shape[:2]
    return (c[k:k + h, k:k + w] - c[:h, k:k + w] - c[k:k + h, :w] + c[:h, :w]) / (k * k)


def blurred_diff(a, b, tol=24):
    return int((np.abs(box_blur(a) - box_blur(b)).max(axis=2) > tol).sum())


SHAKE = 3   # columns: the board's shake moves a frame sideways by up to this much (the shock's size)


def shift_x(p, ref, dx):
    """p moved dx columns to the left (the picture at x + dx comes to x); the columns it does not cover are taken from ref, so they never differ."""
    if dx == 0:
        return p
    q = ref.copy()
    if dx > 0:
        q[:, :-dx] = p[:, dx:]
    else:
        q[:, -dx:] = p[:, :dx]
    return q


def frame(name, o):
    return os.path.join(L, "port", "%s_%+d.png" % (name, o))


if mode == "prepare":
    shutil.rmtree(os.path.join(L, "port"), ignore_errors=True)
    os.makedirs(os.path.join(L, "port"))
    rows = []   # (update, order, text)
    alt = port_tour(tour)
    ai = 0
    for i, line in enumerate(open(os.path.join(D, "vis3d.steps.tsv"))):
        n, c, cmd, rest, _ = line.rstrip("\n").split("\t")
        if cmd != "shot" and alt is not None:
            assert alt[ai][0] == cmd, "the port's tour has another command at step %d" % ai
            rest = alt[ai][1]
            ai += 1
        if cmd != "shot":
            rows.append((int(c) + 1 + shift, i, "%s %s" % ("char" if cmd == "text" else cmd, rest)))
    for j, nm in enumerate(names):
        for o in range(lo, hi + 1):
            rows.append((landed[nm] + 1 + o, 1000000 + j * 1000 + o - lo, "screenshot %s" % frame(nm, o).replace(os.sep, "/")))
    rows.sort()
    text = ["%d %s" % (k, t) for k, _, t in rows]
    text.append("%d quit" % (rows[-1][0] + 2))
    open(os.path.join(L, "port3d.txt"), "w").write("\n".join(text) + "\n")
    print("%d lines -> %s" % (len(text), os.path.join(L, "port3d.txt")))
else:
    best_dir = os.path.join(L, "best")
    shutil.rmtree(best_dir, ignore_errors=True)
    os.makedirs(best_dir)
    tsv = open(os.path.join(L, "best.tsv"), "w")
    for nm in names:
        o_im = np.asarray(Image.open(os.path.join(D, "orig3d", nm + ".png")).convert("RGB")).astype(int)
        # the board's shake (CrtRand, a few pixels at most, docs/IDENTICAL_PLAY.md) moves the whole picture sideways by a random amount, so each frame is
        # also tried moved up to SHAKE columns either way; a shift costs 200 pixels, so that a frame that matches without one keeps its place
        res = []   # (cost, pixels that differ, offset, shift)
        for o in range(lo, hi + 1):
            if os.path.exists(frame(nm, o)):
                p = np.asarray(Image.open(frame(nm, o)).convert("RGB")).astype(int)
                for dx in range(-SHAKE, SHAKE + 1):
                    n = int((np.abs(shift_x(p, o_im, dx) - o_im).max(axis=2) > 8).sum())
                    res.append((n + 200 * abs(dx), n, o, dx))
        res.sort()
        if not res:
            print(nm, "no frames")
            continue
        best_o, best_dx = res[0][2], res[0][3]
        p = np.asarray(Image.open(frame(nm, best_o)).convert("RGB")).astype(int)
        b_im = shift_x(p, o_im, best_dx)
        Image.fromarray(b_im.astype(np.uint8)).save(os.path.join(best_dir, nm + ".png"))
        tsv.write("%s\t%d\t%d\t%d\t%d\n" % (nm, landed[nm], best_o, res[0][1], best_dx))
        other = [(r[1], r[2]) for r in res if r[2] != best_o][:1]
        at0 = [r[1] for r in res if r[2] == 0 and r[3] == 0]
        print("%s C %d best +%d (%d px) dx %d next %s at 0: %d px" % (nm, landed[nm], best_o, res[0][1], best_dx, other, at0[0] if at0 else -1))
        # a structural check: both pictures blurred by a 7x7 box, then the pixels that still differ by more than 24: sub-pixel and filtering
        # noise averages out, a sprite that is missing, moved by a few pixels or another picture does not
        sd = blurred_diff(b_im, o_im)
        print("    structure: %d px differ after a 7x7 blur" % sd)
        # each region that still differs at the best offset: which offset suits that region best, and how many pixels still differ there
        for (x0, y0, x1, y1) in regions(np.abs(b_im - o_im).max(axis=2) > 8):
            per = []
            for o in range(lo, hi + 1):
                if os.path.exists(frame(nm, o)):
                    q = shift_x(np.asarray(Image.open(frame(nm, o)).convert("RGB")).astype(int), o_im, best_dx)
                    per.append((int((np.abs(q[y0:y1, x0:x1] - o_im[y0:y1, x0:x1]).max(axis=2) > 8).sum()), o))
            per.sort()
            at_best = [c for c, o in per if o == best_o]
            print("    region (%d,%d %dx%d): best +%d (%d px), at +%d: %d px" % (x0, y0, x1 - x0, y1 - y0, per[0][1], per[0][0], best_o, at_best[0] if at_best else -1))
