"""mkport3d.py <tour> [--offset N] [--out DIR] [--suf 3d|2d]: the port's script for a 3D run of the original (visrun3d.py): build/ph2/<tour>/vis<suf>.steps.tsv
holds each step with the update C it landed at (the input was read by update C+1, a shot shows the state after update C), and the port takes
a script line K before update K, so every line is K = C + 1 + offset. The shots go to <out>/port/<name>.png (default build/ph2/<tour>/3d).
Writes <out>/port3d.txt. A tour named o2o takes the coordinates of its clicks from o2p.txt for the port (the port's Options dialog has one row less)."""
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


def port_tour(tour):
    """The port's own tour file when the tour has one (o2o -> o2p.txt): the same steps as the original's tour with the coordinates of the
    port's layout, for a dialog that has one row less (the removed More Games row). Returns the commands (not the shots) in order, or None."""
    if not tour.endswith("o"):
        return None
    path = os.path.join(ROOT, "games", "rocketmania", "tools", "shots", tour[:-1] + "p.txt")
    if not os.path.exists(path):
        return None
    out = []
    for line in open(path):
        w = line.split(None, 2)
        if len(w) >= 2 and w[1] not in ("screenshot", "quit"):
            out.append((w[1], w[2].strip() if len(w) > 2 else ""))
    return out


tour_dir = os.path.join(ROOT, "build", "ph2", name)
out = out or os.path.join(tour_dir, "3d")
os.makedirs(os.path.join(out, "port"), exist_ok=True)
lines = []
alt = port_tour(name)
ai = 0
for l in open(os.path.join(tour_dir, "vis%s.steps.tsv" % suf)):
    n, c, cmd, rest, _ = l.rstrip("\n").split("\t")
    k = int(c) + 1 + offset
    if cmd != "shot" and alt is not None:
        assert alt[ai][0] == cmd, "the port's tour has another command at step %d" % ai
        rest = alt[ai][1]
        ai += 1
    if cmd == "shot":
        lines.append("%d screenshot %s" % (k, os.path.join(out, "port", rest + ".png").replace("\\", "/")))
    else:
        lines.append("%d %s %s" % (k, "char" if cmd == "text" else cmd, rest))
assert alt is None or ai == len(alt), "the port's tour has more steps than the original's run"
lines.append("%d quit" % (int(lines[-1].split()[0]) + 2))
open(os.path.join(out, "port3d.txt"), "w").write("\n".join(lines) + "\n")
print("%d lines -> %s" % (len(lines), os.path.join(out, "port3d.txt")))
