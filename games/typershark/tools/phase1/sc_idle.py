"""Scenario: Adventure Easy from the menu, the hint dismissed, then nobody types: the sharks eat every life, the game ends."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tsp import *
from scen_menu import menu_steps, BASE

STEPS = menu_steps() + [(BASE + 250, 'click', 320, 381)]
LO = int(os.environ.get("LO", BASE + 92))
HI = int(os.environ.get("HI", BASE + 6000))
