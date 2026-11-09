"""Scenario: the typing tutor from the menu (new profile). The port's bot types the key that is next, one every K updates (TYPIST,
default every=3), until the lesson is finished or the tutor stops accepting a key; the original gets the same keys.
The tutor's state (lesson, state, task, keys typed, errors, clock, the key that is next) is compared, not a board."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tsp import *
from scen_menu import BASE

TUTOR = True
NEXT = int(os.environ.get("NEXT", "0"))          # presses of the Next button before the typing starts (lesson 1 + NEXT)
STEPS = spread_text(BASE, "Tester") + [(BASE + 20, 'click', 347, 228), (BASE + 40, 'click', 392, 341), (BASE + 70, 'click', 510, 290)]
STEPS += [(BASE + 100 + 10 * i, 'click', 612, 395) for i in range(NEXT)]
TYPIST = os.environ.get("TYPIST", "every=3") + ";start=%d" % (BASE + 130 + 10 * NEXT)
LO = BASE + 72
HI = int(os.environ.get("HI", BASE + 3000))
