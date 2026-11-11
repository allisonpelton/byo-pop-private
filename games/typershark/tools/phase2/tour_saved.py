#!/usr/bin/env python3
"""Tour S: a saved adventure game. The route of the whole-adventure recording (rec/rt2) up to the second level, then the player leaves the
game by the QUIT button, the menu's Adventure button asks "continue?" (the saved level and difficulty), the answers No (the difficulty
dialog: a new game) and Yes (the saved level). Also the in-game buttons: pause, options (the dialog over the board), quit.

    python tour_saved.py <name>
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour, ROOT
from tour_game import load_inputs

PAUSE, OPTIONS, QUIT = (420, 460), (470, 460), (587, 460)
YES, NO = (245, 303), (391, 303)


def build(name, upto=14200):
    t = Tour(name)
    t.steps = [s for s in load_inputs(os.path.join(ROOT, "build", "ts-p2", "rec", "rt2.in.txt")) if s[0] < upto]
    s = t.shot
    u = upto
    for d in (0, 40, 120):
        s(u + d, f"board_{d}")
    u += 200
    # the in-game buttons
    t.move(u, *PAUSE); s(u + 4, "hover_pause")
    t.click(u + 10, *PAUSE); s(u + 14, "paused_14"); s(u + 60, "paused_60")
    t.click(u + 80, *PAUSE); s(u + 84, "unpaused")
    u += 120
    t.move(u, *OPTIONS); s(u + 4, "hover_options")
    t.click(u + 10, *OPTIONS); s(u + 14, "game_options_14"); s(u + 40, "game_options_40")
    t.click(u + 60, 320, 421); s(u + 64, "game_options_closed")
    u += 100
    t.move(u, *QUIT); s(u + 4, "hover_quit")
    t.click(u + 10, *QUIT); s(u + 14, "quit_confirm_14"); s(u + 40, "quit_confirm_40")
    t.move(u + 50, *NO); s(u + 54, "quit_hover_no")
    t.move(u + 60, *YES); s(u + 64, "quit_hover_yes")
    t.click(u + 70, *NO); s(u + 74, "quit_no_74")
    t.click(u + 100, *QUIT)
    t.click(u + 130, *YES); s(u + 134, "quit_yes_134"); s(u + 200, "menu_after_quit")
    u += 260
    # the menu's Adventure button now asks about the saved game
    t.click(u, 510, 70)
    for d in (3, 12, 40):
        s(u + d, f"continue_{d}")
    t.move(u + 50, *YES); s(u + 54, "continue_hover_yes")
    t.move(u + 60, *NO); s(u + 64, "continue_hover_no")
    t.click(u + 70, *NO)
    for d in (74, 100):
        s(u + d, f"continue_no_{d}")                       # the difficulty dialog
    t.click(u + 120, 320, 407)                             # its Cancel
    s(u + 140, "continue_no_cancel")
    t.click(u + 160, 510, 70)
    s(u + 180, "continue_again")
    t.click(u + 200, *YES)
    for d in (204, 230, 300, 420, 600):
        s(u + d, f"continue_yes_{d}")
    u += 700
    t.write(quit_at=u)
    return t


if __name__ == "__main__":
    build(sys.argv[1] if len(sys.argv) > 1 else "s1")
