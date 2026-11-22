# The screen pass (SCREENS.md): a first run in full screen (ScreenMode 1 in both games' settings). No bot. The title, the
# name box, the main menu, the options box (its full screen box ticked), a first campaign's map, its zoom and the help
# before the first game, level 1, the status bar's MENU box and the game resumed. The places are the same as in a window.
FULL_SCREEN = True
END = 1100
BOT_FROM = None
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (100, 'char F'), (103, 'char u'), (106, 'char l'), (120, 'key RETURN'),   # the new player's name
    (200, 'move 505 285'), (200, 'click 505 285'),              # the options box
    (300, 'move 320 375'), (300, 'click 320 375'),              # its DONE
    (400, 'move 100 250'), (400, 'click 100 250'),              # the main menu's campaign
    (579, 'move 160 452'), (579, 'click 160 452'),              # the map's invade
    (675, 'move 560 462'), (675, 'click 560 462'),              # the help's PLAY
    (900, 'move 585 15'), (900, 'click 585 15'),                # the status bar's MENU
    (1000, 'move 320 280'), (1000, 'click 320 280'),            # RESUME
]
SHOTS = [
    (20, 'fs_title'), (80, 'fs_namebox'), (150, 'fs_mainmenu'), (250, 'fs_options'), (350, 'fs_mainmenu_back'),
    (500, 'fs_map'), (620, 'fs_zoom'), (660, 'fs_help'), (800, 'fs_level1'), (950, 'fs_menubox'), (1050, 'fs_resumed'),
]
