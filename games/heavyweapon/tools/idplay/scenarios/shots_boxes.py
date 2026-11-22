# The screen pass (SCREENS.md): the boxes of a campaign that no other tour shows. A player saved before level 8: the
# campaign's continue box, CONTINUE to the armory, its GO with points left (the box that asks for them to be spent) and
# that box's OK; then the bot spends the points and plays level 8 at its full length. When its last tank is lost (frame
# LAST_TANK_LOST, read from the record run's trace) the status bar's MENU and MAIN MENU bring the short warning (no tank
# left to lose), answered NO, and RESUME; then the game-over box and its QUIT to the high scores. The play tips and boxes
# the bot clicks away are shot the frame before (SHOT_BEFORE_CLICKS).
PROFILE = 'level8'
SEED = 308
END = 30000
LAST_TANK_LOST = 4739       # the frame the last tank is lost in the record run (lives 0, the tank's wait begun)
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (150, 'move 100 250'), (150, 'click 100 250'),              # the main menu's campaign: the continue box
    (220, 'move 170 315'), (220, 'click 170 315'),              # CONTINUE: the armory
    (300, 'move 565 445'), (300, 'click 565 445'),              # GO with points left: the box
    (360, 'move 295 310'), (360, 'click 295 310'),              # its OK
]
BOT_FROM = 400
SHOTS = [(200, 'boxes_continue'), (280, 'boxes_armory'), (340, 'boxes_points'), (390, 'boxes_armory_again')]
if LAST_TANK_LOST is not None:
    _at = LAST_TANK_LOST + 20
    OPENING += [
        (_at, 'move 585 15'), (_at, 'click 585 15'),            # the status bar's MENU
        (_at + 60, 'move 320 240'), (_at + 60, 'click 320 240'),    # its MAIN MENU: the short warning
        (_at + 120, 'move 380 290'), (_at + 120, 'click 380 290'),  # its NO
        (_at + 170, 'move 320 280'), (_at + 170, 'click 320 280'),  # RESUME
        (_at + 650, 'move 395 310'), (_at + 650, 'click 395 310'),  # the game-over box's QUIT: the high scores
    ]
    SHOTS += [(_at + 40, 'boxes_menubox'), (_at + 100, 'boxes_confirm_short'), (_at + 150, 'boxes_menubox_again'),
              (_at + 200, 'boxes_resumed'), (_at + 620, 'boxes_gameover'), (_at + 720, 'boxes_highscores'),
              (_at + 900, 'boxes_highscores_later')]
SHOT_EVERY = (500, 500)
SHOT_BEFORE_CLICKS = {'320 335': 'tip'}      # a play tip's OK, which the bot clicks a few updates after it opens
