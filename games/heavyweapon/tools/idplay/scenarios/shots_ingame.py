# Phase two, the boxes of a game: a new player's first campaign up to level 1 (the map, the way into the level), then
# with no bot: the status bar's MENU and its box, the options from it and back, the game resumed, the focus lost (the
# game pauses) and regained (still paused) and a click to go on, the MENU's main menu and the box that warns the progress
# is lost, answered no. The places are the buttons' own (each box's Resize) and the onward buttons the campaign's bot
# clicks.
END = 2400
BOT_FROM = None
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (100, 'char T'), (103, 'char e'), (106, 'char s'), (109, 'char t'), (120, 'key RETURN'),   # the new player's name
    (200, 'move 100 250'), (200, 'click 100 250'),              # the main menu's campaign
    (379, 'move 160 452'), (379, 'click 160 452'),              # the map's invade
    (475, 'move 560 462'), (475, 'click 560 462'),              # the zoomed map's onward button
    (900, 'move 585 15'), (900, 'click 585 15'),                # the status bar's MENU
    (1000, 'move 320 200'), (1000, 'click 320 200'),            # its OPTIONS
    (1100, 'move 320 375'), (1100, 'click 320 375'),            # the options' DONE
    (1200, 'move 320 280'), (1200, 'click 320 280'),            # RESUME
    (1300, 'focus lose'),
    (1400, 'focus regain'),
    (1500, 'move 320 240'), (1500, 'click 320 240'),            # "click anywhere to continue"
    (2000, 'move 585 15'), (2000, 'click 585 15'),              # the MENU again
    (2100, 'move 320 240'), (2100, 'click 320 240'),            # its MAIN MENU
    (2200, 'move 380 317'), (2200, 'click 380 317'),            # the warning's NO
]
SHOTS = [
    (300, 'ingame_map'), (420, 'ingame_into_level'), (600, 'ingame_level1'), (950, 'ingame_menubox'), (1050, 'ingame_options'),
    (1150, 'ingame_menubox_again'), (1250, 'ingame_resumed'), (1350, 'ingame_focus_lost'), (1450, 'ingame_focus_regained'),
    (1550, 'ingame_continued'), (2050, 'ingame_menubox_third'), (2150, 'ingame_confirm'), (2250, 'ingame_after_no'),
]
