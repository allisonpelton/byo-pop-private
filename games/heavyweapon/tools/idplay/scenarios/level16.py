# The campaign continued before level 16 (a players file saved there, as after passing level 15): the continue box,
# the armory (the bot spends the level's points) and the map, then the bot until its last tank is lost or the frames run
# out.
PROFILE = 'level16'
SEED = 16
END = 30000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (150, 'move 100 250'), (150, 'click 100 250'),              # the main menu's campaign
    (200, 'move 170 315'), (200, 'click 170 315'),              # the continue box's CONTINUE
]
BOT_FROM = 260
