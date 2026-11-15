# a player with every mode unlocked (the port makes the profile: `unlock all`), on the main menu: Play Now, Flood and its new game
# are clicked, then the bot plays Flood and on through the levels after it (its shots and clicks are given to both as input).
# The clicks are a move, a press and a release some updates apart (the game reads the button once an update); the places are
# those of the `buttons` command on each screen.
PROFILE_COMMANDS = ['unlock all', 'goto menu']
INPUTS = [
    (100, 'move 400 232', 'move 400 232'),
    (150, 'down 400 232', 'down 400 232'),
    (152, 'up 400 232', 'up 400 232'),
    (700, 'move 401 270', 'move 401 270'),
    (750, 'down 401 270', 'down 401 270'),
    (752, 'up 401 270', 'up 401 270'),
    (1300, 'move 470 330', 'move 470 330'),
    (1350, 'down 470 330', 'down 470 330'),
    (1352, 'up 470 330', 'up 470 330'),
]
BOT_AT = 1800  # the scenario frame the bot starts at (the level is loading)
BOT_FRAMES = 29000
END = 30000  # frames from the start of the inputs
BOT_MODE = "continue"
