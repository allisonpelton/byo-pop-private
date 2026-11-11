#!/usr/bin/env python3
"""Tour U: the players. A first player (Tester), then nine more through the "Who are you?" dialog (every one with the new-user dialog and the
"assess WPM" question), the list with ten players and its delete line, the delete dialog, a player picked from the list.

    python tour_users.py <name>

The select dialog's top is fixed (131, 34) and its list grows downwards: row k of the list is at y = ROW0 + ROWH * k and the OK and CANCEL
buttons sit below it, at OK0 + ROWH * (rows - 2) (calibrated with the port's pictures).
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour

NOTYOU = (150, 93)
ROW0, ROWH = 107, 20
OK0 = 173            # the OK button with two rows (the create line and one player)
OK_X, CANCEL_X = 233, 378


def ok_y(rows):
    return OK0 + ROWH * (rows - 2)


NAMES = ["Alice", "Bob", "Carol", "Dave", "Erin", "Frank", "Grace", "Heidi", "Ivan"]


def build(name):
    t = Tour(name)
    s = t.shot
    t.type(700, "Tester")
    t.click(760, 347, 228)
    t.click(840, 392, 341)
    s(900, "menu_one_user")
    u = 1000
    for i, nm in enumerate(NAMES):
        n_users = 1 + i                      # players before this one
        rows = n_users + 1                   # the create line and the players
        t.click(u, *NOTYOU)
        s(u + 6, f"select_{n_users}_users")
        if i in (0, 4, 8):
            t.move(u + 10, OK_X, ok_y(rows)); s(u + 14, f"select_{n_users}_hover_ok")
            t.move(u + 16, CANCEL_X, ok_y(rows)); s(u + 20, f"select_{n_users}_hover_cancel")
            t.move(u + 22, 320, ROW0 + ROWH * 1); s(u + 26, f"select_{n_users}_hover_row1")
        t.click(u + 30, 320, ROW0)           # the create line
        s(u + 34, f"select_{n_users}_create_row")
        t.click(u + 40, OK_X, ok_y(rows))
        s(u + 50, f"newuser_{i + 2}")
        t.type(u + 60, nm)
        s(u + 70, f"newuser_{i + 2}_typed")
        t.click(u + 80, 347, 228)
        s(u + 90, f"assess_{i + 2}")
        t.click(u + 100, 392, 341)
        s(u + 130, f"menu_{i + 2}_users")
        u += 160
    # ten players now: the list holds the names and the delete line
    t.click(u, *NOTYOU)
    s(u + 6, "select_ten_users")
    t.move(u + 10, 320, ROW0 + ROWH * 10); s(u + 14, "select_ten_hover_delete_row")
    t.click(u + 20, 320, ROW0 + ROWH * 10)
    s(u + 24, "select_ten_delete_row")
    t.click(u + 30, OK_X, ok_y(11))
    s(u + 40, "delete_dialog_open")
    t.click(u + 50, 320, ROW0 + ROWH * 2)
    s(u + 54, "delete_dialog_row2")
    t.click(u + 60, OK_X, ok_y(11))
    s(u + 70, "delete_dialog_confirm")
    u += 100
    t.write(quit_at=u + 100)
    return t


if __name__ == "__main__":
    build(sys.argv[1] if len(sys.argv) > 1 else "u1")
