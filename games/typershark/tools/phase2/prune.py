#!/usr/bin/env python3
"""Delete a tour's raw shots once shotdiff has judged them, so that build/ stays small (the coordinator, 2026-11-10).

    python games/typershark/tools/phase2/prune.py <name>          the pairs shotdiff found equal: their PNGs in build/ts-p2/<name>/{orig,port}
    python games/typershark/tools/phase2/prune.py <name> --all    every shot of the tour, the port's users folder and the original's (the
                                                                  tour is judged: its flagged pairs are fixed or explained in the notes)

Reads build/shotreview/typershark/p2-<name>/summary.json. The scripts (orig.txt, port.txt) and the logs stay: a tour is run again from
them (chain.sh). chain.sh calls this after every comparison unless KEEP=1."""
import json
import os
import shutil
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))


def main():
    name = sys.argv[1]
    d = os.path.join(ROOT, "build", "ts-p2", name)
    if "--all" in sys.argv:
        for sub in ("orig", "port", "ud", "orig.users"):
            shutil.rmtree(os.path.join(d, sub), ignore_errors=True)
        print("pruned %s: every shot" % name)
        return
    s = json.load(open(os.path.join(ROOT, "build", "shotreview", "typershark", "p2-" + name, "summary.json")))
    n = 0
    for p in s["pairs"]:
        if p["status"] != "match":
            continue
        for side in ("orig", "port"):
            f = os.path.join(d, side, p["name"])
            if os.path.exists(f):
                os.remove(f)
                n += 1
    for f in s.get("port_only") or []:        # the port's shots of pairs judged equal before (their original's shot is gone)
        f = os.path.join(d, "port", f)
        if os.path.exists(f):
            os.remove(f)
            n += 1
    left = sum(len(os.listdir(os.path.join(d, side))) for side in ("orig", "port") if os.path.isdir(os.path.join(d, side)))
    for sub in ("ud", "orig.users"):
        shutil.rmtree(os.path.join(d, sub), ignore_errors=True)
    print("pruned %s: %d shots of equal pairs deleted, %d left to judge" % (name, n, left))


if __name__ == "__main__":
    main()
