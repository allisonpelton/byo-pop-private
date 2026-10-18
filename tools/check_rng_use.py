#!/usr/bin/env python3
"""Checks rule 1 of docs/CODE_MODS_DESIGN.md 6.1 (randomness) on the game code, src/game:

  1. No C library or standard-library randomness: rand(, srand(, <random>, std::shuffle,
     std::random_shuffle, std::mt19937, std::random_device. (The autoplay bot's std::mt19937 is test code
     in src/testhooks.)
  2. The global stream is never reseeded or reached around Sexy::Rand: no SRand( and no gMTRand.
  3. Inside Board (Board.h, Board*.cpp) every draw goes through Board::Rand(): no Sexy::Rand, ::Rand(
     or MTRand Next() except in Board::Rand's own definition. A plain Rand() there is Board::Rand (class
     scope hides Sexy::Rand), so the translated code keeps its text.
  4. Outside Board only the documented global draws: the HighScoreWidget constructor's tip roll
     (Rand() % 9, GAME_NOTES 3.16) and WinDMApp::ChooseBoardRng's per-game seed (6.2).

  tools/check_rng_use.py [<dir>]   prints every Rand() site and any violation (in src/game, or <dir>);
                                   exit status 1 on a violation

Comments are ignored. Each allowed site must occur exactly as often as listed, so a second draw in the
same file is reported too.
"""

import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
GAME = os.path.join(ROOT, "src", "game")

FORBIDDEN = [
    (re.compile(r"(?<![\w:.>])(?:std::)?rand\s*\("), "rand( (C library)"),
    (re.compile(r"(?<![\w.>])(?:std::)?srand\s*\("), "srand( (C library)"),
    (re.compile(r"#\s*include\s*<random>"), "<random>"),
    (re.compile(r"\bstd::(?:shuffle|random_shuffle|mt19937\w*|random_device|minstd_rand\w*|default_random_engine)\b"),
     "standard-library randomness"),
    (re.compile(r"\bSRand\s*\("), "SRand( (the global stream is never reseeded)"),
    (re.compile(r"\bgMTRand\b"), "gMTRand (only Sexy::Rand reaches the global stream)"),
]
BOARD_FORBIDDEN = [
    (re.compile(r"\bSexy::Rand\b|(?<![\w:])::Rand\s*\("), "Sexy::Rand inside Board (use Board::Rand)"),
    (re.compile(r"(?:->|\.)\s*Next(?:NoAssert)?\s*\("), "MTRand draw inside Board (use Board::Rand)"),
]
RAND_CALL = re.compile(r"\bRand\s*\(")

# (file, pattern, count, why): the draws allowed by name
ALLOWED = [
    ("Board.h", re.compile(r"int\s+Rand\(\)\s*\{.*mOwnRng->Next\(\).*Sexy::Rand\(\)"), 1,
     "Board::Rand itself: the board's own MTRand or the global stream"),
    ("HighScoreWidget.cpp", re.compile(r"int aTipNum = Rand\(\) % 9;"), 1,
     "the tip roll in the HighScoreWidget constructor (global stream, GAME_NOTES 3.16)"),
    ("WinDMApp.cpp", re.compile(r"uint32_t aSeed = \(uint32_t\) Sexy::Rand\(\);"), 1,
     "WinDMApp::ChooseBoardRng: a per-game seed drawn from the global stream (6.2)"),
]


def strip_comments(text):
    """The source with // and /* */ comments blanked out (line numbers kept) and string literals emptied"""
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("\n" * text.count("\n", i, j))
            i = j
        elif c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            out.append(c + c)
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out)


def is_board_file(name):
    return name == "Board.h" or (name.startswith("Board") and name.endswith(".cpp"))


def main(argv):
    game = argv[1] if len(argv) > 1 else GAME
    violations = []
    sites = []
    allowed_hits = {i: [] for i in range(len(ALLOWED))}
    files = sorted(f for f in os.listdir(game) if f.endswith((".cpp", ".h")))
    for name in files:
        with open(os.path.join(game, name), encoding="utf-8", errors="replace") as f:
            raw = f.read()
        raw_lines = raw.split("\n")
        code_lines = strip_comments(raw).split("\n")
        board = is_board_file(name)
        for number, line in enumerate(code_lines, 1):
            where = "src/game/%s:%d" % (name, number)
            source = raw_lines[number - 1].strip() if number - 1 < len(raw_lines) else ""
            checks = FORBIDDEN + (BOARD_FORBIDDEN if board else [])
            allowed = [i for i, (f, pattern, _, _) in enumerate(ALLOWED) if f == name and pattern.search(line)]
            for i in allowed:
                allowed_hits[i].append(where)
            for pattern, what in checks:
                if pattern.search(line) and not allowed:
                    violations.append("%s: %s: %s" % (where, what, source))
            if RAND_CALL.search(line):
                if allowed:
                    sites.append("%s  allowed      %s" % (where, source))
                elif board:
                    sites.append("%s  Board::Rand  %s" % (where, source))
                else:
                    violations.append("%s: a Rand() draw outside Board that is not one of the documented ones: %s"
                                      % (where, source))

    for i, (name, _, count, why) in enumerate(ALLOWED):
        hits = allowed_hits[i]
        if len(hits) != count:
            violations.append("src/game/%s: expected %d allowed site(s) (%s), found %d: %s"
                              % (name, count, why, len(hits), ", ".join(hits) or "none"))

    for s in sites:
        print(s)
    for v in violations:
        print("VIOLATION " + v)
    board_sites = sum(1 for s in sites if "Board::Rand" in s)
    print("rng rules (docs/CODE_MODS_DESIGN.md 6.1 rule 1): %d files, %d Rand() lines in Board (Board::Rand), %d allowed "
          "sites, %d violations: %s" % (len(files), board_sites, len(sites) - board_sites, len(violations),
                                        "OK" if not violations else "FAIL"))
    return 1 if violations else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
