"""lag.py <steps name> <shot or clip name> [x0 y0 x1 y1]

Which update of the port each of the original's pictures shows. The port runs the pilot's script (gen.py's, made by
run.sh) up to the picture, taking a screenshot at every update from 30 before it to 30 after (to 30 after its last frame
for a clip), under rundesk; each of the original's pictures is then matched with the port's screenshots over the region
(the whole screen by default), blurred as shotdiff blurs, and the best two are printed with how many pixels still differ
visibly. A lag of 0 means the port drew the same picture at the same update; a best match far above 0 pixels means the
difference is not timing at all. The original's pictures come from build/scratch/pilot/orig, as run.sh left them.
"""
import os
import subprocess
import sys

import numpy as np
from PIL import Image, ImageFilter

W = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..", ".."))
P = os.path.join(W, "build", "scratch", "pilot")
T = os.path.dirname(os.path.abspath(__file__))


def main():
    steps, name = sys.argv[1:3]
    box = tuple(int(v) for v in sys.argv[3:7]) if len(sys.argv) >= 7 else (0, 0, 800, 600)
    script = open(os.path.join(P, "scripts", steps + ".port.txt")).read().splitlines()
    # the port's updates of the picture (a still) or of each frame (a clip): its screenshot lines
    shot = [int(l.split()[0]) for l in script if " screenshot " in l and
            (l.endswith("/%s.png" % name) or "/%s/" % name in l)]
    if not shot:
        sys.exit("no screenshot of %s in %s's port script" % (name, steps))
    first, last = shot[0] - 30, shot[-1] + 30
    d = os.path.join(P, "lag", name)
    os.makedirs(d, exist_ok=True)
    lines = [l for l in script if int(l.split()[0]) <= last and " screenshot " not in l and not l.endswith(" quit")]
    lines += ["%d screenshot %s/%d.png" % (u, d.replace("\\", "/"), u) for u in range(first, last + 1)]
    lines.append("%d quit" % (last + 5))
    open(os.path.join(d, "script.txt"), "w").write("\n".join(lines) + "\n")
    user = os.path.join(d, "user")
    subprocess.run(["bash", os.path.join(T, "reset.sh"), user], check=True, stdout=subprocess.DEVNULL)
    with open(os.path.join(d, "stdout.txt"), "w") as out:
        subprocess.run([os.path.join(W, "build", "native", "rundesk.exe"), os.path.join(W, "build", "native", "bookwormadv.exe"),
                        "-userdir=" + user, "-seed=12345", "-drawevery", "-script=" + os.path.join(d, "script.txt")],
                       stdout=out, stderr=subprocess.STDOUT, timeout=280)

    def pic(im):
        return np.asarray(im.convert("RGB").crop(box).filter(ImageFilter.BoxBlur(1))).astype(int)

    port = {u - 1: pic(Image.open(os.path.join(d, "%d.png" % u))) for u in range(first, last + 1)}  # u shows u-1
    src = os.path.join(P, "orig", name)
    origs = ([os.path.join(src, f) for f in sorted(os.listdir(src)) if f.endswith(".png")] if os.path.isdir(src)
             else [src + ".png"])
    for f, u in zip(origs, shot):
        im = Image.open(f)
        if im.size != (800, 600):
            im = im.crop((3, 26, 803, 626))  # the original's window frame inside Windows Sandbox (806x629)
        o = pic(im)
        best = sorted((int((np.abs(o - v).max(2) >= 32).sum()), k) for k, v in port.items())[:2]
        print("%s at update %d: port update %d (lag %+d), %d pixels; next %d, %d pixels" % (
            os.path.basename(f), u - 1, best[0][1], best[0][1] - (u - 1), best[0][0], best[1][1], best[1][0]))


if __name__ == "__main__":
    main()
