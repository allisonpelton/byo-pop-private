"""mkport3d.py <tour> [--offset N] [--out DIR] [--suf 3d|2d]: the port's script for a 3D run of the original (visrun3d.py): build/ph2/<tour>/vis<suf>.steps.tsv
holds each step with the update C it landed at (the input was read by update C+1, a shot shows the state after update C), and the port takes
a script line K before update K, so every line is K = C + 1 + offset. The shots go to <out>/port/<name>.png (default build/ph2/<tour>/3d).
Writes <out>/port3d.txt."""
import os
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
a = sys.argv[1:]
offset, out, suf = 0, None, "3d"
for opt in ("--offset", "--out", "--suf"):
    if opt in a:
        i = a.index(opt)
        if opt == "--offset":
            offset = int(a[i + 1])
        elif opt == "--suf":
            suf = a[i + 1]
        else:
            out = a[i + 1]
        del a[i:i + 2]
name = a[0]
tour_dir = os.path.join(ROOT, "build", "ph2", name)
out = out or os.path.join(tour_dir, "3d")
os.makedirs(os.path.join(out, "port"), exist_ok=True)
lines = []
for l in open(os.path.join(tour_dir, "vis%s.steps.tsv" % suf)):
    n, c, cmd, rest, _ = l.rstrip("\n").split("\t")
    k = int(c) + 1 + offset
    if cmd == "shot":
        lines.append("%d screenshot %s" % (k, os.path.join(out, "port", rest + ".png").replace("\\", "/")))
    else:
        lines.append("%d %s %s" % (k, "char" if cmd == "text" else cmd, rest))
lines.append("%d quit" % (int(lines[-1].split()[0]) + 2))
open(os.path.join(out, "port3d.txt"), "w").write("\n".join(lines) + "\n")
print("%d lines -> %s" % (len(lines), os.path.join(out, "port3d.txt")))
