# The coverage check (GAME_NOTES.md, "Coverage check"): the input no other scenario gives, for the original's functions
# that only it reaches. No bot. The first name box's OK with no name (the box that refuses it), the main menu's survival
# while it is locked (its box), the options' sound sliders dragged and clicked and the cursor and hints boxes ticked and
# unticked, the player list's new-player line and that box's CANCEL, a player picked from the list, the main menu's help
# left up for its four turns and its box ticked and unticked; then a campaign's level 1: the single shots that bring the
# auto-fire tip, the tip's box ticked and its OK, a typed letter, the space bar's pause and the same key's resume, the
# right button's nuke, and the focus lost and regained. The places are each box's Resize.
END = 6400
BOT_FROM = None
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (150, 'move 295 295'), (150, 'click 295 295'),              # the name box's OK with no name
    (230, 'move 295 295'), (230, 'click 295 295'),              # the refusal's OK
    (300, 'char C'), (303, 'char o'), (306, 'char v'),          # the new player's name
    (330, 'move 295 295'), (330, 'click 295 295'),              # the name box's OK
    (430, 'move 230 380'), (430, 'click 230 380'),              # the main menu's survival, still locked
    (500, 'move 295 300'), (500, 'click 295 300'),              # the locked box's OK
    (580, 'move 505 285'), (580, 'click 505 285'),              # the options box
    (650, 'move 300 105'), (650, 'down 300 105'),               # the sound effects slider, dragged
    (655, 'move 350 105'), (660, 'move 400 105'), (665, 'up 400 105'),
    (700, 'move 350 135'), (700, 'click 350 135'),              # the music slider, clicked
    (740, 'move 195 265'), (740, 'click 195 265'),              # the custom cursors box, off
    (780, 'move 195 265'), (780, 'click 195 265'),              # and on
    (820, 'move 195 295'), (820, 'click 195 295'),              # the hints box, off
    (860, 'move 195 295'), (860, 'click 195 295'),              # and on
    (900, 'move 320 375'), (900, 'click 320 375'),              # the options' DONE
    (980, 'move 490 90'), (980, 'click 490 90'),                # the player list
    (1050, 'move 320 112'), (1050, 'click 320 112'),            # its first line: a new player's box
    (1120, 'move 360 295'), (1120, 'click 360 295'),            # that box's CANCEL
    (1190, 'move 320 132'), (1190, 'click 320 132'),            # the list's player
    (1250, 'move 385 395'), (1250, 'click 385 395'),            # the list's CANCEL
    (1330, 'move 505 355'), (1330, 'click 505 355'),            # the main menu's help
    (3900, 'move 35 463'), (3900, 'click 35 463'),              # its box, ticked
    (3950, 'move 35 463'), (3950, 'click 35 463'),              # and unticked
    (4000, 'move 560 462'), (4000, 'click 560 462'),            # its OK
    (4080, 'move 100 250'), (4080, 'click 100 250'),            # the main menu's campaign
    (4260, 'move 160 452'), (4260, 'click 160 452'),            # the map's invade
    (4355, 'move 560 462'), (4355, 'click 560 462'),            # the help's OK at the level's start
    (4900, 'move 300 300'),                                     # single shots, more than the tip waits for: a press
    # held two frames fires one (a click's press and release in one update fires none)
    (4900, 'down 300 300'), (4902, 'up 300 300'), (4920, 'down 310 300'), (4922, 'up 310 300'),
    (4940, 'down 320 300'), (4942, 'up 320 300'), (4960, 'down 330 300'), (4962, 'up 330 300'),
    (4980, 'down 340 300'), (4982, 'up 340 300'), (5000, 'down 350 300'), (5002, 'up 350 300'),
    (5020, 'down 360 300'), (5022, 'up 360 300'), (5040, 'down 370 300'), (5042, 'up 370 300'),
    (5120, 'move 195 295'), (5120, 'click 195 295'),            # the auto-fire tip's box
    (5170, 'move 320 335'), (5170, 'click 320 335'),            # its OK
    (5250, 'move 320 240'), (5250, 'char A'),                   # a typed letter
    (5300, 'key SPACE'),                                        # the pause
    (5400, 'key SPACE'),                                        # and the game resumed
    (5500, 'rclick 320 240'),                                   # the nuke
    (5800, 'focus lose'),
    (5900, 'focus regain'),
    (6000, 'move 320 240'), (6000, 'click 320 240'),            # "click anywhere to continue"
]
SHOTS = [
    (200, 'cov_refused'), (400, 'cov_mainmenu'), (470, 'cov_locked'), (640, 'cov_options'), (690, 'cov_dragged'),
    (760, 'cov_cursors_off'), (840, 'cov_hints_off'), (1020, 'cov_userlist'), (1090, 'cov_newplayer'),
    (1220, 'cov_picked'), (1300, 'cov_mainmenu_back'), (2000, 'cov_help_2000'), (3000, 'cov_help_3000'),
    (3930, 'cov_help_ticked'), (4300, 'cov_into_level'), (5100, 'cov_tip'), (5150, 'cov_tip_ticked'),
    (5350, 'cov_paused'), (5450, 'cov_resumed'), (5530, 'cov_nuke'), (5850, 'cov_focus_lost'), (6100, 'cov_end'),
]
