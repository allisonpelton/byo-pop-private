#!/usr/bin/env python3
"""Unit test of tools/screencheck.py on small fixture folders made in a temporary directory (no game data).

    python -I tools/screenchecktest.py
"""
import json
import os
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import screencheck  # noqa: E402

TABLE_HEAD = "| Screen | Reached by | Shots | Note |\n|---|---|---|---|\n"
failures = 0
checks = 0


def check(name, condition, detail=""):
    global failures, checks
    checks += 1
    if not condition:
        failures += 1
        print("FAIL: %s %s" % (name, detail))


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def make_game(root, game, rows, pairs=None, marks=None, run="run1", mtime=None):
    """A game folder with a SCREENS.md of the given rows; pairs is {name: status} for a shotdiff run, or None for no run."""
    if rows is not None:
        write(os.path.join(root, "games", game, "docs", "SCREENS.md"), "# Screens\n\n" + TABLE_HEAD + rows)
    if pairs is not None:
        folder = os.path.join(root, "build", "shotreview", game, run)
        summary = os.path.join(folder, "summary.json")
        write(summary, json.dumps({"game": game, "run": run, "pairs": [{"name": n, "status": s} for n, s in pairs.items()]}))
        if mtime:
            os.utime(summary, (mtime, mtime))
        if marks is not None:
            write(os.path.join(folder, "marks.json"), json.dumps({"game": game, "run": run, "marks": marks}))


def result(root, game):
    return screencheck.check_game(root, game)


with tempfile.TemporaryDirectory() as root:
    write(os.path.join(root, "cmake", "ByopopWeb.cmake"), "set(BYOPOP_WEB_FINISHED alpha beta\n  gamma)\nset(OTHER x)\n")
    check("finished list read", screencheck.finished_games(root) == ["alpha", "beta", "gamma"])

    line, failed = result(root, "alpha")
    check("no SCREENS.md: reported as no list yet, not a failure", line == "alpha: no list yet" and not failed, line)

    make_game(root, "ok", "| Title | menu | title, menu_hover | |\n| Intro | none | | none: nothing draws it |\n",
              {"title.png": "match", "menu_hover.png": "match", "extra.png": "differ"})
    line, failed = result(root, "ok")
    check("matching shots (with and without extension) and a reasoned row pass", not failed, line)

    make_game(root, "clip", "| Title | launch | title, 3D: title_3d | |\n", {"title/": "match", "title_3d/": "faint"})
    line, failed = result(root, "clip")
    check("a clip's pair, named by its folder with a slash", not failed and "2 shots" in line, line)

    make_game(root, "prefix", "| A | x | 2D: a1 a2, 3D: b1 | |\n| B | x | 2D:a3 | |\n",
              {"a1.png": "match", "a2.png": "match", "b1.png": "match", "a3.png": "match"})
    line, failed = result(root, "prefix")
    check("2D:/3D: prefixes, spaces and commas", not failed and "4 shots" in line, line)

    for reason in ("unreachable: nothing calls it", "Rare: needs a loss", "none: transition"):
        make_game(root, "reason", "| A | x | | %s |\n" % reason, {"a.png": "match"})
        check("reason '%s' accepted" % reason, not result(root, "reason")[1])

    make_game(root, "empty", "| A | x | | draws a dialog |\n", {"a.png": "match"})
    line, failed = result(root, "empty")
    check("empty shots without a reason fail", failed and "no shots and no reason" in line, line)

    make_game(root, "diff", "| A | x | a | |\n", {"a.png": "differ"})
    line, failed = result(root, "diff")
    check("a differing shot fails, naming shot and status", failed and "shot a" in line and "is differ" in line, line)

    make_game(root, "fine", "| A | x | a | |\n", {"a.png": "differ"}, marks={"a.png": {"mark": "fine", "note": ""}})
    check("a differing shot marked fine passes", not result(root, "fine")[1])

    for mark in ("issue", "unsure"):
        make_game(root, "mark", "| A | x | a | |\n", {"a.png": "differ"}, marks={"a.png": {"mark": mark}})
        check("a differing shot marked %s fails" % mark, result(root, "mark")[1])

    make_game(root, "expl", "| A | x | a | the shadow differs, explained in the notes |\n", {"a.png": "differ"})
    check("a differing shot whose Note says explained passes", not result(root, "expl")[1])

    make_game(root, "faint", "| A | x | a | |\n", {"a.png": "faint"})
    check("a faint pair passes", not result(root, "faint")[1])

    make_game(root, "err", "| A | x | a | explained |\n", {"a.png": "error"})
    line, failed = result(root, "err")
    check("an error pair fails even with explained", failed and "is error" in line, line)

    make_game(root, "gone", "| A | x | missing | |\n", {"a.png": "match"})
    line, failed = result(root, "gone")
    check("a shot that is no pair fails", failed and "not a pair" in line, line)

    make_game(root, "norun", "| A | x | a | |\n")
    line, failed = result(root, "norun")
    check("shots but no run fail as no run", failed and "no run" in line, line)

    make_game(root, "norun2", "| A | x | | unreachable: never |\n")
    check("no shots, only reasons: no run needed", not result(root, "norun2")[1])

    now = time.time()
    make_game(root, "newest", "| A | x | a | |\n", {"a.png": "differ"}, run="old", mtime=now - 1000)
    make_game(root, "newest", None, {"a.png": "match"}, run="new", mtime=now)
    check("the newest run counts (a match in the new run, a differ in the old)", not result(root, "newest")[1])
    make_game(root, "newest2", "| A | x | a | |\n", {"a.png": "match"}, run="old", mtime=now - 1000)
    make_game(root, "newest2", None, {"a.png": "differ"}, run="new", mtime=now)
    check("the newest run counts (a differ in the new run)", result(root, "newest2")[1])

    # the whole command: exit status and the summary line
    code = screencheck.main(["--root", root, "--game", "alpha", "--game", "ok"])
    check("main: no list yet and ok exit 0", code == 0)
    code = screencheck.main(["--root", root, "--game", "diff"])
    check("main: a failing game exits 1", code == 1)

    old_required = screencheck.LIST_REQUIRED_FROM
    screencheck.LIST_REQUIRED_FROM = "2027-01-01"
    line, failed = result(root, "alpha")
    check("a required date makes a missing list a failure", failed and "no SCREENS.md" in line, line)
    screencheck.LIST_REQUIRED_FROM = old_required

print("screens test: %d checks, %d failed" % (checks, failures))
sys.exit(1 if failures else 0)
