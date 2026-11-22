#!/usr/bin/env python3
"""Does every game follow the game speed? (tools/check.sh speed; docs/WEB_PLAYER.md, "Game speed")

Each finished game's native exe is run in real time (no -fast), briefly, once at 0.5x and once at 2x, with
-speed=<x> -speedprobe=<ms> (framework/SexyAppBase.cpp, SpeedProbeStep): after the game has loaded and a short warm-up,
the exe counts its updates and its game clock over <ms> of real time, logs one "SPEEDPROBE speed= updates= real_ms=
clock_ms=" line and quits. A game follows the speed when its updates per real second and its game clock both scale
with it: the 2x run's updates per second must be 2.5 to 6 times the 0.5x run's (4 when it follows), and the clock must
run at about the speed in each (game ms per real ms within 25% of 0.5 and of 2). The ratio is printed for every game.
A game that does not print a line before the timeout (no SexyAppBase, or never loaded) is "no probe". KNOWN lists games
expected not to follow (none now); they are named in the summary and do not fail the suite.

A game that fails gets a second try once the others are done, run alone. A machine busy with other work (sandboxes,
other sessions' games) can hold a game below even its 0.5x rate, so that both runs make the same 30 or so updates a
second, or slow its loading past the timeout; on 2026-11-22 Tip Top and Heavy Weapon failed so and passed on a rerun.
Load only ever slows a run, so the second try keeps, at each speed, whichever of the two tries' runs made more updates
a second, and judges the pair it keeps: Atomica's first try held its 2x run to 77 a second and its second its 0.5x run
to 23, while the other run of each made its rate (2026-11-22). A game that ignores the speed makes the same rate at
both speeds in every run, so the best of each is still about equal and it fails. The second try's line says so and
gives the first's.

    python -I tools/speedcheck.py [--out <dir>] [--jobs <n>] [--ms <n>] [<game>...]

Every exe runs from build/native with its assets in assets/<game>, a fresh user folder under <dir>, and
check.sh has put the whole run under rundesk (a private desktop, silent audio, below-normal priority). At most
--jobs (3 by default) run at once; each launch waits for room under the machine's limit (tools/gamecount.py).
Exit status: 0 when every game that is not known follows the speed, 1 otherwise."""
import argparse, concurrent.futures, os, re, shutil, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SPEEDS = (0.5, 2.0)
RATIO_LOW, RATIO_HIGH = 2.5, 6.0     # updates per second at 2x over 0.5x: 4 when the game follows, near 1 when it ignores the speed;
                                     # wide so that a loaded machine, which cannot make every update of a fast run, does not fail one
CLOCK_TOLERANCE = 0.25               # game ms per real ms, against the speed
LOAD_TIMEOUT_S = 60                  # a game that has not printed its line by then is "no probe"
KNOWN = {}  # game -> why it is known not to follow (none: Bonnie's Bookstore and Ning Po follow)

# game folder -> exe name, where it differs from the folder's name
EXES = {}
GAMES = ["bejeweled", "bookworm", "bigmoney", "noahsark", "atomica", "alchemy", "sevenseas", "pixelus", "bookwormadv",
         "bookwormadv2", "tiptop", "mummymaze", "ningpo", "wordharmony", "cosmicbugs", "waterbugs", "rocketmania",
         "dynomite", "mahjongjapan", "mahjongchina", "bonnies", "bejeweledtwist", "bejeweled2", "astropop", "typershark",
         "venice", "heavyweapon", "hammerheads", "pizzafrenzy"]
LINE = re.compile(r"SPEEDPROBE speed=(\S+) updates=(-?\d+) real_ms=(\d+) clock_ms=(\d+)")


def wait_for_room():
    while subprocess.call([sys.executable, "-I", os.path.join(ROOT, "tools", "gamecount.py"), "--limit"],
                          stdout=subprocess.DEVNULL) != 0:
        time.sleep(2)


