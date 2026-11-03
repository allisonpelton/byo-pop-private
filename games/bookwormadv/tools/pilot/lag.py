"""lag.py <steps name> <shot or clip name> [x0 y0 x1 y1]

Which update of the port each of the original's pictures shows. The port runs the pilot's script (gen.py's, made by
run.sh) up to the picture, taking a screenshot at every update from 30 before it to 30 after (to 30 after its last frame
for a clip), under rundesk; each of the original's pictures is then matched with the port's screenshots over the region
(the whole screen by default), blurred as shotdiff blurs, and the run of port updates that match best is printed with how
many pixels still differ visibly (a still picture matches many). Against more than 300 port screenshots (a long clip)
the pictures are compared at a quarter of their size, or the matching would take many minutes. A lag of 0 means the
port drew the same picture at the same update; a best match far above 0 pixels means the
difference is not timing at all. The original's pictures come from build/scratch/pilot/orig, as run.sh left them.
With PILOT3D=on, the port runs in 3D mode and the original's pictures come from build/scratch/pilot/orig3d (visrun.py
leaves them there, with their landed.tsv), as do the load lengths (<steps>3d.loads).
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
    suf = "3d" if os.environ.get("PILOT3D") == "on" else ""
    box = tuple(int(v) for v in sys.argv[3:7]) if len(sys.argv) >= 7 else (0, 0, 800, 600)
    script = open(os.path.join(P, "scripts", steps + ".port.txt")).read().splitlines()
    # the port's updates of the picture (a still) or of each frame (a clip): its screenshot lines
    shot = [int(l.split()[0]) for l in script if " screenshot " in l and
            (l.endswith("/%s.png" % name) or "/%s/" % name in l)]
    if not shot:
        sys.exit("no screenshot of %s in %s's port script" % (name, steps))
    first, last = shot[0] - 30, shot[-1] + 30
    d = os.path.join(P, "lag" + suf, name)
    os.makedirs(d, exist_ok=True)
    lines = [l for l in script if int(l.split()[0]) <= last and " screenshot " not in l and not l.endswith(" quit")]
    lines += ["%d screenshot %s/%d.png" % (u, d.replace("\\", "/"), u) for u in range(first, last + 1)]
    lines.append("%d quit" % (last + 5))
    open(os.path.join(d, "script.txt"), "w").write("\n".join(lines) + "\n")
    lf = os.path.join(P, steps + suf + ".loads")  # the original's chapter loads, in updates (run.sh)
    loads = open(lf).read().strip() if os.path.exists(lf) else ""
    sf = os.path.join(P, steps + suf + ".startload")  # and the startup load's
    startload = open(sf).read().strip() if os.path.exists(sf) else ""
    user = os.path.join(d, "user")
    subprocess.run(["bash", os.path.join(T, "reset.sh"), user], check=True, stdout=subprocess.DEVNULL)
    if suf:
        ini = os.path.join(user, "registry.ini")
        text = open(ini).read()
        open(ini, "w").write(text.replace("Is3D=dword:0", "Is3D=dword:1"))
    with open(os.path.join(d, "stdout.txt"), "w") as out:
        subprocess.run([os.path.join(W, "build", "native", "rundesk.exe"), os.path.join(W, "build", "native", "bookwormadv.exe"),
                        "-userdir=" + user, "-seed=12345", "-drawevery", "-chapterloadupdates=" + loads, "-startloadupdates=" + startload, "-script=" + os.path.join(d, "script.txt")],
                       stdout=out, stderr=subprocess.STDOUT, timeout=280)

    small = last - first > 300

    def pic(im):
        im = im.convert("RGB").crop(box).filter(ImageFilter.BoxBlur(1))
        if small:
            im = im.resize((im.width // 4, im.height // 4), Image.BILINEAR)
        return np.asarray(im).astype(np.int16)

    keys = list(range(first - 1, last))  # the port's screenshot u shows update u-1
    stack = np.stack([pic(Image.open(os.path.join(d, "%d.png" % (k + 1)))) for k in keys])
    src = os.path.join(P, "orig" + suf, name)
    origs = ([os.path.join(src, f) for f in sorted(os.listdir(src)) if f.endswith(".png")] if os.path.isdir(src)
             else [src + ".png"])
    lt = (os.path.join(P, "orig3d", "landed.tsv") if suf else
          os.path.join(W, "build", "origrun", "pilot-" + steps, "shots", "landed.tsv"))
    landed = {l.split("	")[0]: int(l.split("	")[1]) for l in open(lt).read().splitlines()} if os.path.exists(lt) else {}
    for f, u in zip(origs, shot):
        im = Image.open(f)
        if im.size != (800, 600):
            im = im.crop((3, 26, 803, 626))  # the original's window frame inside Windows Sandbox (806x629)
        o = pic(im)
        diff = (np.abs(stack - o).max(3) >= 32).sum((1, 2))
        low = int(diff.min())
        ties = [keys[i] for i in np.nonzero(diff == low)[0]]  # a still picture matches a run of updates
        # the update the original's shot really showed (landed.tsv, from origrun), else the one it was taken at
        at = landed.get(name + "/" + os.path.splitext(os.path.basename(f))[0], landed.get(name, u - 1))
        print("%s at update %d: port updates %d-%d (lag %+d to %+d), %d pixels" % (
            os.path.basename(f), at, ties[0], ties[-1], ties[0] - at, ties[-1] - at, low))


if __name__ == "__main__":
    main()
