"""The introduction of level 6 (the shop sign) of `lv1`: the port's shots at ages 1000 to 2000 updates after the point is chosen,
every 5 updates, to match the original's `intro_level06` of scr_lv1 (age about 1495) and find the port's offset in time."""
import levels_common

SEED = 1
PROFILE = 'lv1'
CENTRES = levels_common.button_centres(6)
click = levels_common.click

INPUTS = click(1000, 397, 532)
INPUTS += click(2000, 140, 320)
INPUTS += click(5000, *CENTRES[5])
NAMES = {5005 + age: 'phase6_%d' % age for age in range(1000, 2001, 5)}
SHOTS = sorted(NAMES)
END = 7100
