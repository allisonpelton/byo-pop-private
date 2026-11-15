# Level 18's boss: the campaign continued before level 18 (a players file saved there), with every level 1200 updates
# long (SHORT_LEVELS: a copy of data/levels.xml that both games read), so the boss comes 600 updates in: the continue
# box, the armory and the map, then the bot until its last tank is lost or the frames run out.
PROFILE = 'level18'
SHORT_LEVELS = 1200
SEED = 118
END = 20000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (150, 'move 100 250'), (150, 'click 100 250'),              # the main menu's campaign
    (200, 'move 170 315'), (200, 'click 170 315'),              # the continue box's CONTINUE
]
BOT_FROM = 260
