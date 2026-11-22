"""The introductions, Ready boxes and first boards of levels 8 to 12 of Classic Bash, for the screens pass (SCREENS.md): the player of
mkprofile.py `lv2`, whose restart points start the game on each of them (levels_common.py)."""
import levels_common

SEED = 1
PROFILE = 'lv2'
INPUTS, NAMES, END = levels_common.make(8, 12)
SHOTS = sorted(NAMES)
