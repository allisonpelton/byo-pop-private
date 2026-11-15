# Phase two, the end of the campaign: continued before level 18 with short levels and weak bosses (as shots_middle), so the
# bot beats levels 18 and 19 and the final boss: the final boss's fight and death, the final cutscene, the campaign's end
# and what follows it; a shot every 150 frames.
PROFILE = 'level18'
SHORT_LEVELS = 1200
WEAK_BOSSES = True
SEED = 218
END = 30000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (150, 'move 100 250'), (150, 'click 100 250'),              # the main menu's campaign
    (200, 'move 170 315'), (200, 'click 170 315'),              # the continue box's CONTINUE
]
BOT_FROM = 260
SHOT_EVERY = (100, 150)
