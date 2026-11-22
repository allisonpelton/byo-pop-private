# The screen pass (SCREENS.md): the menus' boxes no other tour shows. No bot. A first run's name, the main menu's QUIT
# box opened ten times (each draws its quote with Rand(), so the quotes vary) and answered NO, the survival box while it
# is locked, the player list's new-player box from the list, the box for a name that is taken, a second player made,
# the list with two players, the RENAME box and its CANCEL, the DELETE box answered NO and then YES, the high scores'
# RESET box for each list answered NO. The places are each box's Resize.
END = 2600
BOT_FROM = None
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (100, 'char A'), (103, 'char c'), (106, 'char e'), (120, 'key RETURN'),   # the new player's name
]
SHOTS = []
for _turn in range(10):                                         # the QUIT box ten times
    _at = 200 + _turn * 80
    OPENING += [(_at, 'move 535 425'), (_at, 'click 535 425'), (_at + 50, 'move 385 315'), (_at + 50, 'click 385 315')]
    SHOTS.append((_at + 40, 'dlg_quit%d' % _turn))
OPENING += [
    (1000, 'move 230 380'), (1000, 'click 230 380'),            # SURVIVAL, still locked
    (1080, 'move 300 300'), (1080, 'click 300 300'),            # the locked box's OK
    (1120, 'move 490 90'), (1120, 'click 490 90'),              # the player list
    (1180, 'move 320 112'), (1180, 'click 320 112'),            # its first line: a new player's box
    (1230, 'char A'), (1233, 'char c'), (1236, 'char e'),       # a name that is taken
    (1250, 'move 255 295'), (1250, 'click 255 295'),            # OK (this box has CANCEL beside it)
    (1320, 'move 300 295'), (1320, 'click 300 295'),            # the taken box's OK
    (1360, 'key BACK'), (1363, 'key BACK'), (1366, 'key BACK'),
    (1370, 'char B'), (1373, 'char o'), (1376, 'char b'),       # another name
    (1390, 'move 255 295'), (1390, 'click 255 295'),            # OK: the new player, chosen, and the list closes
    (1460, 'move 490 90'), (1460, 'click 490 90'),              # the player list again, with two players
    (1520, 'move 320 132'), (1520, 'click 320 132'),            # the first player picked
    (1560, 'move 255 360'), (1560, 'click 255 360'),            # RENAME
    (1620, 'move 385 295'), (1620, 'click 385 295'),            # the rename box's CANCEL
    (1660, 'move 385 360'), (1660, 'click 385 360'),            # DELETE
    (1720, 'move 385 295'), (1720, 'click 385 295'),            # the delete box's NO
    (1760, 'move 385 360'), (1760, 'click 385 360'),            # DELETE again
    (1800, 'move 255 295'), (1800, 'click 255 295'),            # YES: the player is deleted
    (1860, 'move 385 395'), (1860, 'click 385 395'),            # the list's CANCEL
    (1920, 'move 535 215'), (1920, 'click 535 215'),            # the high scores
    (1990, 'move 415 455'), (1990, 'click 415 455'),            # RESET (the mission list)
    (2050, 'move 385 325'), (2050, 'click 385 325'),            # its NO
    (2100, 'move 275 455'), (2100, 'click 275 455'),            # the survival list
    (2150, 'move 415 455'), (2150, 'click 415 455'),            # RESET (the survival list)
    (2210, 'move 385 325'), (2210, 'click 385 325'),            # its NO
    (2260, 'move 535 455'), (2260, 'click 535 455'),            # MAIN
]
SHOTS += [
    (1050, 'dlg_locked'), (1160, 'dlg_userlist'), (1220, 'dlg_newplayer'), (1245, 'dlg_newplayer_typed'),
    (1300, 'dlg_taken'), (1385, 'dlg_newplayer_bob'), (1440, 'dlg_mainmenu_bob'), (1500, 'dlg_userlist_two'),
    (1540, 'dlg_userlist_picked'), (1600, 'dlg_rename'), (1640, 'dlg_userlist_back'), (1700, 'dlg_delete'),
    (1745, 'dlg_userlist_kept'), (1785, 'dlg_delete_again'), (1840, 'dlg_userlist_deleted'), (1900, 'dlg_mainmenu_after_list'),
    (1970, 'dlg_highscores'), (2030, 'dlg_reset_mission'), (2080, 'dlg_highscores_back'), (2130, 'dlg_highscores_survival'),
    (2190, 'dlg_reset_survival'), (2240, 'dlg_highscores_survival_back'), (2320, 'dlg_mainmenu_end'),
]
