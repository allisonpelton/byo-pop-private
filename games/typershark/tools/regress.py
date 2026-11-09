#!/usr/bin/env python3
"""Typer Shark's regression check: the port's own play, game by game, against what it played when it was last compared with the original.

    python games/typershark/tools/regress.py                 run every game with build/native/typershark.exe; compare each state log's
                                                              hash with games/typershark/tools/regress_expected.txt and, when
                                                              build/baseline-typershark/typershark.exe exists, with the frozen baseline's
    python games/typershark/tools/regress.py --quick         the first games only (about a minute)
    python games/typershark/tools/regress.py --update        write the expected hashes from the exe's own run (after a change that must change play,
                                                              or a new state log format: say why in the commit)
    python games/typershark/tools/regress.py --exe <path>    another build; --only <tag>[,<tag>] some games; --no-baseline

Each game is a scenario of games/typershark/tools/phase1 (the menu clicks and the port's own typing bot, `-typist`, which plays from the
game's state: no word of the game is stored here) with a seed, run by phase1/cmp.py --hash-only under rundesk with -fast: invisible,
silent, one game process at a time (tools/gamecount.py's room is waited for). The phase-1 comparison against the original (docs/PHASE1.md)
proved that these games play out as the original's do; this check proves that later work (leak fixes, speed-ups, a mod's code, the web
build's sources) leaves them as they were. Exit 0 when every hash matches, 1 otherwise; the last line is the summary.
"""
import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
PHASE1 = os.path.join(HERE, "phase1")
EXPECTED = os.path.join(HERE, "regress_expected.txt")

# tag, scenario file, seed, environment (TSBASE moves the menu flow later: the original's loading thread needs it, the port does not care)
ROUTE_ALL = "6+9+21+26+27+32+33+34+35"
GAMES = [
    ("menu-s1", "sc_l1.py", 1, {}, True),
    ("levels1-3-s1", "sc_t2.py", 1, {"TYPIST": "every=4", "HI": "34000"}, True),
    ("levels1-3-s2", "sc_t2.py", 2, {"TYPIST": "every=4", "HI": "34000"}, False),
    ("typos-s1", "sc_t2.py", 1, {"TYPIST": "every=4;typo=7", "HI": "20000"}, False),
    ("idle-s1", "sc_idle.py", 1, {"HI": "15000"}, True),
    ("abyss-s1", "sc_abyss.py", 1, {"HI": "22000"}, True),
    ("abyss-s2", "sc_abyss.py", 2, {"HI": "22000"}, False),
    ("tutor-lesson1-s1", "sc_tutor.py", 1, {"HI": "7000"}, True),
    ("tutor-lesson15-s1", "sc_tutor.py", 1, {"NEXT": "14", "HI": "13000", "TSBASE": "8000"}, False),
    ("bonus-s1", "sc_bonus.py", 1, {"TSBASE": "12000", "HI": "102000"}, False),
    ("hard-clams-s1", "sc_route.py", 1, {"TSBASE": "12000", "DIFF": "2", "WANT": "4+5", "WLVL": "5", "COARSE": "25", "HI": "90000",
                                         "TYPIST": "every=3;typo=5;streak=3;want=4+5;clams=mixed"}, False),
    ("expert-piranhas-s1", "sc_route.py", 1, {"TSBASE": "12000", "DIFF": "3", "WANT": "6+9", "HI": "70000",
                                              "TYPIST": "every=3;typo=3;streak=3;want=6+9"}, False),
    ("adventure-route-s1", "sc_route.py", 1, {"TSBASE": "40000", "WANT": ROUTE_ALL, "WIN": "no", "HI": "200000"}, False),
]


def room():
    r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "gamecount.py"), "--limit", "3"], stdout=subprocess.DEVNULL)
    return r.returncode == 0


