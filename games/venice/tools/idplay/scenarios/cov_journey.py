# a coverage run (docs/COVERAGE.md): a player with every mode unlocked and Journey on level LEVEL (the port makes the profile:
# `unlock all`, `set mode`, `set level`), on the main menu: Play Now, Journey and Continue (the left button; the right one starts a new game) are clicked, then the bot plays Journey
# from that level on, through the summaries and on through the levels after it (its shots and clicks are given to both as input).
# The scenarios cov_j<level>.py set LEVEL and the length of the run; the places are those of the `buttons` command on each screen.
LEVEL = globals().get('LEVEL', 4)
PROFILE_COMMANDS = ['unlock all', 'set mode 1', 'set level %d' % LEVEL, 'goto menu']
INPUTS = [
    (100, 'move 400 232', 'move 400 232'),
    (150, 'down 400 232', 'down 400 232'),
    (152, 'up 400 232', 'up 400 232'),
    (700, 'move 400 220', 'move 400 220'),
    (750, 'down 400 220', 'down 400 220'),
    (752, 'up 400 220', 'up 400 220'),
    (1300, 'move 330 330', 'move 330 330'),
    (1350, 'down 330 330', 'down 330 330'),
    (1352, 'up 330 330', 'up 330 330'),
]
BOT_AT = 1800  # the scenario frame the bot starts at (the level is loading)
BOT_FRAMES = globals().get('FRAMES', 40000)
END = BOT_AT + BOT_FRAMES + 1000  # frames from the start of the inputs
BOT_MODE = "continue"
