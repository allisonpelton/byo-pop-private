"""Scenario: Adventure on another difficulty (DIFF 0 Easy .. 4 X-Treme; the dialog's buttons are 45 pixels apart), the port's bot types
a letter every K updates (TYPIST, default every=3)."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tsp import *
from scen_menu import menu_steps, BASE

DIFF = int(os.environ.get("DIFF", "1"))
STEPS = menu_steps(difficulty_y=163 + 45 * DIFF)
TYPIST = os.environ.get("TYPIST", "every=3")
LO = int(os.environ.get("LO", BASE + 92))
HI = int(os.environ.get("HI", BASE + 8000))
