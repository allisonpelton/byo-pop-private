#!/usr/bin/env python3
"""Tour D: a player whose data file is gone (coverage check of 2026-11-11, GAME_NOTES, "Coverage check"). Two runs of each program.

    python tour_missing.py a <name>     run A: the first player, "Tester" (new-user box, OK, "assess WPM" answered No), the menu, the end
    python tour_missing.py b <name>     run B: the program starts again with the player in the registry and no data file (missing.sh
                                        deletes users/user0.dat between the runs): the error message of the map screen's startup
                                        (0x426440: a new, empty profile of that name is saved), its button, the menu

The message's button is OK at (320, 304), read from the port's picture of the box.
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour

OK = (320, 304)


def run_a(name):
    t = Tour(name)
    t.type(700, "Tester")
    t.click(740, 347, 228)
    t.click(780, 392, 341)
    t.shot(820, "a_menu")
    t.write(quit_at=900)


def run_b(name):
    t = Tour(name)
    for u in (300, 500, 700, 1000):
        t.shot(u, f"b_{u}")
    t.move(1020, *OK); t.shot(1024, "b_hover_ok")
    t.click(1030, *OK)
    for d in (34, 80, 200):
        t.shot(1030 + d, f"b_after_ok_{d}")
    t.write(quit_at=1400)


if __name__ == "__main__":
    {"a": run_a, "b": run_b}[sys.argv[1]](sys.argv[2])
