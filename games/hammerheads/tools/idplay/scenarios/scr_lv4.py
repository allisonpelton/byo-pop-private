"""The introductions, Ready boxes and first boards of levels 17 to 21 of Classic Bash, for the screens pass (SCREENS.md): the player of
mkprofile.py `lv4`, whose restart points start the game on each of them (levels_common.py)."""
import levels_common

SEED = 1
PROFILE = 'lv4'
INPUTS, NAMES, END = levels_common.make(17, 21)
SHOTS = sorted(NAMES)
