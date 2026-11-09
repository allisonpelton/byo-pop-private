"""Scenario: the menu clicks to the first Adventure board (Easy), the opening hint dismissed, no typing."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tsp import *
from scen_menu import menu_steps, BASE

STEPS = menu_steps() + [(BASE + 250, 'click', 320, 381)]
LO = BASE + 92        # the board's first update is BASE + 91
HI = BASE + 700
