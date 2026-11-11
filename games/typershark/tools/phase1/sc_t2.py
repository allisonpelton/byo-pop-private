"""Scenario: Adventure Easy from the menu, the port's bot types a letter every 4 updates for 8000 updates (the first level and on)."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tsp import *
from scen_menu import menu_steps, BASE

STEPS = menu_steps()
TYPIST = os.environ.get("TYPIST", "every=4")
LO = int(os.environ.get("LO", BASE + 92))
HI = int(os.environ.get("HI", BASE + 8000))
