# Survival, by a player whose modes are all unlocked (a players file the port makes): the title's play, the main menu's
# survival (its help first, as on a player's first survival), and then the bot until its last tank is lost or the frames
# run out.
PROFILE = 'unlocked'
END = 30000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (150, 'move 230 380'), (150, 'click 230 380'),              # the main menu's survival
]
BOT_FROM = 200
