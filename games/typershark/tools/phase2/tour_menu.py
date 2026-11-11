#!/usr/bin/env python3
"""Tour M: the menus, from the first start to the difficulty dialog. Both programs get the same input at the same updates.

    python tour_menu.py <name>      writes build/ts-p2/<name>/orig.txt and port.txt

Coordinates are the 640x480 client's. The first start meets the NEW USER dialog (no profile); the original's loading ends before update
500 in a sandbox run (the port's before update 5), so nothing is clicked before update 600.
"""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour

# menu geometry (MenuScreen.cpp): buttons and the two links
ADV, ABYSS, TUTOR = (450, 80), (450, 190), (450, 290)
HALL, OPTIONS, QUIT = (400, 380), (400, 420), (560, 400)
NOTYOU, STATS = (150, 93), (150, 118)


def build(name):
    t = Tour(name)
    shots = []

    def s(u, n):
        t.shot(u, n)

    # --- the title and the new-user dialog on the first start
    for u in (5, 30, 100, 300):
        s(u, f"title_{u:04d}")
    for u in (600, 601, 603, 610, 625, 650, 680):
        s(u, f"newuser_empty_{u}")
    # the name: a character an update, a shot after each of the first ones, then a few later (the caret)
    t.type(700, "Tester")
    for i in range(7):
        s(700 + i, f"newuser_typed_{i}")
    for u in (720, 735, 750):
        s(u, f"newuser_typed_late_{u}")
    # OK; the question "assess WPM"
    t.click(760, 347, 228)
    for u in (761, 765, 780, 820):
        s(u, f"assess_{u}")
    t.click(840, 392, 341)                        # No
    for u in (841, 845, 860, 900, 1000):
        s(u, f"menu_{u}")
    # --- the menu: the cursor over each button and link (a move, then the shots 3 and 20 updates later)
    u = 1100
    for nm, (x, y) in (("adv", ADV), ("abyss", ABYSS), ("tutor", TUTOR), ("hall", HALL), ("options", OPTIONS), ("quit", QUIT),
                       ("notyou", NOTYOU), ("stats", STATS), ("away", (20, 20))):
        t.move(u, x, y)
        s(u + 3, f"hover_{nm}_3")
        s(u + 20, f"hover_{nm}_20")
        u += 40
    # --- the options dialog
    u += 20
    t.click(u, *OPTIONS)
    for k in (1, 3, 8, 30):
        s(u + k, f"options_open_{k}")
    u += 60
    t.move(u, 170, 195); s(u + 5, "options_hover_fx")
    u += 20
    t.click(u, 170, 195); s(u + 5, "options_fx_off")                # Enable Fx
    u += 20
    t.click(u, 170, 228); s(u + 5, "options_cursors_on")            # Custom Cursors
    u += 20
    # the original's capture (a Windows Sandbox screenshot) holds the cursor the game draws itself (the image cursors that "Custom
    # Cursors" turns on); the port's screenshot command never does, so the option goes off again for the rest of the tour
    t.click(u, 170, 228); s(u + 5, "options_cursors_off")
    u += 20
    t.click(u, 170, 262); s(u + 5, "options_space_on")              # Space bar pauses
    u += 20
    t.click(u, 170, 195); s(u + 5, "options_fx_on")
    u += 20
    # the sliders: a drag of the music slider to the left and of the sound-effects slider to the middle
    t.down(u, 449, 96); t.move(u + 2, 400, 96); t.move(u + 4, 330, 96); t.up(u + 6, 330, 96); s(u + 10, "options_music_330")
    u += 30
    t.down(u, 449, 129); t.move(u + 2, 380, 129); t.up(u + 4, 380, 129); s(u + 8, "options_sfx_380")
    u += 30
    t.move(u, 320, 382); s(u + 4, "options_hover_credits")
    u += 10
    t.click(u, 320, 382)                                              # Credits
    for k in (1, 3, 8, 30):
        s(u + k, f"credits_{k}")
    u += 60
    t.move(u, 300, 394); s(u + 4, "credits_hover_ok")
    u += 10
    t.click(u, 300, 394)
    s(u + 10, "credits_closed")
    u += 30
    t.move(u, 320, 421); s(u + 4, "options_hover_done")
    u += 10
    t.click(u, 320, 421)
    s(u + 3, "options_done_3"); s(u + 30, "options_done_30")
    u += 60
    # --- the hall of fame
    t.click(u, *HALL)
    for k in (1, 3, 8, 30, 120):
        s(u + k, f"hall_{k}")
    u += 160
    t.move(u, 320, 457); s(u + 4, "hall_hover_close")
    u += 10
    t.click(u, 320, 457); s(u + 3, "hall_closed_3"); s(u + 30, "hall_closed_30")
    u += 60
    # --- the statistics: the three tabs
    t.click(u, *STATS)
    for k in (1, 3, 8, 30):
        s(u + k, f"stats_{k}")
    u += 60
    t.click(u, 238, 27); s(u + 10, "stats_abyss")
    u += 30
    t.click(u, 370, 27); s(u + 10, "stats_tutor")
    u += 30
    t.click(u, 530, 432); s(u + 3, "stats_closed_3"); s(u + 30, "stats_closed_30")
    u += 60
    # --- the difficulty dialog (cancelled)
    t.click(u, 510, 70)
    for k in (1, 3, 8, 30):
        s(u + k, f"difficulty_{k}")
    u += 60
    for i, nm in enumerate(("easy", "normal", "hard", "expert", "xtreme")):
        t.move(u, 320, 163 + 45 * i); s(u + 4, f"difficulty_hover_{nm}")
        u += 20
    t.move(u, 320, 407); s(u + 4, "difficulty_hover_cancel")
    u += 10
    t.click(u, 320, 407); s(u + 3, "difficulty_cancelled_3"); s(u + 40, "difficulty_cancelled_40")
    u += 60
    t.write(quit_at=u + 100)
    return t


if __name__ == "__main__":
    build(sys.argv[1] if len(sys.argv) > 1 else "m1")
