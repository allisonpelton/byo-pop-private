#!/usr/bin/env python3
"""Tour G: replay a phase-1 scenario's recorded input (build/ts-p1/<tag>.port.txt: the menu clicks and the port's bot's letters, clicks and
keys, one line `<update> char|click|key ...` for the port) in both programs, with shots at updates chosen from that run's state log
(build/ts-p1/<tag>.port.sl): what is on the screen is read from the log, never from the game's text files.

    python tour_game.py <name> <tag> [--grid N] [--max M] [--lo U] [--hi U] [--seed S]

The shots: the first update of each creature kind (and 12, 40 updates later), of each boss, boss shot and bonus creature; the first updates of
each level (the fade-in, the level box, the first wave); the hint dialog; the pause; the end of a level (the countdown, the diver's
states); the game-over dialog; the stretches with no board (the map) at their start and then every so often; the typing tutor; and a
grid every N updates (default 600). At most M shots (default 700). Names: <update>_<what>.png.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "phase1"))
from p2lib import Tour, ROOT, PORT_SHIFT

P1 = os.path.join(ROOT, "build", "ts-p1")


def load_inputs(path):
    """the port's script lines (update char|click|key ...) -> steps in the original's numbering (the port's line N is the original's `at N-1`)"""
    steps = []
    for line in open(path):
        f = line.split()
        if len(f) < 3 or f[1] == "quit":
            continue
        u = int(f[0]) - PORT_SHIFT
        if f[1] == "char":
            steps.append((u, "text1", " " if f[2] == "SPACE" else f[2]))
        elif f[1] == "click":
            steps.append((u, "click", int(f[2]), int(f[3]), None))
        elif f[1] == "key":
            steps.append((u, "key", 13 if f[2] == "RETURN" else int(f[2])))
        elif f[1] == "move":
            steps.append((u, "move", int(f[2]), int(f[3])))
        else:
            raise SystemExit("unhandled script line: " + line)
    return steps


KIND = re.compile(r"(?:^| )([A-Za-z]+):")


def scan_log(path, lo, hi):
    """per update: dict of what the state log says (only the fields the selection needs)"""
    rows = {}
    for line in open(path):
        m = re.match(r"u=(\d+) ", line)
        if not m:
            continue
        u = int(m.group(1))
        if u < lo or u > hi:
            continue
        head, _, tail = line.partition(" |")
        r = {"noboard": "noboard=1" in head, "tut": " tut=1" in head}
        for k in ("lvl", "dstate", "hint", "paused", "gameover", "boss", "bonus", "clams", "typed", "pairs", "tfirst", "tles", "tst", "texp", "ttask"):
            mm = re.search(r"(?:^| )%s=(-?\d+)" % k, head)
            r[k] = int(mm.group(1)) if mm else 0
        mm = re.search(r" f38=(-?[\d.]+)", head)
        r["f38"] = float(mm.group(1)) if mm else 0.0
        mm = re.search(r" sw=(\S+)", head)
        r["sw"] = mm.group(1) if mm else ""
        r["kinds"] = set(KIND.findall(tail))
        rows[u] = r
    return rows


def choose(rows, grid, lo, hi, clicks=()):
    shots = {}

    def add(u, what):
        if lo <= u <= hi and u not in shots:
            shots[u] = what

    seen_kind = set()
    seen_exp = set()
    prev = None
    nob_start = None
    for u in sorted(rows):
        r = rows[u]
        for k in sorted(r["kinds"]):
            if k not in seen_kind:
                seen_kind.add(k)
                for d in (2, 14, 40):
                    add(u + d, f"first_{k}_{d}")
        if prev is not None:
            p = rows[prev]
            if r["lvl"] != p["lvl"] and not r["noboard"] and not p["noboard"]:
                for d in (1, 30, 120, 160, 250):
                    add(u + d, f"lvl{r['lvl']}_{d}")
            if r["hint"] and not p["hint"]:
                for d in (2, 12, 25):
                    add(u + d, f"hint_{d}")
            if r["paused"] and not p["paused"]:
                for d in (3, 30):
                    add(u + d, f"paused_{d}")
            if r["gameover"] and not p["gameover"]:
                for d in (2, 12, 40, 120):
                    add(u + d, f"gameover_{d}")
            if r["dstate"] != p["dstate"] and not r["noboard"]:
                for d in ((2, 20, 60, 100, 150, 200, 300, 400, 490) if r["dstate"] == 7 else (2, 20)):
                    add(u + d, f"dstate{r['dstate']}_{d}")
            if r["f38"] > 0 and p["f38"] <= 0:
                for d in (2, 120, 245):
                    add(u + d, f"endcount_{d}")
            if r["sw"] and not p["sw"]:
                for d in (2, 30):
                    add(u + d, f"streak_{d}")
            if r["boss"] and not p["boss"]:
                for d in (3, 30, 90, 200):
                    add(u + d, f"boss_{d}")
            if r["bonus"] and not p["bonus"]:
                for d in (3, 20):
                    add(u + d, f"bonuscreature_{d}")
            if r["clams"] and not p["clams"]:
                for d in (3, 20, 80):
                    add(u + d, f"clams_{d}")
            if r["pairs"] != p["pairs"] and r["clams"]:
                add(u + 2, f"pair{r['pairs']}")
            if r["tut"] and not p["tut"]:
                for d in (3, 30):
                    add(u + d, f"tutor_{d}")
            if r["tut"] and p["tut"]:
                if r["tfirst"] != p["tfirst"]:
                    add(u + 2, f"tfirst{r['tfirst']}")
                if r["tles"] != p["tles"]:
                    for d in (2, 30):
                        add(u + d, f"lesson{r['tles']}_{d}")
                if r["tst"] != p["tst"]:
                    for d in (2, 20, 120):
                        add(u + d, f"tst{r['tst']}_{d}")
                if r["ttask"] != p["ttask"]:
                    add(u + 2, f"task{r['ttask']}")
                if r["texp"] in (42, 56, 123, 125, 95, 124, 92, 33, 64, 35, 36, 37, 94, 38, 40, 41, 45, 43, 61, 91, 93, 59, 39, 58, 34, 44, 46, 47, 60, 62, 63, 126, 96) and r["texp"] not in seen_exp:
                    seen_exp.add(r["texp"])
                    add(u + 2, f"key{r['texp']}")
            if r["noboard"] and not p["noboard"]:
                nob_start = u
            if not r["noboard"] and p["noboard"] and nob_start is not None and u - nob_start > 200:
                n = u - nob_start
                for d in (2, 20, 60, 120, 250, 400, 700, 1000, 1500):
                    if d < n:
                        add(nob_start + d, f"map_{d}")
                add(u - 3, "map_end")
        prev = u
    for u in clicks:                        # every click after the menu flow: the screen it meets and the one it leaves
        add(u - 2, "click_pre")
        add(u + 4, "click_post")
    first = min(rows)
    for u in range(first - first % grid + grid, max(rows), grid):
        if u in rows:
            add(u, "grid")
    return shots


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("tag")
    ap.add_argument("--grid", type=int, default=600)
    ap.add_argument("--max", type=int, default=700)
    ap.add_argument("--lo", type=int, default=0)
    ap.add_argument("--hi", type=int, default=10 ** 9)
    ap.add_argument("--sl", default=None, help="state log (default build/ts-p1/<tag>.port.sl)")
    ap.add_argument("--input", default=None, help="port script (default build/ts-p1/<tag>.port.txt)")
    ap.add_argument("--peek", action="store_true", help="also peek the update counter, the board's update count, level, score and lives in the original at every shot")
    ap.add_argument("--quit", type=int, default=0)
    a = ap.parse_args()
    t = Tour(a.name)
    t.steps = load_inputs(a.input or os.path.join(P1, a.tag + ".port.txt"))
    rows = scan_log(a.sl or os.path.join(P1, a.tag + ".port.sl"), a.lo, a.hi)
    first_u = min(rows) if rows else 0
    clicks = [st[0] for st in t.steps if st[1] == "click" and st[0] > first_u + 200]
    shots = choose(rows, a.grid, a.lo, a.hi, clicks)
    items = sorted(shots.items())
    if len(items) > a.max:       # keep the event shots, thin the grid first
        ev = [x for x in items if x[1] != "grid"]
        gr = [x for x in items if x[1] == "grid"]
        keep = a.max - len(ev)
        if keep < 0:
            ev = ev[:a.max]
            gr = []
        else:
            stride = max(1, len(gr) // max(keep, 1))
            gr = gr[::stride][:keep]
        items = sorted(ev + gr)
    if a.peek:
        from tsp import app, BRD, board, player
        exprs = [app(0x4b8), BRD, board(0x64), board(0x1c4), player(0xf4), player(0x110)]
    for u, what in items:
        t.shot(u, f"{u:06d}_{what}")
        if a.peek:
            t.peek(u, *exprs)
    last = max(rows) if rows else t.last()
    t.write(quit_at=a.quit or last)
    kinds = {}
    for u, what in items:
        kinds[what.split("_")[0]] = kinds.get(what.split("_")[0], 0) + 1
    print(f"{a.name}: {len(t.steps)} inputs, {len(items)} shots, to update {a.quit or last}; {kinds}")


if __name__ == "__main__":
    main()
