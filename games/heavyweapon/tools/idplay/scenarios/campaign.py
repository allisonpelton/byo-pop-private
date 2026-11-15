# A first run: the title's play, a new player's name typed, the main menu's campaign, and then the bot from the map on: the
# help before the first game, level 1 with its boss, the results, the armory and the levels after, until the bot's last tank
# is lost or the frames run out.
END = 40000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play" (the bar is gone by 31)
    (100, 'char T'), (103, 'char e'), (106, 'char s'), (109, 'char t'), (120, 'key RETURN'),   # the new player's name
    (200, 'move 100 250'), (200, 'click 100 250'),              # the main menu's campaign
]
BOT_FROM = 260
