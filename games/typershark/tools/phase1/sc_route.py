"""Scenario (P1b): Adventure on a difficulty (DIFF 0 Easy, 1 Normal, 2 Hard, 3 Expert, 4 X-Treme), the port's bot sails the route that goes by
the sites in WANT (a '+' list, e.g. WANT=6+9+21+26+27+32+33+34+35; at a fork the first wanted chest on offer is taken, else the first
chest) and plays every level, a letter every 3 updates and at most 3 streak words a level. TYPIST replaces the bot's options. At the
end of the adventure WIN=yes|no clicks the CONGRATULATIONS! dialog's button. HI defaults to BASE+90000; LO..HI is read every STEP
(the command line's --step) unless RANGES/WLVL give a fine window around the levels in WLVL (a '+' list of level numbers)."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tsp import *
from scen_menu import menu_steps, BASE

DIFF = int(os.environ.get("DIFF", "0"))
STEPS = menu_steps(difficulty_y=163 + 45 * DIFF)
WANT = os.environ.get("WANT", "")
_t = "every=3;streak=3"
if WANT:
    _t += ";want=" + WANT
if os.environ.get("WIN"):
    _t += ";win=" + os.environ["WIN"]
TYPIST = os.environ.get("TYPIST", _t)
LO = int(os.environ.get("LO", BASE + 92))
HI = int(os.environ.get("HI", BASE + 90000))
if os.environ.get("WLVL"):
    WINDOW = {'lvl': [int(x) for x in os.environ["WLVL"].split("+")], 'pad': 300, 'coarse': int(os.environ.get("COARSE", "25"))}
