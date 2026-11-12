# a player with every mode unlocked (the port makes the profile: `unlock all`), on the main menu: Play Now and Survival are clicked,
# then the bot plays Survival (its shots and clicks are given to both as input). The clicks are a move, a press and a release some
# updates apart (the game reads the button once an update); the places are those of the `buttons` command on each screen.
PROFILE_COMMANDS = ['unlock all', 'goto menu']
INPUTS = [
    (100, 'move 400 232', 'move 400 232'),
    (150, 'down 400 232', 'down 400 232'),
    (152, 'up 400 232', 'up 400 232'),
    (700, 'move 400 370', 'move 400 370'),
    (750, 'down 400 370', 'down 400 370'),
    (752, 'up 400 370', 'up 400 370'),
]
BOT_AT = 1500  # the scenario frame the bot starts at (the level is loading)
BOT_FRAMES = 19000
END = 20000  # frames from the start of the inputs
BOT_MODE = "continue"
