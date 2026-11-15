"""The 3D screen wipe, frame by frame: the original on the desktop and the port, shot at the same updates (GAME_NOTES section 10).

    python -I wipeshots.py tour <dir> [every [count]]
        writes <dir>/tour.txt (a classic game as the tours play it, then Escape, the Main Menu button and the End Game box's Yes, which
        starts the wipe, then a shot every <every> updates (default 4), <count> of them (default 45)) and, through tour.py, <dir>/orig.txt
        and <dir>/port.txt
    python -I wipeshots.py orig <dir> <seed> [--pos x,y]
        the original in 3D on the desktop (visrun3d.py, 22:00 to 08:00) with the seed patch of wipeseeds.py, through <dir>/orig.txt:
        its shots and <dir>/orig/steps.tsv
    python -I wipeshots.py port <dir> [<first> <last>]
        writes <dir>/port-all.txt, the port's script from the steps the run of the original landed at (<dir>/orig/steps.tsv): each input at
        its update, then a shot of every update from <first> to <last> into <dir>/port-all/ (wipematch.py compares the two sets)

The original (visrun3d.py, 22:00 to 08:00) runs on the real clock, so a step lands on the update asked for or a few after it: every input
and shot is written with the update C it landed at (steps.tsv), and the port's script puts each input at C plus the tour's own offset for
that kind of input (tour.py: the port's click is stamped one earlier than the original's). A shot of the original shows the frame of C or of
one or two updates before it (see visrun3d.py), so the port shoots every update and wipematch.py takes the best match of each. wipe.sh runs
the whole chain for one seed. The offsets were tried first: the port's inputs one update earlier than the tour's, at the tour's, and one
later, and its shots at C, C+1 and C+2 (a table of the differing pixels each gave): inputs at the tour's offset gave the most exact matches.
"""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
TOUR = os.path.join(ROOT, "games", "astropop", "tools", "tour", "tour.py")
INGAME = os.path.join(ROOT, "games", "astropop", "tools", "tour", "ingame1.txt")


def make_tour(d, every, count):
    os.makedirs(d, exist_ok=True)
    lines = ["# the classic game of ingame1.txt up to its last move, the options box, Main Menu, End Game and Yes (the wipe), then shots"]
    for ln in open(INGAME):
        w = ln.split()
        if w and not ln.startswith("#") and int(w[0]) <= 3393 and w[1] != "shot":
            lines.append(ln.rstrip())
    lines += ["3450 key Escape", "3470 shot pre_options", "3490 click 230 388", "3510 shot pre_endbox", "3530 click 255 279"]
    for k in range(count):
        lines.append("%d shot w%03d" % (3532 + every * k, k))
    open(os.path.join(d, "tour.txt"), "w", newline="\n").write("\n".join(lines) + "\n")
    subprocess.run([sys.executable, "-I", TOUR, os.path.join(d, "tour.txt"), d], check=True)


def run_original(d, seed, pos):
    sys.path.insert(0, HERE)
    import visrun3d as v
    import wipeseeds
    first = v.prepare

    def prepare_with_seed():
        first()
        wipeseeds.patch_seed(os.path.join(v.GAME, "AstroPopGame.exe"), seed)
    v.prepare = prepare_with_seed
    sys.argv = ["visrun3d.py", os.path.join(d, "orig.txt"), "--shots", os.path.join(d, "orig")] + (["--pos", pos] if pos else [])
    v.main()


def make_port(d, first=None, last=None):
    """<dir>/port-all.txt: every input of the original's run at the update it landed at (plus the tour's own offset for that kind of input,
    tour.py's: the port's click is stamped one earlier than the original's), then a shot of every update from `first` to `last` (default:
    from 3528 to four after the original's last shot) into <dir>/port-all/<update>.png."""
    steps = [l.rstrip("\n").split("\t") for l in open(os.path.join(d, "orig", "steps.tsv"))]
    steps = [x for x in steps if x[2:4] != ["move", "1 1"]]      # the tour's closing move of the original (the port's script ends in quit)
    orig = [l.split(None, 3) for l in open(os.path.join(d, "orig.txt")) if l.startswith("at ") and " move 1 1" not in l]
    port = [l.rstrip("\n").split(None, 1) for l in open(os.path.join(d, "port.txt")) if not l.rstrip().endswith(" quit")]
    if len(orig) != len(port):
        raise SystemExit("wipeshots: orig.txt has %d steps and port.txt %d" % (len(orig), len(port)))
    pdir = os.path.abspath(os.path.join(d, "port-all")).replace(os.sep, "/")
    out, k = [], 0
    last_shot = 0
    for n, c, cmd, rest in (s[:4] for s in steps):
        # the next step of the tour that this one is (a refused shot is missing from the run's steps)
        while k < len(orig) and not (orig[k][2] == cmd and (orig[k][3].split()[:1] == [rest] if cmd == "shot" else int(orig[k][1]) == int(n))):
            k += 1
        if k == len(orig):
            raise SystemExit("wipeshots: step %s %s %s is not in orig.txt" % (n, cmd, rest))
        if cmd == "shot":
            last_shot = max(last_shot, int(c))
        else:
            out.append((int(c) + int(port[k][0]) - int(orig[k][1]), port[k][1]))
        k += 1
    first = 3528 if first is None else first
    last = last_shot + 4 if last is None else last
    out += [(u, "screenshot %s/%d.png" % (pdir, u)) for u in range(first, last + 1)]
    out.append((last + 20, "quit"))
    out.sort(key=lambda x: x[0])
    shutil.rmtree(os.path.join(d, "port-all"), ignore_errors=True)
    os.makedirs(os.path.join(d, "port-all"))
    path = os.path.join(d, "port-all.txt")
    open(path, "w", newline="\n").write("".join("%d %s\n" % x for x in out))
    print(path)


def main():
    a = sys.argv[1:]
    if not a:
        sys.stderr.write(__doc__)
        return 2
    if a[0] == "tour":
        make_tour(a[1], int(a[2]) if len(a) > 2 else 4, int(a[3]) if len(a) > 3 else 45)
    elif a[0] == "orig":
        run_original(a[1], int(a[2]), a[a.index("--pos") + 1] if "--pos" in a else None)
    elif a[0] == "port":
        make_port(a[1], int(a[2]) if len(a) > 2 else None, int(a[3]) if len(a) > 3 else None)
    else:
        sys.stderr.write(__doc__)
        return 2


if __name__ == "__main__":
    sys.exit(main())
