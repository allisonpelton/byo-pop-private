# Phase two, a first campaign: the title's play, a new player's name, the main menu's campaign, the help before the first
# game, the map, level 1 with its play tip, its boss and the results, the armory, the map and level 2, played by the bot
# until its last tank is lost, and the game-over box; a shot every 400 frames.
END = 40000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (100, 'char T'), (103, 'char e'), (106, 'char s'), (109, 'char t'), (120, 'key RETURN'),   # the new player's name
    (200, 'move 100 250'), (200, 'click 100 250'),              # the main menu's campaign
]
BOT_FROM = 260
SHOT_EVERY = (100, 400)
SHOT_BEFORE_CLICKS = {'320 335': 'tip'}      # a play tip's OK, which the bot clicks a few updates after it opens (the screen pass)
