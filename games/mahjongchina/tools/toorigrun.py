#!/usr/bin/env python3
"""Turn a port -script (adventure.py's, or one written by hand) into an origrun script for the original.

The port's script is timed in the framework's updates (200 a second, 5 ms each); origrun's waits are in milliseconds and its clicks are posted, so
each click gets a posted move 150 ms before it (the game reads the hover from it). The port's first command waits for
the splash and the loading; the original's run waits for its first frame and seven seconds instead. A screenshot
becomes a shot of the same name.

    python games/mahjongchina/tools/toorigrun.py <port script> [--scale <factor>] > script.txt

--scale stretches or shortens every wait after the splash (origrun stops a run after 240 s).
"""
import os
import sys

KEYS = {"RETURN": 13, "ESCAPE": 27, "SPACE": 32}


def main():
    scale = float(sys.argv[sys.argv.index("--scale") + 1]) if "--scale" in sys.argv else 1.0
    lines = [l.split(None, 1) for l in open(sys.argv[1]) if l.strip() and not l.startswith("#")]
    out = ["waitshot splash 30000", "wait 7000"]
    prev = None
    text = ""
    for t, cmd in ((int(a), b.strip()) for a, b in lines):
        p = cmd.split()
        if p[0] != "char" and text:
            out.append("text " + text)
            text = ""
        wait = 0 if prev is None else int((t - prev) * 5 * scale)
        prev = t
        if p[0] == "click":
            out.append("wait %d" % max(wait - 150, 0))
            out.append("move %s %s" % (p[1], p[2]))
            out.append("wait 150")
            out.append("click %s %s" % (p[1], p[2]))
        elif p[0] == "char":
            if not text and wait:
                out.append("wait %d" % wait)
            text += p[1]
        elif p[0] == "key":
            out.append("wait %d" % wait)
            out.append("key %d" % KEYS[p[1]])
        elif p[0] == "screenshot":
            out.append("wait %d" % wait)
            out.append("shot " + os.path.splitext(os.path.basename(p[1]))[0])
        elif p[0] == "quit":
            out.append("wait %d" % wait)
            out.append("quit")
        else:
            sys.exit("no origrun form for: " + cmd)
    print("\n".join(l for l in out if l != "wait 0"))


main()
