"""Turns the port's bot script (-autoplay ... -autoplaylog=<file>) into an origrun script for the original.

    python games/bejeweled/tools/origcmp/fromport.py <bot.txt> [--title-click U] [--slack N] [--shots K] [--end U]

The original's title link appears later than the port's (its loading takes time), so the bot's title click moves to
update U (default 800) and everything after it by the same amount. origrun holds each input until the original is
outside its update code, which can post it a few updates late; every later input is moved on by --slack updates
(default 40) more, so a late input never lands before the board is ready for the next. A drag (down, move, up) becomes
two clicks, since a down and an up the original reads in one update are lost. A shot every --shots updates (default
25, 0 for none) runs from the title click to the end. The port is then given the original's real input updates by
toport.py.
"""
import sys


def main():
    a = sys.argv[1:]
    opt = {"--title-click": 800, "--slack": 40, "--shots": 25, "--end": 0}
    for k in list(opt):
        if k in a:
            i = a.index(k)
            opt[k] = int(a[i + 1])
            del a[i:i + 2]
    cmds = []
    for ln in open(a[0]):
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        p = ln.split()
        if p[1] != "quit":
            cmds.append((int(p[0]), p[1], [int(v) for v in p[2:]]))
    # drags to two clicks
    out = []
    i = 0
    while i < len(cmds):
        u, c, v = cmds[i]
        if c == "down":
            j = i + 1
            mv = None
            while j < len(cmds) and cmds[j][1] in ("move", "up"):
                if cmds[j][1] == "move":
                    mv = cmds[j]
                j += 1
            out.append((u, "click", v))
            if mv:
                dx, dy = mv[2][0] - v[0], mv[2][1] - v[1]
                if abs(dx) > abs(dy):
                    t = (v[0] + (51 if dx > 0 else -51), v[1])
                else:
                    t = (v[0], v[1] + (51 if dy > 0 else -51))
                out.append((mv[0], "click", list(t)))
            i = j
            continue
        out.append((u, c, v))
        i += 1
    base = out[0][0]
    shift = opt["--title-click"] - base
    lines = []
    for n, (u, c, v) in enumerate(out):
        lines.append((u + shift + opt["--slack"] * n, "%s %s" % (c, " ".join(map(str, v)))))
    end = max(opt["--end"], lines[-1][0] + 600)
    if opt["--shots"] > 0:
        s = opt["--title-click"] + 1
        while s <= end:
            lines.append((s, "shot g%06d" % s))
            s += opt["--shots"]
    lines.sort(key=lambda t: (t[0], 0 if t[1].startswith("shot") else 1))
    for u, t in lines:
        print("at %d %s" % (u, t))
    print("quit")


if __name__ == "__main__":
    main()
