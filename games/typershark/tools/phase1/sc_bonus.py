"""Scenario (P1b): Adventure Easy from the menu; the port's bot sails the route 0,1,2,3,4,5 (WANT) to the treasure level of site 5 (a
bonus level: the clams), plays it and goes on. TYPIST (default: a letter every 3 updates, at most 3 streak words a level, WANT=4+5)
may be replaced, e.g. TYPIST="every=3;want=4+5;streak=3;clams=mixed". The comparison range LO..HI defaults to the whole run."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tsp import *
from scen_menu import menu_steps, BASE

STEPS = menu_steps()
TYPIST = os.environ.get("TYPIST", "every=3;want=4+5;streak=3")
LO = int(os.environ.get("LO", BASE + 92))
HI = int(os.environ.get("HI", BASE + 90000))
# a fine window (every update) around the bonus level, the rest every COARSE updates; the window comes from the recorded run
WINDOW = {'lvl': [int(x) for x in os.environ.get("WLVL", "5").split("+")], 'pad': 300, 'coarse': int(os.environ.get("COARSE", "25"))}
