#!/usr/bin/env python3
"""The mod menu draws no random numbers (docs/MOD_MENU_DESIGN.md 5.4, 7.3): a static check of its sources.

Opening the menu in a game must not move the game's random streams, or a replay, a seed or the oracle would play
differently. At run time the menu's handlers run inside MTAutoDisallowRand (a debug build asserts on a draw) and
tools/modmenutest/run_tests.sh compares -tracehash runs; this check covers the sources: no call of Rand, SRand,
rand, srand or random, and no MTRand of their own, in the menu, its font and the model it edits.

    python tools/modmenutest/check_no_rand.py      exit status 1 if any is found
"""
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
FILES = [
    "framework/ModMenu.cpp", "framework/ModMenu.h", "framework/BuiltinFont.cpp", "framework/BuiltinFont.h",
    "platform/ModSettings.cpp", "platform/ModSettings.h", "platform/ModManager.cpp", "platform/ModManager.h",
    "platform/Relaunch.cpp", "platform/UserFiles.cpp", "platform/Text.cpp",
]
CALL = re.compile(r"\b(Rand|SRand|rand|srand|random|rand_r|drand48)\s*\(")
OWN_RNG = re.compile(r"\bMTRand\b(?!::SetRandAllowed)")


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def main():
    found = 0
    for rel in FILES:
        path = os.path.join(ROOT, rel)
        if not os.path.exists(path):
            print("missing: %s" % rel)
            found += 1
            continue
        with open(path, encoding="utf-8", errors="replace") as f:
            code = strip_comments(f.read())
        for number, line in enumerate(code.split("\n"), 1):
            if line.lstrip().startswith("#include"):
                continue
            if CALL.search(line) or OWN_RNG.search(line.replace("MTAutoDisallowRand", "")):
                print("%s:%d: %s" % (rel, number, line.strip()))
                found += 1
    print("check_no_rand: %d file(s), %d draw(s) found" % (len(FILES), found))
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
