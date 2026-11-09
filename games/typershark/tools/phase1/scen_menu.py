#!/usr/bin/env python3
"""Scenario A: the same menu clicks in the original and the port, up to the first Adventure board (Easy).
    python scen_menu.py <orig script out> <port script out> [explore]"""
import sys
sys.path.insert(0, __file__.rsplit('/', 1)[0] if '/' in __file__ else '.')
from tsp import *

import os
# the update at which the menu flow starts: the original's loading thread runs on the wall clock, so a busy machine needs a later
# start (TSBASE=12000); every update number of a scenario follows from it
BASE = int(os.environ.get("TSBASE", "4000"))


def menu_steps(base=BASE, difficulty_y=163):
    s = spread_text(base, "Tester")
    s += [(base + 20, 'click', 347, 228),        # NEW USER: OK
          (base + 40, 'click', 392, 341),        # ASSESS WPM?: No
          (base + 70, 'click', 510, 70),         # menu: Adventure
          (base + 90, 'click', 320, difficulty_y)]   # DIFFICULTY: Easy
    return s


def scalars():
    return [app(0x4b8), BRD, board(0x64), board(0x1c4), player(0xf4), player(0x110), board(0xdf8), board(0xe00),
            board(0xd78), board(0xd7c), board(0xd88), board(0xd8c), board(0xd98), board(0xd9c), board(0xdc8), board(0xdcc),
            board(0xeb8), ind(board(0xd34), 0x4c), board(0xd48), board(0xd4c), app(0x688), app(0x618), app(0x624)]


if __name__ == '__main__':
    s = menu_steps()
    for u in [BASE - 1, BASE + 19, BASE + 21, BASE + 39, BASE + 41, BASE + 69, BASE + 71, BASE + 89, BASE + 91]:
        s.append((u, 'peek', [app(0x4b8), BRD, app(0x628), app(0x8e8), app(0x618), app(0x624), app(0x688)]))
    for u in range(BASE + 100, BASE + 2500, 10):
        s.append((u, 'peek', scalars()))
    open(sys.argv[1], 'w').write(orig_script(s))
    open(sys.argv[2], 'w').write(port_script(s, [f"{BASE + 2600} quit"]))
