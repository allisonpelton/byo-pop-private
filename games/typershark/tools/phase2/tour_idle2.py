#!/usr/bin/env python3
"""Tour I: the idle adventure game (tour G's g_idle: the menu clicks, then nothing typed until the lives are gone at about update 21160)
and its game-over box answered with PLAY AGAIN: the app restarts the board (RestartBoard 0x42ee10; the coverage check of 2026-11-11, GAME_NOTES,
"Coverage check"), and the second game's first screens.

    python tour_idle2.py <name>

The menu input is read from g_idle's own script (build/ts-p2/g_idle/orig.txt, which must have been made: tour_game.py g_idle ...).
PLAY AGAIN is at (238, 333) in the port's picture of the box.
"""
import os
import re
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour, ROOT

PLAY_AGAIN = (238, 333)
OVER = 21161                           # the box comes up (g_idle's first game-over shot)


def build(name):
    t = Tour(name)
    for line in open(os.path.join(ROOT, "build", "ts-p2", "g_idle", "orig.txt")):
        m = re.match(r"at (\d+) (text|click) (.*)", line)
        if not m:
            continue
        u, kind, arg = int(m.group(1)), m.group(2), m.group(3).split()
        if kind == "text":
            t.steps.append((u, "text1", arg[0]))
        else:
            t.steps.append((u, "click", int(arg[0]), int(arg[1]), None))
    s = t.shot
    s(OVER + 120, "gameover")
    t.move(OVER + 140, *PLAY_AGAIN); s(OVER + 144, "gameover_hover")
    t.click(OVER + 150, *PLAY_AGAIN)
    for d in (154, 170, 220, 330, 500, 800):
        s(OVER + d, f"again_{d}")
    t.write(quit_at=OVER + 900)
    return t


if __name__ == "__main__":
    build(sys.argv[1])
