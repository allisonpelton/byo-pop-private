#!/usr/bin/env python3
"""Tour W: a new player, the adventure's first board and the sea moving behind it, for the 3D comparison with the original
(GAME_NOTES.md, "The 3D drawing"): Board::BuildWaterImage draws the sea as strips of triangles only on a 3D accelerated screen.
The menu input is tour G's g_idle (build/ts-p2/g_idle/orig.txt: the new-player box, the WPM question answered No, Adventure, the
difficulty, the hint box closed), 11300 updates earlier.

    python tour_water.py <name>      writes build/ts-p2/<name>/orig.txt and port.txt
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour

U = 700                                   # the name is typed from here


def build(name):
    t = Tour(name)
    t.type(U, "Tester")
    t.click(U + 20, 347, 228)             # OK
    t.click(U + 40, 392, 341)             # No: no WPM assessment
    t.shot(U + 60, "menu_760")
    t.click(U + 70, 510, 70)              # Adventure
    t.click(U + 90, 320, 163)             # the difficulty
    for d in (93, 111, 231, 241):
        t.shot(U + d, f"dialog_{d}")
    t.click(U + 250, 320, 381)            # the hint box closed
    for d in (254, 259, 289, 301, 327):
        t.shot(U + d, f"board_{d}")
    for d in range(400, 1000, 60):
        t.shot(U + d, f"sea_{d}")
    t.write(quit_at=U + 1020)
    return t


if __name__ == "__main__":
    build(sys.argv[1])
