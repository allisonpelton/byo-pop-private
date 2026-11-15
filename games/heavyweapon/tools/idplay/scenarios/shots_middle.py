# Phase two, the middle of the campaign: continued before level 8 with every level 1200 updates long and every boss at 1
# point of armor (WEAK_BOSSES: a copy of data/bosses.xml both games read), so the bot beats levels 8 and 9 and their bosses:
# the boss warnings, the bosses' deaths, the results and armories, the middle cutscene after level 9, and the levels after
# it until the bot's last tank is lost; a shot every 150 frames.
PROFILE = 'level8'
SHORT_LEVELS = 1200
WEAK_BOSSES = True
SEED = 208
END = 30000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (150, 'move 100 250'), (150, 'click 100 250'),              # the main menu's campaign
    (200, 'move 170 315'), (200, 'click 170 315'),              # the continue box's CONTINUE
]
BOT_FROM = 260
SHOT_EVERY = (100, 150)
