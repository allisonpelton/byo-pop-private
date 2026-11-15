# a coverage run (docs/COVERAGE.md): Survival played by the bot for a minute and a half, then left to be lost: the water takes the
# level, the summary counts a bonus for the minute survived, EXIT leads to the honor roll, a click to the main menu. Clicks at (350, 415)
# while the bot is idle dismiss the cupid's warning (the one place that is a button of it and not of the summary). The frames of the
# loss and of the summary are those of the port's run of this script (`state` and `buttons`).
PROFILE_COMMANDS = ['unlock all', 'goto menu']


def click(frame, x, y):
    return [(frame, 'move %d %d' % (x, y), 'move %d %d' % (x, y)), (frame + 50, 'down %d %d' % (x, y), 'down %d %d' % (x, y)),
            (frame + 52, 'up %d %d' % (x, y), 'up %d %d' % (x, y))]


INPUTS = click(100, 400, 232) + click(700, 400, 370)
for slot in range(21800, 32500, 300):
    INPUTS += click(slot, 350, 415)
INPUTS += click(32800, 578, 395)  # EXIT of the summary
INPUTS += click(34500, 400, 300)  # the honor roll
BOT_AT = 1500  # the scenario frame the bot starts at (the level is loading)
BOT_FRAMES = 20000
END = 37000  # frames from the start of the inputs
BOT_MODE = "continue"