def ensure_fixture(exe):
    """build/ts-p1/fixtures/port-clean/users: what a first run of the port leaves (the score tables), so that every game starts from the
    same profile folder. Made once by a short run."""
    fix = os.path.join(ROOT, "build", "ts-p1", "fixtures", "port-clean")
    if os.path.exists(os.path.join(fix, "users", "user.idx")):
        return
    tmp = os.path.join(ROOT, "build", "ts-p1", "fixtures", "port-clean-make")
    subprocess.call(["rm", "-rf", tmp, fix])
    os.makedirs(tmp)
    script = os.path.join(tmp, "q.txt")
    open(script, "w").write("200 quit\n")
    while not room():
        import time
        time.sleep(15)
    subprocess.call([os.path.join(ROOT, "build", "native", "rundesk.exe"), exe, "-fast", "-seed=1", "-userdir=" + os.path.join(tmp, "ud"),
                     "-script=" + script], stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    os.makedirs(fix)
    subprocess.call(["cp", "-r", os.path.join(tmp, "ud", "users"), os.path.join(fix, "users")])
    subprocess.call(["rm", "-rf", tmp])


def run_game(exe, game):
    tag, scen, seed, env, _quick = game
    e = dict(os.environ)
    e.update(env)
    e["TSEXE"] = exe
    p = subprocess.run([sys.executable, os.path.join(PHASE1, "cmp.py"), os.path.join(PHASE1, scen), "--seed", str(seed),
                        "--tag", "regress_" + tag + "_" + os.path.basename(os.path.dirname(exe)), "--hash-only"],
                       env=e, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    for line in p.stdout.splitlines()[::-1]:
        if line.startswith("HASH "):
            return line.split()[1], line
    return None, "no HASH line: " + " | ".join(p.stdout.splitlines()[-3:])


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", default=os.path.join(ROOT, "build", "native", "typershark.exe"))
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--update", action="store_true")
    ap.add_argument("--only")
    ap.add_argument("--no-baseline", action="store_true")
    a = ap.parse_args()
    exe = os.path.abspath(a.exe)
    base = os.path.join(ROOT, "build", "baseline-typershark", "typershark.exe")
    games = GAMES
    if a.quick:
        games = [g for g in games if g[4]]
    if a.only:
        want = a.only.split(",")
        games = [g for g in GAMES if g[0] in want]
    if not os.path.exists(exe):
        print("regress: no exe " + exe)
        return 1
    ensure_fixture(exe)
    expected = {}
    if os.path.exists(EXPECTED):
        for line in open(EXPECTED):
            f = line.split()
            if len(f) >= 2:
                expected[f[0]] = f[1]
    use_base = os.path.exists(base) and not a.no_baseline and not a.update
    bad = []
    new = dict(expected)
    for g in games:
        h, info = run_game(exe, g)
        msg = "%-22s %s" % (g[0], info.replace("HASH ", ""))
        status = "ok"
        if h is None:
            status = "FAILED TO RUN"
        elif a.update:
            new[g[0]] = h
            status = "updated"
        elif g[0] not in expected:
            status = "NO EXPECTED HASH"
        elif expected[g[0]] != h:
            status = "DIFFERS FROM EXPECTED"
        if use_base and h is not None and status == "ok":
            hb, _ = run_game(base, g)
            if hb != h:
                status = "DIFFERS FROM BASELINE"
        print(msg, "->", status, flush=True)
        if status not in ("ok", "updated"):
            bad.append(g[0] + ": " + status)
    if a.update:
        with open(EXPECTED, "w", newline="\n") as f:
            f.write("# tools/regress.py: sha256 (first 32 hex) of each game's state log, from build/native/typershark.exe; see regress.py\n")
            for g in GAMES:
                if g[0] in new:
                    f.write("%s %s\n" % (g[0], new[g[0]]))
        print("regress: wrote " + EXPECTED)
    print("regress: %d games, %s" % (len(games), "all match" if not bad else "FAILED: " + "; ".join(bad)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
