# Phase two, survival by a player whose modes are all unlocked: the title's play, the main menu's survival with the help
# before a first survival, the bot's game until its last tank is lost, and the game-over box; a shot every 300 frames.
PROFILE = 'unlocked'
END = 30000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (150, 'move 230 380'), (150, 'click 230 380'),              # the main menu's survival
]
BOT_FROM = 200
SHOT_EVERY = (100, 300)
