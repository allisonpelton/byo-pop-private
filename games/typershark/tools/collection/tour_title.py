"""Tour of the title screen, for the Collection check (GAME_NOTES.md "The PopCap Game Collection version"): the port's loading ends
before its first update in a -fast run, so the title screen is never drawn there; the test command `goto title` shows it.

    python tour_title.py <name>      writes build/ts-p2/<name>/port.txt (shots title_12, title_14 and title_20)
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "phase2"))
from p2lib import Tour

t = Tour(sys.argv[1])
t.raw(10, "", "goto title")
for u in (12, 14, 20):
    t.shot(u, "title_%02d" % u)
t.write(quit_at=30)
