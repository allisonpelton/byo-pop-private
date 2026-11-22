"""The introductions, Ready boxes and first boards of levels 12 to 16 of Classic Bash, for the screens pass (SCREENS.md): the player of
mkprofile.py `lv3`, whose restart points start the game on each of them (levels_common.py)."""
import levels_common

SEED = 1
PROFILE = 'lv3'
INPUTS, NAMES, END = levels_common.make(12, 16)
SHOTS = sorted(NAMES)
