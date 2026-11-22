"""A player with a past (mkprofile.py `rich`: Tough Cookie and Marathon open, restart points in Classic and Tough Cookie, trophies won), for
the screens pass (SCREENS.md): the main menu with every mode open, each mode's button offering its restart-point screen, and the trophy
room with trophies."""

SEED = 1
END = 14000
PROFILE = 'rich'


def click(count, x, y):
    return [(count, 'move %d %d' % (x, y), 'move %d %d' % (x, y)),
            (count + 5, 'click %d %d' % (x, y), 'click %d %d' % (x, y))]


INPUTS = click(1000, 397, 532)                                  # the loading screen's "Click to Play"
INPUTS += click(2500, 400, 360)                                 # OK on the Tough Cookie notice
INPUTS += click(3500, 140, 320)                                 # Classic Bash: the restart-point screen

INPUTS += click(5000, 400, 200)                                 # "Start on Level 10"
INPUTS += click(6500, 300, 550)                                 # "Click here to play" on the how-to-play page
INPUTS += click(8000, 400, 438)                                 # Go, on the introduction
INPUTS += click(9000, 400, 344)                                 # Go, on the Ready box
INPUTS += click(10500, 750, 30)                                 # the in-game menu's button
INPUTS += click(11500, 400, 235)                                # "Main Menu"

INPUTS += click(12500, 300, 365)                                # "Yes": leave the game (it is saved)
INPUTS += click(15000, 140, 320)                                # Classic Bash: the box that offers the saved game

INPUTS += click(16500, 285, 390)                                # "Continue": the saved game goes on

NAMES = {1900: 'tough_notice', 3000: 'menu_open', 4500: 'resurrect_classic', 7500: 'intro_restart', 10000: 'board_restart',
         11000: 'game_menu', 12000: 'leave_game', 14500: 'menu_after_leave', 16000: 'continue_box', 18000: 'board_continued'}
SHOTS = sorted(NAMES)
END = 19000
