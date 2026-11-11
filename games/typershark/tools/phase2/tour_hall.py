#!/usr/bin/env python3
"""Tour H: an abyss game to its game over, the Hall of Fame with the entry it made, and the abyss button's "continue?" list.

    python tour_hall.py <name>

The input is the abyss recording (rec/abyss2: a new player, the menu, the abyss, typed to 640 feet, then nothing: the diver is
eaten near update 24000 at 688 feet). Then the game-over box and its two buttons, MAIN MENU, the Hall of Fame (now with a line of
the player's), its close button, the Abyss button again (the profile has a deepest depth above 400 feet, so the game asks where to
start), each line of the list under the pointer, the second line and OK, and the board at that depth.
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour, ROOT
from tour_game import load_inputs
from tsp import app, BRD, board, player

AGAIN, MENU = (238, 374), (394, 374)        # the abyss game-over box (realplay.py's table)
HALL, HALL_CLOSE = (400, 380), (320, 457)   # the menu's Hall of Fame button and the screen's close (tour_menu.py)
ABYSS = (510, 165)
LIST_OK = (235, 221)
EXPRS = [app(0x4b8), BRD, board(0x64), board(0x1c4), player(0xf4), player(0x110)]


def line(i):
    return (285, 115 + 19 * i)


def build(name, over=24300):
    t = Tour(name)
    t.steps = [s for s in load_inputs(os.path.join(ROOT, "build", "ts-p2", "rec", "abyss2.in.txt")) if s[0] < 23000]
    s = t.shot
    u = over
    s(u, "gameover_0"); s(u + 40, "gameover_40")
    t.move(u + 50, *AGAIN); s(u + 54, "gameover_hover_again")
    t.move(u + 60, *MENU); s(u + 64, "gameover_hover_menu")
    t.click(u + 70, *MENU); s(u + 74, "gameover_menu_4"); s(u + 130, "menu_after_abyss")
    u += 160
    t.click(u, *HALL)
    for d in (3, 30, 80):
        s(u + d, f"hall_{d}")
    t.click(u + 100, *HALL_CLOSE); s(u + 130, "hall_closed")
    u += 160
    t.click(u, *ABYSS)
    for d in (3, 12, 40):
        s(u + d, f"abyss_list_{d}")
    for i in range(4):
        t.move(u + 50 + 10 * i, *line(i)); s(u + 54 + 10 * i, f"abyss_list_hover_{i}")
    t.click(u + 100, *line(1)); s(u + 104, "abyss_list_line1")
    t.move(u + 110, *LIST_OK); s(u + 114, "abyss_list_hover_ok")
    t.click(u + 120, *LIST_OK)
    for d in (124, 160, 300, 600, 900):
        s(u + d, f"abyss_from_depth_{d}"); t.peek(u + d, *EXPRS)
    t.write(quit_at=u + 1000)


if __name__ == "__main__":
    build(sys.argv[1])
