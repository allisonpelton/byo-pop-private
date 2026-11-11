#!/usr/bin/env python3
"""Tour P: what the coverage check of 2026-11-11 found that the other tours never did (docs/COVERAGE.md; GAME_NOTES, "Coverage check").
The first player's name confirmed with Return in the name box instead of the OK click; ten players; a player picked from the "Who are you?"
list with its OK button; a player deleted (the delete dialog, "Are you sure?" with No and then Yes); the typing tutor with Next Lesson and
Previous Lesson, the resting mouse (tooltips) and Back To Menu's question answered No and then Yes; then an Easy adventure whose hint box is closed with the space bar, the window's focus
lost and regained on the board, and the pause.

    python tour_more.py <name>

Positions come from the port's own pictures (the delete dialog: rows 19 apart from y = 185, OK (235, 402), CANCEL (378, 402); its question
box: YES (248, 293), NO (392, 293); the tutor: Prev (492, 396), Next (611, 396), Back To Menu (535, 460)).
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour
from tour_users import NOTYOU, ROW0, ROWH, OK_X, ok_y, NAMES

DEL_ROW0, DEL_ROWH = 185, 19


def build(name):
    t = Tour(name)
    s = t.shot
    t.type(700, "Tester")
    s(750, "newuser_typed")
    t.key(760, "enter")                      # the Enter key in the edit box is the OK path (NewUserDialog::EditWidgetText)
    s(780, "assess_after_enter")
    t.click(840, 392, 341)
    s(900, "menu_one_user")
    u = 1000
    for i, nm in enumerate(NAMES):
        rows = 2 + i
        t.click(u, *NOTYOU)
        t.click(u + 30, 320, ROW0)           # the create line
        t.click(u + 40, OK_X, ok_y(rows))
        t.type(u + 60, nm)
        t.click(u + 80, 347, 228)
        t.click(u + 100, 392, 341)
        u += 160
    s(u, "menu_ten_users")
    # a player picked: Bob, the third line (no create line with ten players)
    t.click(u + 10, *NOTYOU)
    t.click(u + 30, 320, ROW0 + ROWH * 2)
    s(u + 34, "select_bob")
    t.click(u + 40, OK_X, ok_y(11))
    s(u + 60, "menu_bob")
    u += 100
    # delete Carol through the delete line: the delete dialog, the question with No, again, and Yes
    t.click(u, *NOTYOU)
    t.click(u + 20, 320, ROW0 + ROWH * 10)
    t.click(u + 30, OK_X, ok_y(11))
    s(u + 40, "delete_dialog")
    t.click(u + 50, 320, DEL_ROW0 + DEL_ROWH * 3)
    s(u + 54, "delete_carol_selected")
    t.click(u + 60, 235, 402)
    s(u + 70, "delete_question")
    t.click(u + 80, 392, 293)                # NO
    s(u + 90, "delete_question_no")
    t.click(u + 100, 235, 402)
    t.click(u + 110, 248, 293)               # YES
    s(u + 120, "delete_yes_1")
    s(u + 150, "delete_yes_2")
    t.click(u + 160, 378, 402)               # CANCEL of the (smaller) delete dialog, or what lies there
    s(u + 180, "after_cancel")
    u += 200
    t.click(u, *NOTYOU)                      # the list has nine players now: the new-user line, then the names
    s(u + 6, "select_nine")
    t.key(u + 10, "esc")
    s(u + 20, "select_nine_esc")
    t.key(u + 30, "enter")
    s(u + 40, "select_nine_enter")
    t.click(u + 60, 378, ok_y(10) - 5)       # CANCEL (nine players and the create line: ten rows)
    s(u + 80, "select_closed")
    u += 120
    # the typing tutor: Next Lesson twice, Previous Lesson, the resting mouse over a button
    t.click(u, 510, 290)
    s(u + 30, "tutor_0")
    t.click(u + 40, 611, 396)
    t.click(u + 50, 611, 396)
    s(u + 60, "tutor_2")
    t.click(u + 70, 492, 396)
    s(u + 80, "tutor_1")
    t.click(u + 90, 492, 396)
    t.click(u + 100, 492, 396)
    s(u + 110, "tutor_prev_at_0")
    t.move(u + 120, 535, 460)
    s(u + 124, "tutor_hover_back")
    s(u + 300, "tutor_hover_back_rest")
    t.click(u + 320, 535, 460)               # Back To Menu: "Are you sure you want to quit the typing tutor?"
    s(u + 340, "tutor_quit_question")
    t.click(u + 350, 392, 293)               # NO
    s(u + 370, "tutor_quit_no")
    t.click(u + 380, 535, 460)
    t.click(u + 400, 248, 293)               # YES
    s(u + 440, "menu_again")
    t.move(u + 460, 410, 383)                # the Hall of Fame button
    s(u + 620, "menu_rest")
    u += 640
    # an Easy adventure: the hint box closed with the space bar
    t.click(u, 510, 70)
    t.click(u + 20, 320, 163)
    s(u + 190, "hint_box")                  # the box is up from about update 150 on
    t.type(u + 200, " ")                     # the space as the typed character (the port's `key 32` sends no character for it)
    s(u + 230, "after_space")
    t.raw(u + 400, "focus lose", "focus lose")
    s(u + 420, "focus_lost")
    t.raw(u + 440, "focus regain", "focus regain")
    s(u + 460, "focus_back")
    t.move(u + 480, 320, 460)                # the status bar, at rest
    s(u + 700, "bar_rest")
    t.write(quit_at=u + 760)
    return t


if __name__ == "__main__":
    build(sys.argv[1])
