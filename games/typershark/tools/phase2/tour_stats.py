#!/usr/bin/env python3
"""Tour T: the statistics screen with a lesson played. The typing tutor's lesson 18 from its recording (rec/tut18), the
"view stats?" question at its end, Yes, the statistics screen (tour M shows it empty), each of its three views, Close.

    python tour_stats.py <name>
"""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p2lib import Tour, ROOT
from tour_game import load_inputs

YES = (248, 272)                                    # the "view stats?" box (realplay.py's table)
VIEWS = [(145, 359), (145, 396), (145, 323)]        # the statistics screen's three buttons
CLOSE = (530, 433)


def build(name, end=3440):
    t = Tour(name)
    t.steps = [s for s in load_inputs(os.path.join(ROOT, "build", "ts-p2", "rec", "tut18.in.txt")) if s[0] < end]
    s = t.shot
    u = end + 60
    s(u, "viewstats")
    t.move(u + 10, *YES); s(u + 14, "viewstats_hover_yes")
    t.click(u + 20, *YES)
    for d in (23, 50, 100):
        s(u + d, f"stats_{d}")
    u += 120
    for i, (x, y) in enumerate(VIEWS):
        t.move(u, x, y); s(u + 4, f"stats_hover_{i}")
        t.click(u + 10, x, y); s(u + 14, f"stats_view_{i}_4"); s(u + 50, f"stats_view_{i}_40")
        u += 70
    t.move(u, *CLOSE); s(u + 4, "stats_hover_close")
    t.click(u + 10, *CLOSE); s(u + 14, "stats_closed_4"); s(u + 60, "tutor_after_stats")
    t.write(quit_at=u + 100)


if __name__ == "__main__":
    build(sys.argv[1])
