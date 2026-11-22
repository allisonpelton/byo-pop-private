"""The introductions, Ready boxes and first boards of levels 22 to 25 of Classic Bash, for the screens pass (SCREENS.md): the player of
mkprofile.py `lv5`, whose restart points start the game on each of them (levels_common.py)."""
import levels_common

SEED = 1
PROFILE = 'lv5'
INPUTS, NAMES, END = levels_common.make(22, 25)
SHOTS = sorted(NAMES)
