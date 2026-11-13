# a coverage run (docs/COVERAGE.md): a player with every mode unlocked and Trick Shot on level LEVEL (the port makes the profile:
# `unlock all`, `set mode`, `set level`), on the main menu: Play Now, Trick Shot and Continue are clicked, then the bot plays Trick Shot from that
# level on (its shots and clicks are given to both as input). The scenarios cov_t<level>.py set LEVEL.
LEVEL = globals().get('LEVEL', 1)
PROFILE_COMMANDS = ['unlock all', 'set mode 3', 'set level %d' % LEVEL, 'goto menu']
INPUTS = [
    (100, 'move 400 232', 'move 400 232'),
    (150, 'down 400 232', 'down 400 232'),
    (152, 'up 400 232', 'up 400 232'),
    (700, 'move 398 320', 'move 398 320'),
    (750, 'down 398 320', 'down 398 320'),
    (752, 'up 398 320', 'up 398 320'),
    (1300, 'move 330 330', 'move 330 330'),  # Continue (a player past the first level is asked)
    (1350, 'down 330 330', 'down 330 330'),
    (1352, 'up 330 330', 'up 330 330'),
]
BOT_AT = 1800  # the scenario frame the bot starts at (the level is loading)
BOT_FRAMES = globals().get('FRAMES', 20000)
END = BOT_AT + BOT_FRAMES + 1000  # frames from the start of the inputs
BOT_MODE = "continue"
