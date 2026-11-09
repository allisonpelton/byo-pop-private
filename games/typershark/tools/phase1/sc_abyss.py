"""Scenario: Abyss mode from the menu (new profile, so no CONTINUE? question), the port's bot types."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tsp import *
from scen_menu import BASE

STEPS = spread_text(BASE, "Tester") + [(BASE + 20, 'click', 347, 228), (BASE + 40, 'click', 392, 341), (BASE + 70, 'click', 510, 165)]
TYPIST = os.environ.get("TYPIST", "every=4")
LO = BASE + 72
HI = int(os.environ.get("HI", BASE + 8000))
