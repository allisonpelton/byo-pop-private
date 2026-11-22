#!/usr/bin/env python3
"""Every screen, listed: checks each finished game's games/<game>/docs/SCREENS.md (docs/SHOTDIFF.md, "Every screen,
listed") against the game's newest shotdiff run.

    python -I tools/screencheck.py [--game <game>]... [--root <repo>]

A SCREENS.md is a markdown table with the columns Screen | Reached by | Shots | Note. A row fails when its Shots cell is
empty and its Note gives no reason, or when a shot it names is not a pair of the newest run (build/shotreview/<game>/
<run>/summary.json, the run with the newest summary.json) that matches, or that differs and is marked "fine" in the
run's marks.json or explained in the row's Note. One line per game, then a summary line; exit 1 on any failure.
"""
import argparse
import json
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WEB_CMAKE = os.path.join("cmake", "ByopopWeb.cmake")
FINISHED_LIST = "BYOPOP_WEB_FINISHED"
SCREENS_FILE = os.path.join("docs", "SCREENS.md")
REVIEW_DIR = os.path.join("build", "shotreview")
SUMMARY_FILE = "summary.json"
MARKS_FILE = "marks.json"

# A game without a SCREENS.md is reported as "no list yet" and is not a failure until this date (YYYY-MM-DD), when the
# lists become required. Unset (None) for now: the coordinator sets it once every game has its list.
LIST_REQUIRED_FROM = None

# A Note starting with one of these gives the reason a row has no shots.
REASON_PREFIXES = ("unreachable", "rare", "none:")
EXPLAINED_WORD = "explained"       # in a Note: the difference of a differing shot is explained
FINE_MARK = "fine"                 # marks.json: the user accepted the difference
STATUS_MATCH = "match"
STATUS_DIFFER = "differ"
DIMENSION_PREFIX = re.compile(r"^(2D|3D):", re.IGNORECASE)
SHOT_SEPARATORS = re.compile(r"[,\s]+")
SHOT_EXTENSIONS = (".png", ".gif")
HEADER_CELLS = ("screen", "reached by", "shots", "note")
SEPARATOR_CELL = re.compile(r":?-{2,}:?")


def finished_games(root):
    """The game names in cmake/ByopopWeb.cmake's BYOPOP_WEB_FINISHED list."""
    with open(os.path.join(root, WEB_CMAKE), encoding="utf-8", errors="replace") as f:
        text = f.read()
    found = re.search(r"set\(\s*" + FINISHED_LIST + r"\s+([^)]*)\)", text)
    if not found:
        raise SystemExit("screencheck: no %s list in %s" % (FINISHED_LIST, WEB_CMAKE))
    words = []
    for line in found.group(1).splitlines():
        words.extend(line.split("#")[0].split())
    return words


def split_row(line):
    """The cells of one markdown table line, without the outer pipes."""
    text = line.strip()
    if text.startswith("|"):
        text = text[1:]
    if text.endswith("|"):
        text = text[:-1]
    return [cell.strip() for cell in text.split("|")]


def parse_rows(text):
    """The table's rows as (screen, shots_cell, note) tuples: lines with at least four cells, minus the header and the
    separator line of dashes."""
    rows = []
    for line in text.splitlines():
        if "|" not in line:
            continue
        cells = split_row(line)
        if len(cells) < len(HEADER_CELLS):
            continue
        if all(SEPARATOR_CELL.fullmatch(c) for c in cells):
            continue
        if [c.lower() for c in cells[:len(HEADER_CELLS)]] == list(HEADER_CELLS):
            continue
        rows.append((cells[0], cells[2], "|".join(cells[3:])))
    return rows


def shot_names(cell):
    """The shot names in a Shots cell: separated by commas or spaces, each optionally behind a "2D:" or "3D:" prefix
    (alone, or glued to the name)."""
    names = []
    for word in SHOT_SEPARATORS.split(cell.replace("`", "")):
        word = DIMENSION_PREFIX.sub("", word)
        if word:
            names.append(word)
    return names


