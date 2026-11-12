"""lag3d.py <tour> prepare|compare [--lo N] [--hi N] [--names a,b,...]: which update does the original's 3D shot show?

The original's 3D run (visrun3d.py) is on the wall clock. The game is held to read its counter C and released for the picture, and until the
picture is drawn it runs on: the shot shows the state of an update after C (docs/IDENTICAL_PLAY.md, "3D screens"). So the port takes a shot at every update C+lo ..
C+hi of each chosen shot (default -1 .. +12), and each of the original's shots is compared with those frames; the frame with the fewest pixels that
differ by more than 8 is the one the original's shot shows, whose offset says how far the game ran on.
  prepare  writes build/ph2/<tour>/lag3d/port3d.txt, the port's script of the run (mkport3d.py's lines, the shots of the window added)
  compare  reads <tour>/lag3d/port/<name>_<offset>.png, prints one line a shot (its best offset and differing pixels, and the next best),
           copies each best frame to <tour>/lag3d/best/<name>.png and writes <tour>/lag3d/best.tsv; shotdiff then compares best/ with orig3d/."""
import os
import shutil
import sys

import numpy as np
from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
a = sys.argv[1:]
lo, hi, only = -1, 12, None
for opt in ("--lo", "--hi", "--names"):
    if opt in a:
        i = a.index(opt)
        if opt == "--names":
            only = a[i + 1].split(",")
        elif opt == "--lo":
            lo = int(a[i + 1])
        else:
            hi = int(a[i + 1])
        del a[i:i + 2]
tour, mode = a[0], a[1]
D = os.path.join(ROOT, "build", "ph2", tour)
L = os.path.join(D, "lag3d")
landed = {}
for line in open(os.path.join(D, "orig3d", "landed.tsv")):
    name, c, _ = line.rstrip("\n").split("\t")
    landed[name] = int(c)
names = only or sorted(landed, key=lambda n: landed[n])


def frame(name, o):
    return os.path.join(L, "port", "%s_%+d.png" % (name, o))


if mode == "prepare":
    shutil.rmtree(os.path.join(L, "port"), ignore_errors=True)
    os.makedirs(os.path.join(L, "port"))
    rows = []   # (update, order, text)
    for i, line in enumerate(open(os.path.join(D, "vis3d.steps.tsv"))):
        n, c, cmd, rest, _ = line.rstrip("\n").split("\t")
        if cmd != "shot":
            rows.append((int(c) + 1, i, "%s %s" % ("char" if cmd == "text" else cmd, rest)))
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
        res = []
        for o in range(lo, hi + 1):
            if os.path.exists(frame(nm, o)):
                p = np.asarray(Image.open(frame(nm, o)).convert("RGB")).astype(int)
                res.append((int((np.abs(p - o_im).max(axis=2) > 8).sum()), o))
        res.sort()
        if not res:
            print(nm, "no frames")
            continue
        shutil.copyfile(frame(nm, res[0][1]), os.path.join(best_dir, nm + ".png"))
        tsv.write("%s\t%d\t%d\t%d\n" % (nm, landed[nm], res[0][1], res[0][0]))
        print("%s C %d best +%d (%d px) next %s at 0: %d px" % (nm, landed[nm], res[0][1], res[0][0], res[1:2], [r[0] for r in res if r[1] == 0][0] if any(r[1] == 0 for r in res) else -1))
