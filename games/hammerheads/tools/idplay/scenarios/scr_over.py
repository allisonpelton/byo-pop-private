"""Games that end, for the screens pass (SCREENS.md): the player of mkprofile.py `over` has one heart in each of his restart points and
the game is left to run, so that the game over box is reached in Tough Cookie, then in Classic Bash, then a Marathon Bash is left
to run to its end."""

SEED = 1
PROFILE = 'over'


def click(count, x, y):
    return [(count, 'move %d %d' % (x, y), 'move %d %d' % (x, y)),
            (count + 5, 'click %d %d' % (x, y), 'click %d %d' % (x, y))]


INPUTS = click(1000, 397, 532)                                  # the loading screen's "Click to Play"
INPUTS += click(2500, 195, 235)                                 # Tough Cookie: the choice of restart points
INPUTS += click(4000, 400, 274)                                 # the point of level 4 (the second of three buttons)
INPUTS += click(6000, 400, 344)                                 # Go, on the Ready box
INPUTS += click(11000, 520, 368)                                # "Main Menu" on the game over box of Tough Cookie
INPUTS += click(13000, 140, 320)                                # Classic Bash: the choice of restart points
INPUTS += click(15000, 400, 274)                                # the point of level 5
INPUTS += click(17000, 400, 438)                                # Go, on the introduction
INPUTS += click(22500, 522, 470)                                # "Main Menu" on the game over box of Classic Bash
INPUTS += click(24500, 670, 320)                                # Marathon Bash
INPUTS += click(26500, 400, 385)                                # Go, on the Marathon box

NAMES = {3500: 'tough_points', 5200: 'tough_ready', 7500: 'tough_board', 10500: 'tough_over', 14500: 'classic_points',
         16400: 'classic_intro', 18000: 'classic_board', 22000: 'classic_over'}
NAMES[26000] = 'marathon_box'
NAMES[28000] = 'marathon_board'
NAMES[30000] = 'marathon_wave_cleared'
NAMES[32000] = 'marathon_board_two'
NAMES[36000] = 'marathon_over'
INPUTS += click(37000, 520, 368)                                # "Main Menu" on the game over box of Marathon Bash
NAMES[39000] = 'menu_after_marathon'
SHOTS = sorted(NAMES)
END = 40000
