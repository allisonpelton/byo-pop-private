# The screen pass (SCREENS.md): the campaign from level 11 on, levels no other tour plays, as shots_middle plays 8 to 10:
# continued before level 11 with every level 1200 updates long and every boss at 1 point of armor, so the bot meets each
# level's enemies and boss, the boss's death, the results, the armory and the map, level after level until its last tank
# is lost; a shot every 300 frames, and the play tips and boxes the bot clicks away the frame before (SHOT_BEFORE_CLICKS).
PROFILE = 'level11'
SHORT_LEVELS = 1200
WEAK_BOSSES = True
SEED = 211
END = 30600                 # the final cutscene, the high scores after the win, and 500 frames of them
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (150, 'move 100 250'), (150, 'click 100 250'),              # the main menu's campaign
    (200, 'move 170 315'), (200, 'click 170 315'),              # the continue box's CONTINUE
]
BOT_FROM = 260
SHOT_EVERY = (100, 300)
SHOT_BEFORE_CLICKS = {'320 335': 'tip'}      # a play tip's OK, which the bot clicks a few updates after it opens
