"""Where the original and the port are in time on the level screens (the screens pass): the first level point of `lv1` (level 2), a shot
every 100 updates through its introduction and its game over box (the port's are taken every 10 by a run of its own, and matched)."""
import levels_common

SEED = 1
PROFILE = 'lv1'
CENTRES = levels_common.button_centres(6)
click = levels_common.click

INPUTS = click(1000, 397, 532)
INPUTS += click(2000, 140, 320)                                 # Classic Bash
INPUTS += click(5000, *CENTRES[1])                              # the point of level 2
INPUTS += click(8000, 400, 438)                                 # Go, on the introduction
NAMES = {}
for u in range(5100, 8001, 100):
    NAMES[u] = 'phase_intro_%d' % u
for u in range(8100, 20001, 250):
    NAMES[u] = 'phase_board_%d' % u
SHOTS = sorted(NAMES)
END = 20500