def probe(game, speed, ms, out):
    """One run: returns (updates per second, game ms per real ms) or a string saying why not."""
    exe = os.path.join(ROOT, "build", "native", EXES.get(game, game) + ".exe")
    if not os.path.isfile(exe):
        return "no exe"
    user = os.path.join(out, game, "user-%g" % speed)
    shutil.rmtree(user, ignore_errors=True)
    os.makedirs(user)
    cmd = [exe, "-gamedir=" + os.path.join(ROOT, "assets", game), "-userdir=" + user, "-speed=%g" % speed,
           "-speedprobe=%d" % ms]
    wait_for_room()
    log = os.path.join(out, game, "run-%g.txt" % speed)
    with open(log, "w") as f:
        try:
            subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, timeout=LOAD_TIMEOUT_S + ms / 1000.0, cwd=ROOT)
        except subprocess.TimeoutExpired:
            pass  # run() has killed the exe
    text = open(log, errors="replace").read()
    m = LINE.search(text)
    if not m:
        return "no probe"
    updates, real_ms, clock_ms = int(m.group(2)), int(m.group(3)), int(m.group(4))
    return updates * 1000.0 / real_ms, clock_ms / float(real_ms)


def better(a, b):
    """Of two runs at one speed, the one that made more updates a second (a string, no probe, loses to a measured run)."""
    if isinstance(a, str):
        return b
    if isinstance(b, str):
        return a
    return b if b[0] > a[0] else a


def check_game(game, ms, out, prior=None):
    """(game, state, detail, runs); with prior, the runs of an earlier try, each speed keeps the better of its two runs."""
    os.makedirs(os.path.join(out, game), exist_ok=True)
    if not os.path.isdir(os.path.join(ROOT, "assets", game)):
        return game, "skip", "no assets/%s" % game, []
    runs = [probe(game, s, ms, out) for s in SPEEDS]
    if prior:
        runs = [better(a, b) for a, b in zip(prior, runs)]
    for r in runs:
        if isinstance(r, str):
            return game, "none", r, runs
    (ups_slow, clk_slow), (ups_fast, clk_fast) = runs
    ratio = ups_fast / ups_slow if ups_slow > 0 else 0
    detail = "updates/s %.0f -> %.0f (x%.2f), clock x%.2f -> x%.2f" % (ups_slow, ups_fast, ratio, clk_slow, clk_fast)
    ok = (RATIO_LOW <= ratio <= RATIO_HIGH and abs(clk_slow / SPEEDS[0] - 1) <= CLOCK_TOLERANCE
          and abs(clk_fast / SPEEDS[1] - 1) <= CLOCK_TOLERANCE)
    return game, "follows" if ok else "DOES NOT FOLLOW", detail, runs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "build", "logs", "check", "speed"))
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--ms", type=int, default=2500, help="real milliseconds each run counts over")
    ap.add_argument("games", nargs="*")
    a = ap.parse_args()
    out = os.path.abspath(a.out)
    os.makedirs(out, exist_ok=True)
    games = a.games or GAMES
    jobs = max(1, min(a.jobs, 3))
    results, runs = [], {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        for g, state, detail, kept in pool.map(lambda g: check_game(g, a.ms, out), games):
            results.append((g, state, detail))
            runs[g] = kept
            print("%-15s %-16s %s" % results[-1], flush=True)
    for i, (game, state, detail) in enumerate(results):
        if state in ("DOES NOT FOLLOW", "none") and game not in KNOWN:
            g, again, again_detail, _ = check_game(game, a.ms, out, prior=runs[game])
            results[i] = (g, again, "%s (second try, the better run at each speed; the first: %s, %s)" % (again_detail, state, detail))
            print("%-15s %-16s %s" % results[i], flush=True)
    bad = [g for g, s, d in results if s in ("DOES NOT FOLLOW", "none") and g not in KNOWN]
    known = [g for g, s, d in results if s in ("DOES NOT FOLLOW", "none") and g in KNOWN]
    ok = sum(1 for g, s, d in results if s == "follows")
    skipped = [g for g, s, d in results if s == "skip"]
    line = "speed: %d of %d games follow the speed" % (ok, len(results) - len(skipped))
    if known:
        line += "; known not to: " + ", ".join(known)
    if skipped:
        line += "; skipped (no assets): " + ", ".join(skipped)
    if bad:
        line += "; DO NOT FOLLOW: " + ", ".join(bad)
    print(line)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
