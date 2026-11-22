"""scr_phase's inputs, with the port's shots every 2 updates around two of the original's, to find the port's offset in time."""
import levels_common

SEED = 1
PROFILE = 'lv1'
CENTRES = levels_common.button_centres(6)
click = levels_common.click

INPUTS = click(1000, 397, 532)
INPUTS += click(2000, 140, 320)
INPUTS += click(5000, *CENTRES[1])
INPUTS += click(8000, 400, 438)
NAMES = {}
for base in (5200, 6000):
    for u in range(base - 60, base + 61, 2):
        NAMES[u] = 'phase_intro_%d' % u
SHOTS = sorted(NAMES)
END = 6100
