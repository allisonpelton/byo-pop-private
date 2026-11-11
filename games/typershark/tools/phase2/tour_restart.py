#!/usr/bin/env python3
"""Tour R: quit, start again, carry on. Two runs of each program, the second from what the first left.

    python tour_restart.py a <name>     run A: the route of the whole-adventure recording (rec/rt2) up to the second level, the QUIT
                                        button and its Yes (the game is saved, the menu comes back), then the program ends
    python tour_restart.py b <name>     run B: the program starts again with the player run A made (no NEW USER dialog), the menu, its
                                        Adventure button, the "continue?" question and its Yes, the saved level going on

The original's run B starts from its own run A: the users folder that run left (runorig.sh keeps it as build/ts-p2/<A>/orig.users;
STAGE=<that folder>) and its registry values (REG=<the export origrun wrote after run A>, logs/registry-after-<n>.reg of the run's
folder, copied away before the next run clears it). Both original runs take `--reg-map popcap`, so that the values land under
HKCU in the sandbox as on the host (docs/ORIGRUN.md, "The registry map"). The port's run B starts from run A's user folder
(build/ts-p2/<A>/ud, copied to a fixture folder and named with USERS=).
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour, ROOT
from tour_game import load_inputs
from tsp import app, BRD, board, player

QUIT = (587, 460)
YES, NO = (245, 303), (391, 303)
ADVENTURE = (510, 70)
EXPRS = [app(0x4b8), BRD, board(0x64), board(0x1c4), player(0xf4), player(0x110)]


def run_a(name, upto=14200):
    t = Tour(name)
    t.steps = [s for s in load_inputs(os.path.join(ROOT, "build", "ts-p2", "rec", "rt2.in.txt")) if s[0] < upto]
    u = upto
    t.shot(u, "a_board"); t.peek(u, *EXPRS)
    t.click(u + 10, *QUIT); t.shot(u + 14, "a_quit_confirm")
    t.click(u + 40, *YES); t.shot(u + 44, "a_quit_yes"); t.shot(u + 110, "a_menu")
    t.write(quit_at=u + 200)


def run_b(name):
    t = Tour(name)
    t.steps = []
    for u in (1000, 1040):
        t.shot(u, f"b_menu_{u}")
    u = 1071
    t.move(u - 20, *ADVENTURE); t.shot(u - 10, "b_menu_hover")
    t.click(u, *ADVENTURE)
    for d in (3, 12, 40):
        t.shot(u + d, f"b_continue_{d}")
    t.move(u + 50, *YES); t.shot(u + 54, "b_continue_hover_yes")
    t.click(u + 70, *YES)
    for d in (74, 100, 170, 290, 470, 700, 1000):
        t.shot(u + d, f"b_continue_yes_{d}"); t.peek(u + d, *EXPRS)
    t.write(quit_at=u + 1100)


if __name__ == "__main__":
    {"a": run_a, "b": run_b}[sys.argv[1]](sys.argv[2])
