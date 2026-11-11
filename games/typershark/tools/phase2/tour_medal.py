#!/usr/bin/env python3
"""Tour D: the two screens that draw a stretched medal (GAME_NOTES sections 17 and 18), in both programs at the same updates.

    python tour_medal.py stats <name>                  the statistics screen's adventure panel of a player with a finished adventure
                                                        (mkprofile.py <ngc>: the profile comes from a registry preset and a users
                                                        folder, no play): the menu, the Statistics link, the panel, Close
    python tour_medal.py win <name> <open> [<click>]   the end-of-adventure dialog: the route of the whole-adventure recording
                                                        (rec/rt2) up to the dialog, a shot at every few updates from <open> (the update
                                                        the port's state log shows it first, in the original's numbering) to the button
                                                        the recording presses (<click>, default its update), then the statistics
                                                        screen of the profile that finished the adventure

    python tour_medal.py dialog <name> [<open>]        the same dialog from a saved adventure at the last site (mkprofile.py <ngc> saved:
                                                        its totals are set, no adventure is played): the menu's Adventure, YES to the
                                                        saved game, then the map in a step: the original's board takes the space bar
                                                        for it with a patch of its copy (medal-patch.txt: the space bar's state test, 7
                                                        to 1), and the port has its test command `goto map`, which calls the same
                                                        ReturnToMap; the map's final scroll opens the dialog at <open> (the original's
                                                        update; 868 with these two profiles). Shots through the dialog, the two
                                                        buttons' hover, RETURN HOME, and the statistics screen with the medal

Coordinates are the 640x480 client's (tour_menu.py's table).
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour, ROOT
from tour_game import load_inputs

STATS, CLOSE = (150, 118), (530, 432)
REC = os.path.join(ROOT, "build", "ts-p2", "rec", "rt2.in.txt")


def stats_panel(t, u):
    """the Statistics link, the adventure panel (the first one shown), the other two views, Close; returns the next free update"""
    s = t.shot
    t.move(u, *STATS); s(u + 3, "hover_stats")
    u += 20
    t.click(u, *STATS)
    for k in (1, 3, 8, 30, 60):
        s(u + k, f"stats_{k}")
    u += 100
    t.click(u, 238, 27); s(u + 10, "stats_abyss")
    u += 30
    t.click(u, 370, 27); s(u + 10, "stats_tutor")
    u += 30
    t.click(u, 145, 27); s(u + 10, "stats_adv_again")               # the adventure tab again
    u += 30
    t.move(u, *CLOSE); s(u + 4, "stats_hover_close")
    t.click(u + 10, *CLOSE); s(u + 14, "stats_closed_4"); s(u + 60, "stats_closed_50")
    return u + 100


def build_stats(name):
    t = Tour(name)
    for u in (700, 1000):
        t.shot(u, f"menu_{u}")
    u = stats_panel(t, 1100)
    t.write(quit_at=u)


def build_win(name, opened, click=None):
    t = Tour(name)
    steps = load_inputs(REC)
    # the recording's press of the dialog's button (KEEP GOING) is the last input it holds; the tour presses it itself, later
    # (<click>), so the dialog stays up for the shots in between
    rec_click = max(s[0] for s in steps if s[1] == "click" and s[2] == 417 and s[3] == 434)
    if click is None:
        click = rec_click
    t.steps = [s for s in steps if s[0] < rec_click]
    for u in list(range(opened - 3, opened + 8)) + list(range(opened + 8, click, 25)):
        t.shot(u, f"win_{u:06d}")
    t.shot(click - 1, "win_last")
    t.click(click, 417, 434)
    u = click
    for k in (1, 3, 8, 30):
        t.shot(u + k, f"after_{k}")
    u = stats_panel(t, u + 200)
    t.write(quit_at=u)


def build_dialog(name, opened=868):
    t = Tour(name)
    s = t.shot
    s(700, "menu_700")
    t.click(701, 510, 70)                       # Adventure: the saved game is offered
    s(703, "ask_3"); s(720, "ask_20")
    t.click(721, 245, 303)                      # YES: the board of the saved game, level 35
    s(725, "board_5"); s(740, "board_20")
    t.raw(750, "key 32", "goto map")            # the board's step back to the map (ReturnToMap)
    for u in (751, 752, 755, 760, 780, 800, 820, 840, 860):
        s(u, f"map_{u}")
    for u in range(opened - 4, opened + 9):
        s(u, f"dlg_{u}")
    for u in (opened + 12, opened + 20, opened + 40, opened + 80):
        s(u, f"dlg_{u}")
    u = opened + 120
    t.move(u, 237, 434); s(u + 4, "hover_home")
    t.move(u + 20, 417, 434); s(u + 24, "hover_keep")
    t.move(u + 40, 237, 434)
    t.click(u + 50, 237, 434)                   # RETURN HOME: the menu
    for k in (1, 3, 8, 30):
        s(u + 50 + k, f"home_{k}")
    u = stats_panel(t, u + 250)
    t.write(quit_at=u)


if __name__ == "__main__":
    if sys.argv[1] == "stats":
        build_stats(sys.argv[2])
    elif sys.argv[1] == "dialog":
        build_dialog(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 868)
    else:
        build_win(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]) if len(sys.argv) > 4 else None)
