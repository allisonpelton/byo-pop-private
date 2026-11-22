"""The introductions, Ready boxes and first boards of levels 2 to 7 of Classic Bash, for the screens pass (SCREENS.md): the player of
mkprofile.py `lv1`, whose restart points start the game on each of them (levels_common.py)."""
import levels_common

SEED = 1
PROFILE = 'lv1'
INPUTS, NAMES, END = levels_common.make(2, 7)
SHOTS = sorted(NAMES)