def stem(name):
    """A shot's name without its folder and picture extension, so "a/b.png" and "b" are the same shot."""
    base = name.replace("\\", "/").rsplit("/", 1)[-1]
    for ext in SHOT_EXTENSIONS:
        if base.lower().endswith(ext):
            return base[:-len(ext)]
    return base


def has_reason(note):
    return note.strip().lower().startswith(REASON_PREFIXES)


def newest_run(root, game):
    """The run folder under build/shotreview/<game>/ with the newest summary.json, or None."""
    base = os.path.join(root, REVIEW_DIR, game)
    best, best_time = None, -1.0
    if os.path.isdir(base):
        for run in os.listdir(base):
            path = os.path.join(base, run, SUMMARY_FILE)
            if os.path.isfile(path) and os.path.getmtime(path) > best_time:
                best, best_time = os.path.join(base, run), os.path.getmtime(path)
    return best


def load_json(path):
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def run_statuses_and_marks(run):
    """({shot stem: status}, {stems marked fine}) of a run folder."""
    statuses = {}
    for pair in load_json(os.path.join(run, SUMMARY_FILE)).get("pairs", []):
        statuses[stem(str(pair.get("name", "")))] = pair.get("status", "?")
    marks = load_json(os.path.join(run, MARKS_FILE)).get("marks", {})
    fine = {stem(name) for name, m in marks.items() if isinstance(m, dict) and m.get("mark") == FINE_MARK}
    return statuses, fine


def check_game(root, game):
    """Returns (line, failed) for one game: its report line and whether it fails."""
    screens = os.path.join(root, "games", game, SCREENS_FILE)
    if not os.path.isfile(screens):
        if LIST_REQUIRED_FROM:
            return "%s: FAIL no SCREENS.md (required from %s)" % (game, LIST_REQUIRED_FROM), True
        return "%s: no list yet" % game, False
    with open(screens, encoding="utf-8", errors="replace") as f:
        rows = parse_rows(f.read())
    run = newest_run(root, game)
    statuses, fine = run_statuses_and_marks(run) if run else ({}, set())
    problems = []
    shots_checked = 0
    for screen, cell, note in rows:
        names = shot_names(cell)
        if not names:
            if not has_reason(note):
                problems.append("row '%s' has no shots and no reason" % screen)
            continue
        if not run:
            problems.append("no run (row '%s' names shots, %s/%s has no summary.json)" % (screen, REVIEW_DIR, game))
            continue
        for name in names:
            shots_checked += 1
            status = statuses.get(stem(name))
            if status is None:
                problems.append("shot %s (row '%s') is not a pair of the newest run" % (name, screen))
            elif status == STATUS_MATCH:
                continue
            elif status == STATUS_DIFFER and (stem(name) in fine or EXPLAINED_WORD in note.lower()):
                continue
            else:
                problems.append("shot %s (row '%s') is %s" % (name, screen, status))
    if problems:
        return "%s: FAIL %d problems: %s" % (game, len(problems), "; ".join(problems)), True
    return "%s: ok, %d rows, %d shots compared (run %s)" % (
        game, len(rows), shots_checked, os.path.basename(run) if run else "none"), False


def main(argv):
    parser = argparse.ArgumentParser(description="Check every game's SCREENS.md against its newest shotdiff run.")
    parser.add_argument("--game", action="append", default=[], help="a game to check (repeatable); default: every finished game")
    parser.add_argument("--root", default=REPO, help="the repository root")
    args = parser.parse_args(argv)
    games = args.game or finished_games(args.root)
    failed = no_list = 0
    for game in games:
        line, game_failed = check_game(args.root, game)
        print(line)
        failed += 1 if game_failed else 0
        no_list += 1 if line.endswith("no list yet") else 0
    print("screens: %d games, %d failed, %d with no list yet" % (len(games), failed, no_list))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
