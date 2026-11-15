# Phase two, the menus: a first run's title and name box, the main menu with a button's description, the options box and
# the credits and help it leads to, the high scores (both lists), the main menu's help, the player list and the quit box,
# each opened and closed by clicks. No bot. The places are the buttons' own (each screen's Resize).
END = 2000
BOT_FROM = None
OPENING = [
    (150, 'move 320 419'), (152, 'click 320 419'),              # the title's "click here to play"
    (210, 'char A'), (213, 'char c'), (216, 'char e'),          # the new player's name
    (240, 'key RETURN'),
    (330, 'move 505 285'),                                      # over the main menu's options: its description
    (380, 'click 505 285'),                                     # the options box
    (450, 'click 395 340'),                                     # its CREDITS
    (900, 'click 320 240'),                                     # the credits closed
    (1000, 'click 245 340'),                                    # the options box's HELP
    (1100, 'click 560 462'),                                    # its OK
    (1150, 'click 320 375'),                                    # the options box's DONE
    (1250, 'click 535 215'),                                    # the high scores
    (1320, 'click 275 455'),                                    # the survival list
    (1400, 'click 535 455'),                                    # back to the main menu
    (1500, 'click 505 355'),                                    # the main menu's help
    (1600, 'click 560 462'),                                    # its OK
    (1650, 'click 490 90'),                                     # the player list
    (1720, 'click 385 395'),                                    # its CANCEL
    (1800, 'click 535 425'),                                    # the quit box
    (1870, 'click 385 315'),                                    # its NO
]
SHOTS = [
    (20, 'menus_title_early'), (100, 'menus_title'), (200, 'menus_namebox'), (230, 'menus_name_typed'),
    (300, 'menus_mainmenu'), (360, 'menus_mainmenu_hover'), (440, 'menus_options'), (520, 'menus_credits_start'),
    (800, 'menus_credits_scrolled'), (960, 'menus_options_again'), (1060, 'menus_options_help'), (1200, 'menus_mainmenu_back'),
    (1300, 'menus_highscores'), (1370, 'menus_highscores_survival'), (1450, 'menus_mainmenu_from_scores'),
    (1560, 'menus_help'), (1630, 'menus_mainmenu_from_help'), (1700, 'menus_userlist'), (1770, 'menus_mainmenu_from_list'),
    (1850, 'menus_quitbox'), (1920, 'menus_mainmenu_end'),
]
