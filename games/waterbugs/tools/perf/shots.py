#!/usr/bin/env python
"""shots.py: two exes' screenshots over one scripted play, compared byte by byte: the proof that a change of how the port draws
(the performance pass) leaves the pictures as they were.

  build/native/rundesk.exe python games/waterbugs/tools/perf/shots.py A.exe B.exe [OUTDIR] [--long SEED] [--env-a K=V]

Each exe runs the same -script windowed with -fast -drawevery (every update is drawn, so a screenshot shows its update's frame):
the logo, the main menu, the world map, the story screen, the first board and beams fired into it, 16 screenshots in all. Prints one line,
"SAME n of n" or the names of the pictures that differ (and exits 1). With --long SEED the play is 90 s of game time: after the
first board's clicks, random clicks (a seeded generator, the aim switched at random, as tools/play/gen.py plays) and a picture every
300 updates, 300 in all (a few hundred MB: the script deletes them when they are the same); --env-a K=V sets an environment
variable for the first exe only.
"""
import filecmp
import os
import random
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip()).resolve()
CONFIG = '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n'
# the framework's updates are 10 ms: Start Game, the world map's first area, the story screen's button, then clicks on the board
# (each fires a beam toward it)
CLICKS = [(300, 400, 303), (800, 76, 263), (1400, 400, 467), (1700, 400, 300), (1900, 200, 200), (2200, 600, 400), (2800, 120, 500),
          (3500, 700, 150), (4300, 400, 100), (5000, 250, 350)]
SHOTS = [100, 250, 350, 700, 900, 1000, 1300, 1450, 1600, 1750, 2000, 2400, 3000, 3700, 4400, 5100]


def long_play(seed):
    """(clicks, right clicks, shots, the last update) of 90 s of game time: random clicks 1.5 to 3.5 s apart on the first board"""
    rng = random.Random(seed)
    clicks, rights = list(CLICKS[:3]), []
    u = 1600
    while u < 9000:
        if rng.random() < 0.4:
            rights.append(u - 50)
        clicks.append((u, rng.randint(120, 680), rng.randint(130, 500)))
        u += rng.randint(150, 350)
    return clicks, rights, list(range(1500, 9000, 30)), 9010


def run(exe, out, long_seed=None, env=None):
    shutil.rmtree(out, ignore_errors=True)
    (out / "user").mkdir(parents=True)
    (out / "user" / "cp.cfg").write_text(CONFIG)
    clicks, rights, shots, end = (CLICKS, [], SHOTS, 5200) if long_seed is None else long_play(long_seed)
    lines = []
    for (u, x, y) in clicks:
        lines += ["%d down %d %d" % (u, x, y), "%d up %d %d" % (u + 3, x, y)]
    for u in rights:
        lines += ["%d down 2" % u, "%d up 2" % (u + 3)]
    lines += ["%d screenshot %s" % (u, (out / ("s%04d.png" % u)).as_posix()) for u in shots]
    lines += ["%d quit" % end]
    script = out / "script.txt"
    script.write_text("\n".join(lines) + "\n")
    cmd = [str(Path(exe).resolve()), "-gamedir=" + str(ROOT / "assets/waterbugs"), "-userdir=" + str(out / "user"), "-fast", "-drawevery",
           "-script=" + str(script), "-modsdir=" + str(ROOT / "build/native/mods-waterbugs")]
    subprocess.run(cmd, cwd=str(ROOT), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=900, env=dict(os.environ, **(env or {})))
    return sorted(out.glob("s*.png"))


def main():
    args = sys.argv[1:]
    seed, env_a = None, {}
    if "--long" in args:
        i = args.index("--long")
        seed = int(args[i + 1])
        del args[i:i + 2]
    if "--env-a" in args:
        i = args.index("--env-a")
        k, v = args[i + 1].split("=", 1)
        env_a = {k: v}
        del args[i:i + 2]
    a, b = args[0], args[1]
    out = Path(args[2]) if len(args) > 2 else ROOT / "build/wbperf/shots"
    pa, pb = run(a, out / "a", seed, env_a), run(b, out / "b", seed)
    count = len(SHOTS) if seed is None else len(long_play(seed)[2])
    bad = [x.name for x in pa if not (out / "b" / x.name).exists() or not filecmp.cmp(x, out / "b" / x.name, shallow=False)]
    if len(pa) != count or len(pb) != count or bad:
        print("DIFFERENT: %d and %d pictures of %d; differing: %s" % (len(pa), len(pb), count, ", ".join(bad[:20])))
        sys.exit(1)
    print("SAME %d of %d" % (len(pa), count))
    if seed is not None:
        shutil.rmtree(out, ignore_errors=True)


main()
