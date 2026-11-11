#!/usr/bin/env python3
"""Tour Q: the typing test of the tutor (lesson 0) typed to its end from the port's recording (rec/tut0, made by
`rec.py tut0 "every=3" 6000 --tutor --prev 1`): the "Test Completed!" message at its end (the message dialog 0x41bd80, with its
"Return to Main Menu" button), the button, the menu after it (coverage check of 2026-11-11, GAME_NOTES, "Coverage check").

    python tour_test.py <name>

The recording's last typed character is at update 2280 of the port's script (the test is over there: the state log's tst=2).
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour, ROOT
from tour_game import load_inputs

END = 2280
BUTTON = (320, 350)                     # the message box's one button, from the port's picture of the box


def build(name):
    t = Tour(name)
    t.steps = [s for s in load_inputs(os.path.join(ROOT, "build", "ts-p2", "rec", "tut0.in.txt")) if s[0] <= END + 4]
    s = t.shot
    u = END
    s(u - 40, "typing_test")
    s(u + 20, "completed_20")
    s(u + 60, "completed_60")
    t.move(u + 80, *BUTTON); s(u + 84, "completed_hover")
    t.click(u + 90, *BUTTON); s(u + 94, "after_button_4"); s(u + 140, "after_button_50")
    t.write(quit_at=u + 200)
    return t


if __name__ == "__main__":
    build(sys.argv[1])
