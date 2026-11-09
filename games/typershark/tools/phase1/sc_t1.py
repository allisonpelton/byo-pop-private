"""Scenario: the menu clicks to the first Adventure board (Easy), then the port's bot types (a letter every 6 updates), 2000 updates."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tsp import *
from scen_menu import menu_steps, BASE

STEPS = menu_steps()
TYPIST = "every=6"
LO = BASE + 92
HI = BASE + 2200
