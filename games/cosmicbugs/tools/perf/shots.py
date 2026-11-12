#!/usr/bin/env python
"""shots.py: two exes' screenshots over one scripted play, compared byte by byte: the proof that a change of how the port draws
(the performance pass) leaves the pictures as they were.

  build/native/rundesk.exe python games/cosmicbugs/tools/perf/shots.py A.exe B.exe [OUTDIR]

Each exe runs the same -script windowed with -fast -drawevery (every update is drawn, so a screenshot shows its update's frame):
the logo, the title, the instructions page, the first board and shots fired into it, 14 screenshots in all. Prints one line,
"SAME n of n" or the names of the pictures that differ (and exits 1).
"""
import filecmp
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip()).resolve()
CONFIG = '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n'
# the framework's updates are 10 ms: Start Game, the instructions page's Play, then clicks on the board (shots are fired at the pointer)
CLICKS = [(300, 175, 313), (400, 400, 563), (750, 400, 300), (900, 200, 200), (1100, 600, 400), (1500, 120, 500), (2200, 700, 150),
          (3000, 400, 100), (3600, 250, 350)]
SHOTS = [100, 250, 350, 700, 800, 1000, 1400, 1800, 2400, 3100, 3700, 4200, 4600, 5000]


def run(exe, out):
    shutil.rmtree(out, ignore_errors=True)
    (out / "user").mkdir(parents=True)
    (out / "user" / "cp.cfg").write_text(CONFIG)
    lines = []
    for (u, x, y) in CLICKS:
        lines += ["%d down %d %d" % (u, x, y), "%d up %d %d" % (u + 3, x, y)]
    lines += ["%d screenshot %s" % (u, (out / ("s%04d.png" % u)).as_posix()) for u in SHOTS]
    lines += ["5010 quit"]
    script = out / "script.txt"
    script.write_text("\n".join(lines) + "\n")
    cmd = [str(Path(exe).resolve()), "-gamedir=" + str(ROOT / "assets/cosmicbugs"), "-userdir=" + str(out / "user"), "-fast", "-drawevery",
           "-script=" + str(script), "-modsdir=" + str(ROOT / "build/native/mods-cosmicbugs")]
    subprocess.run(cmd, cwd=str(ROOT), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=300)
    return sorted(out.glob("s*.png"))


def main():
    a, b = sys.argv[1], sys.argv[2]
    out = Path(sys.argv[3]) if len(sys.argv) > 3 else ROOT / "build/cbperf/shots"
    pa, pb = run(a, out / "a"), run(b, out / "b")
    bad = [x.name for x in pa if not (out / "b" / x.name).exists() or not filecmp.cmp(x, out / "b" / x.name, shallow=False)]
    if len(pa) != len(SHOTS) or len(pb) != len(SHOTS) or bad:
        print("DIFFERENT: %d and %d pictures of %d; differing: %s" % (len(pa), len(pb), len(SHOTS), ", ".join(bad)))
        sys.exit(1)
    print("SAME %d of %d" % (len(pa), len(SHOTS)))


main()
