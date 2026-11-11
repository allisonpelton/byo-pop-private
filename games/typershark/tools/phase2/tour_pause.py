#!/usr/bin/env python3
"""Tour W2: the adventure's board paused once, for the 3D comparison with the original (GAME_NOTES.md, "The 3D drawing"). On the real clock
of a desktop run the moving sprites (sharks, the diver, bubbles) are drawn a fraction of an update apart from the port's, and a board that
has been paused and resumed has run other updates than the port's, so the board is compared in a frame where nothing moves: the Pause button
(420, 460) is clicked at the given update after the hint box is closed, and three shots follow. A run holds one pause; the menu input is tour
W's.

    python tour_pause.py <name> <update>      writes build/ts-p2/<name>/orig.txt and port.txt
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour

U = 700
PAUSE = (420, 460)


def build(name, at):
    t = Tour(name)
    t.type(U, "Tester")
    t.click(U + 20, 347, 228)             # OK
    t.click(U + 40, 392, 341)             # No: no WPM assessment
    t.click(U + 70, 510, 70)              # Adventure
    t.click(U + 90, 320, 163)             # the difficulty
    t.click(U + 250, 320, 381)            # the hint box closed
    base = U + 250 + at
    t.click(base, *PAUSE)
    for d in (4, 30, 80):
        t.shot(base + d, f"paused_{at}_{d}")
    t.write(quit_at=base + 100)
    return t


if __name__ == "__main__":
    build(sys.argv[1], int(sys.argv[2]))
