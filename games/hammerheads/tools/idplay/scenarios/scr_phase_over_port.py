"""The game over box of level 3 of `lv1` (the point's one heart is lost): the port's shots every 10 updates around the original's
`over_level03` of scr_lv1 (7600 updates after the point is chosen), to find the dialog age that matches it."""
import levels_common

SEED = 1
PROFILE = 'lv1'
CENTRES = levels_common.button_centres(6)
click = levels_common.click

INPUTS = click(1000, 397, 532)
INPUTS += click(2000, 140, 320)
INPUTS += click(5000, *CENTRES[2])
INPUTS += click(8000, 400, 438)      # Go, on the introduction
INPUTS += click(9000, 400, 344)      # Go, on the Ready box
NAMES = {5005 + age: 'over3_%d' % age for age in range(6000, 9001, 10)}
SHOTS = sorted(NAMES)
END = 14100
