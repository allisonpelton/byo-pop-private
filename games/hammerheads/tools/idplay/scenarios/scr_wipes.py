"""The zoom wipes between the screens, for the screens pass (SCREENS.md): a player with no restart points and no how-to-play page, and
a shot every few updates through each wipe: from the loading screen to the menu, to the Trophy Room and back, to the High Scores and
back, from the main menu into a new game, and from the game back to the menu (the hammer, the trophy and the quit wipes)."""

SEED = 1
PROFILE = 'plain'
STEP = 12
LENGTH = 220                                                    # how long after a click the shots go on


def click(count, x, y):
    return [(count, 'move %d %d' % (x, y), 'move %d %d' % (x, y)),
            (count + 5, 'click %d %d' % (x, y), 'click %d %d' % (x, y))]


INPUTS = click(1000, 397, 532)                                  # the loading screen's "Click to Play"
INPUTS += click(2500, 740, 540)                                 # Trophy Room
INPUTS += click(4500, 400, 570)                                 # Back to Main Menu
INPUTS += click(6500, 50, 540)                                  # High Scores
INPUTS += click(8500, 400, 570)                                 # Main Menu
INPUTS += click(10500, 140, 320)                                # Classic Bash: no points to choose, a new game
INPUTS += click(12500, 400, 438)                                # Go, on the introduction
INPUTS += click(13500, 400, 344)                                # Go, on the Ready box
INPUTS += click(14500, 750, 30)                                 # the in-game menu's button
INPUTS += click(15000, 400, 235)                                # "Main Menu"
INPUTS += click(15500, 300, 365)                                # "Yes": leave the game

NAMES = {}
for label, start in (('loading', 1000), ('to_trophies', 2500), ('from_trophies', 4500), ('to_scores', 6500), ('from_scores', 8500),
                     ('to_game', 10500), ('leave_game', 15500)):
    for i in range(1, LENGTH // STEP + 1):
        NAMES[start + STEP * i] = 'wipe_%s_%02d' % (label, i)
NAMES[12000] = 'intro_level01'
NAMES[13000] = 'ready_level01'
NAMES[14000] = 'board_level01'
SHOTS = sorted(NAMES)
END = 18000
