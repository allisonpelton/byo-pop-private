#!/usr/bin/env python3
"""Makes the port's script for a desktop run of the original (visrun3d.py): every step the run did (build/ts-p2/<name>/vis3d.steps.tsv, or
vis2d for 2d) goes in at the update it landed at on the real clock, so the port gets each input and each shot at the update the original
did. The command itself is the tour's own port line for that step (build/ts-p2/<name>/port.txt, made with orig.txt by p2lib.py), so a typed
space stays `char SPACE` where the original's line is `key 32`; only its update number and the shot's file name change.

    python mkport3d.py <name> [2d]      writes build/ts-p2/<name>_p3d/port.txt (<name>_p2d with 2d)
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import ROOT
from tsp import PORT_SHIFT  # noqa: E402


def main():
    name = sys.argv[1]
    two = len(sys.argv) > 2 and sys.argv[2] == "2d"
    d = os.path.join(ROOT, "build", "ts-p2", name)
    steps = [l.rstrip("\n").split("\t") + [""] for l in open(os.path.join(d, "vis2d.steps.tsv" if two else "vis3d.steps.tsv"))]
    orig = [l.split(None, 3) for l in open(os.path.join(d, "orig.txt")) if l.startswith("at ") and " peek" not in l]
    port = [l.rstrip("\n").split(None, 1) for l in open(os.path.join(d, "port.txt")) if not l.rstrip().endswith(" quit")]
    if len(orig) != len(port):
        raise SystemExit("mkport3d: orig.txt has %d steps and port.txt %d" % (len(orig), len(port)))
    out_name = name + ("_p2d" if two else "_p3d")
    out = os.path.join(ROOT, "build", "ts-p2", out_name)
    pd = os.path.join(out, "port").replace("\\", "/")
    if pd.startswith("/c/"):
        pd = "C:/" + pd[3:]
    lines, k, last = [], 0, 0
    for n, c, cmd, rest in (s[:4] for s in steps):
        # the next step of the tour that this one is (a refused shot is missing from the run's steps)
        # (a shot is matched by its name: the run may have asked for it an update later than the script, after an input)
        while k < len(orig) and not (orig[k][2] == cmd and (orig[k][3].split()[:1] == [rest] if cmd == "shot" else orig[k][1] == n)):
            k += 1
        if k == len(orig):
            raise SystemExit("mkport3d: step %s %s %s is not in orig.txt" % (n, cmd, rest))
        text = port[k][1]
        k += 1
        if cmd == "shot":
            text = "shot %s/%s.png" % (pd, rest)
        c = int(c)
        last = max(last, c)
        lines.append("%d %s" % (c + PORT_SHIFT, text))
    lines.append("%d quit" % (last + 20 + PORT_SHIFT))
    os.makedirs(out, exist_ok=True)
    open(os.path.join(out, "port.txt"), "w", newline="\n").write("\n".join(lines) + "\n")
    print(out)


main()
