"""Writes origrun scripts (docs/ORIGRUN.md, "Script commands") for the coverage check of Mahjong Escape: Ancient Japan or
Ancient China (docs/COVERAGE.md). The coordinates are the port's own buttons, which the original shares (the state
command lists them); the clicks of a stage come from the port's solver, which wins a stage with its special tiles:

    python mkscript.py adventure <solve dir> <first stage> <last stage> [--new NAME]    stages of the escape adventure
    python mkscript.py tour                                                            the menus, dialogs and one classic board

<solve dir> holds s<n>.txt (one "x1 y1 x2 y2" line per move) and e<n>.txt (the solver's events: the tips that come after
a move and the special tiles) of each stage, from `mahjongjapan_dealtest assets/mahjongjapan --stage <n>`. A click comes
after a move of the mouse 150 ms before it (the game reads the hover with GetCursorPos, which posted input does not move).
The script goes to stdout.
"""
import os
import sys

GAME = os.environ.get('GAME', 'mahjongjapan')    # or mahjongchina: the same code, other art, so other places

out = []


def emit(*words):
    out.append(' '.join(str(w) for w in words))


def wait(ms):
    emit('wait', ms)


def click(x, y, after=250):
    emit('move', x, y)
    wait(150)
    emit('click', x, y)
    wait(after)


def key(vk, after=200):
    emit('key', vk)
    wait(after)


VK_RETURN, VK_ESCAPE = 13, 27

# the main menu's buttons
MENU_ADVENTURE, MENU_CLASSIC, MENU_OPTIONS, MENU_HELP, MENU_QUIT = (401, 276), (403, 387), (238, 559), (400, 558), (566, 558)
MAP_MENU, MAP_PLAY = (86, 576), (719, 576)
PLAYERS_LABEL = (400, 516)    # the player's name at the foot of the menu
END_CONTINUE, END_MENU = (109, 557), (709, 557)    # the ending's two buttons: back to the map, and the main menu
END_BUTTON = None    # Ancient China's map has a button that shows the ending again
# the tips at the start of a stage and the timed tips (adventure.py's table): the click that closes each
START_TIP = {1: (400, 407, 0), 2: (312, 433, 0), 3: (409, 314, 300), 6: (412, 315, 300), 10: (392, 341, 300)}
TIMED_TIP = {1: (403, 330), 7: (566, 440), 5: (400, 245)}   # ids 1 and 7 come only in stages 1 and 2
LEVEL_DONE_OK = (397, 400)


def new_player(name):
    click(*MENU_ADVENTURE, after=1500)
    emit('text', name)
    wait(300)
    key(VK_RETURN, after=6000)    # the map's marker walks to the first level before Play answers


def stage(sol, st, limit=None):
    moves = [l.split() for l in open(os.path.join(sol, 's%d.txt' % st)) if l[0].isdigit() or l[0] == '-'][:limit]
    tips = {}
    special = set()
    ntiles = 144
    for l in open(os.path.join(sol, 'e%d.txt' % st)):
        p = l.split()
        if len(p) > 1 and p[1] == 'tiles,':
            ntiles = int(p[0])
        if p[0] == 'special':
            special.add(int(p[1]))
        if p[0] == 'tip' and ((p[1] == '1' and st == 1) or (p[1] == '7' and st == 2) or p[1] == '5'):
            tips.setdefault(int(p[2]), []).append(int(p[1]))
    # The fade to the board takes about a second and the start tip opens 100 ms after the board is shown (Board::ShowTip),
    # so a click on the tip's button earlier would land on a tile.
    click(*MAP_PLAY, after=3000)
    if st in START_TIP:
        x, y, d = START_TIP[st]
        wait(d * 5)
        click(x, y, after=1000)
    wait(1000)
    for i, m in enumerate(moves):
        for tip in tips.get(i, []):
            wait(1250)    # the key and flower tips open a second after the move (Board.cpp, AddTimer 1000)
            click(*TIMED_TIP[tip], after=500)
        click(int(m[0]), int(m[1]), after=300)
        click(int(m[2]), int(m[3]), after=1250)    # the pair's tiles fly to the tray, and the board ignores clicks meanwhile
        if i in special:
            wait(2000)   # a special tile's flights last longer
    if limit is not None:
        return    # a probe: the first moves only
    wait(15000 + 300 * ntiles)    # the tiles left fly off one by one at the end
    click(*LEVEL_DONE_OK, after=7500)


# the help dialog: next page, close; the options dialog's sliders, auto hint, the three quad percentages and OK
HELP_NEXT, HELP_CLOSE = (570, 444), (405, 446)
OPT_MUSIC, OPT_SOUND, OPT_HINT = (400, 177), (450, 207), (258, 281)
OPT_OK = {True: (406, 444), False: (401, 364)}    # from the main menu (with the quad row), and from a board
OPT_QUADS = [(255, 396), (375, 396), (495, 396)]
QUIT_CHECK, QUIT_OK = (248, 301), (531, 308)
# the classic list: previous and next page, Play Layout, and the nine previews
CL_PREV, CL_NEXT, CL_PLAY = (74, 561), (722, 561), (519, 577)
SLOTS = [(165, 140), (400, 140), (635, 140), (165, 305), (400, 305), (635, 305), (165, 470), (400, 470), (635, 470)]
# the board's buttons and the in-game menu's items
B_FREE, B_UNDO, B_HINT, B_SHUFFLE, B_MENU, B_PAUSE = (212, 565), (297, 563), (503, 563), (605, 563), (740, 543), (740, 578)
GM_MAIN, GM_LAYOUT, GM_OPTIONS, GM_TILES, GM_BACKGROUNDS, GM_HELP, GM_RESUME = (548, 116), (548, 161), (548, 223), (548, 268), (548, 313), (548, 375), (548, 420)
TIP_OK = (402, 409)


def help_pages():
    click(*MENU_HELP, after=900)
    for _ in range(3):
        click(*HELP_NEXT, after=500)
    click(*HELP_CLOSE, after=600)


def options(from_menu):
    """The options dialog: both sliders, auto hint on, (from the menu) each quad percentage, OK. Full screen, wide
    screen and 3D are left alone: they change the machine's display."""
    click(*OPT_MUSIC, after=300)
    click(*OPT_SOUND, after=300)
    click(*OPT_HINT, after=300)
    if from_menu:
        for q in OPT_QUADS:
            click(*q, after=250)
    click(*OPT_OK[from_menu], after=700)


def tour():
    emit('focus', 'lose')
    wait(500)
    emit('focus', 'regain')
    wait(500)
    help_pages()
    # Options with no player: the name box opens first, and the options dialog after it
    click(*MENU_OPTIONS, after=900)
    emit('text', 'Cov')
    wait(300)
    key(VK_RETURN, after=1200)
    options(True)
    # the map, and the quit note: once with the check box set, once without
    click(*MENU_ADVENTURE, after=2500)
    click(*MAP_MENU, after=900)
    click(*QUIT_CHECK, after=300)
    click(*QUIT_OK, after=1500)
    click(*MENU_ADVENTURE, after=2500)
    click(*MAP_MENU, after=1500)
    # the classic list: pages, a preview, then the game
    click(*MENU_CLASSIC, after=2000)
    click(*CL_NEXT, after=800)
    click(*CL_PREV, after=800)
    click(*SLOTS[4], after=500)
    click(*CL_PLAY, after=3000)
    click(*TIP_OK, after=1500)
    click(*B_FREE, after=300)
    click(*B_FREE, after=300)
    click(*B_HINT, after=800)
    click(*B_SHUFFLE, after=1500)
    click(*B_PAUSE, after=800)
    emit('focus', 'lose')
    wait(500)
    emit('focus', 'regain')
    wait(500)
    click(400, 300, after=800)
    # the in-game menu: options, tile sets, backgrounds, help, resume
    click(*B_MENU, after=800)
    click(*GM_OPTIONS, after=800)
    options(False)
    click(*B_MENU, after=800)
    click(*GM_TILES, after=800)
    click(269, 455, after=400)
    click(412, 422, after=800)
    click(*B_MENU, after=800)
    click(*GM_BACKGROUNDS, after=800)
    click(334, 235, after=400)
    click(411, 425, after=800)
    click(*B_MENU, after=800)
    click(*GM_HELP, after=800)
    click(*HELP_NEXT, after=500)
    click(405, 428, after=600)
    click(*B_MENU, after=800)
    click(*GM_RESUME, after=800)
    # the in-game menu's layout item (the list again), then the main menu by the board's menu, then Quit
    click(*B_MENU, after=800)
    click(*GM_LAYOUT, after=1500)
    click(288, 577, after=2000)    # the list's Menu button
    click(*MENU_QUIT, after=3000)


def players():
    """The players dialog from the main menu: a new player from its list, the second one deleted (the question answered No,
    then Yes), OK. The list's rows are 23 pixels high from about y=240; the question box stands at 212,180."""
    new_player('Cov')
    click(*MAP_MENU, after=900)
    click(*QUIT_OK, after=2500)
    click(*PLAYERS_LABEL, after=900)       # the player's name at the foot of the menu opens the list
    click(340, 252, after=900)       # its first row, New Player
    emit('text', 'Two')
    wait(300)
    key(VK_RETURN, after=1500)
    click(340, 275, after=500)       # the second row
    click(244, 405, after=900)       # Delete
    click(452, 340, after=900)       # No
    click(244, 405, after=900)
    click(359, 340, after=1200)      # Yes
    click(354, 405, after=1500)      # OK
    click(*PLAYERS_LABEL, after=900)
    click(464, 405, after=1500)      # Cancel
    click(*MENU_QUIT, after=3000)


# the escape board's own buttons (the classic board's have other commands and places) and its game menu's items
E_UNDO, E_HINT, E_SHUFFLE, E_MENU, E_PAUSE = (296, 568), (501, 568), (603, 568), (732, 547), (732, 581)
EM_MAIN, EM_OPTIONS, EM_TILES, EM_HELP, EM_RESUME = (600, 206), (600, 268), (600, 313), (600, 375), (600, 420)
YES, NO = (353, 284), (446, 284)    # the escape menu's question


def escape_probe(sol):
    """Stage 1 for a few moves, then every button of the escape board and its game menu: undo (and the pair again), hint,
    the menu's options, tile sets and help, pause and resume, Main Menu answered No and then Yes (the game is saved),
    and the map again with its saved game."""
    new_player('Esc')
    stage(sol, 1, 4)
    click(*E_UNDO, after=1500)
    for m in [l.split() for l in open(os.path.join(sol, 's1.txt'))][3:4]:
        click(int(m[0]), int(m[1]), after=300)
        click(int(m[2]), int(m[3]), after=1250)
    click(*E_HINT, after=1500)
    click(*E_MENU, after=900)
    click(*EM_OPTIONS, after=900)
    options(False)
    click(*E_MENU, after=900)
    click(*EM_TILES, after=900)
    click(269, 455, after=400)
    click(412, 422, after=900)
    click(*E_MENU, after=900)
    click(*EM_HELP, after=900)
    click(*HELP_NEXT, after=500)
    click(405, 437, after=900)
    click(*E_MENU, after=900)
    click(*EM_RESUME, after=900)
    click(*E_PAUSE, after=900)
    click(400, 300, after=900)
    click(*E_MENU, after=900)
    click(*EM_MAIN, after=900)
    click(*NO, after=900)
    click(*E_MENU, after=900)
    click(*EM_MAIN, after=900)
    click(*YES, after=3000)
    click(*MENU_ADVENTURE, after=6000)    # the map, with the saved game of level 1
    click(*MAP_PLAY, after=4000)
    click(*TIP_OK, after=1500)
    emit('focus', 'lose')
    wait(500)
    emit('focus', 'regain')
    wait(500)


RESUME_YES, RESUME_NO = (276, 350), (404, 350)    # the question the map asks when the level has a saved game


def pairs(sol, st, first, last):
    """The solver's moves first..last-1 of a stage, as clicks, on a board already shown"""
    for m in [l.split() for l in open(os.path.join(sol, 's%d.txt' % st)) if l[0].isdigit() or l[0] == '-'][first:last]:
        click(int(m[0]), int(m[1]), after=300)
        click(int(m[2]), int(m[3]), after=1250)


def leave_saving():
    """The escape menu's Main Menu, answered Yes (the board is saved), and Adventure again to the map"""
    click(*E_MENU, after=900)
    click(*EM_MAIN, after=900)
    click(*YES, after=3000)
    click(*MENU_ADVENTURE, after=6000)


def resume_probe(sol):
    """A level left with a saved game: Play on the map asks, and Yes resumes it (PlayLayer::LoadSave); leaving it saved
    again, No starts the level over (the save is deleted)"""
    new_player('Res')
    stage(sol, 1, 3)
    leave_saving()
    click(*MAP_PLAY, after=1500)
    click(*RESUME_YES, after=4000)
    pairs(sol, 1, 3, 5)
    leave_saving()
    click(*MAP_PLAY, after=1500)
    click(*RESUME_NO, after=3000)
    click(*START_TIP[1][:2], after=1000)
    wait(1000)
    pairs(sol, 1, 0, 2)
    click(*E_UNDO, after=1500)
    click(*E_SHUFFLE, after=1500)


RESTART_YES, RESTART_NO = (353, 368), (446, 368)    # the question after Restart
RESTART_BOX, RESTART_DONT = (350, 363), (450, 363)    # the box the map of a won adventure opens with


def end_box():
    """From a player whose adventure is won (progress 60): the map opens with the restart box. Don't leaves it; opened
    again, Restart asks, No leaves the progress alone, and Restart, Yes starts the adventure over. Ancient China has no
    restart box: its map has a button for the ending instead, and the ending's second button goes to the main menu."""
    if END_BUTTON is not None:
        click(*MENU_ADVENTURE, after=6000)
        click(*END_BUTTON, after=9000)
        click(*END_MENU, after=3000)
        click(*MENU_QUIT, after=3000)
        return
    click(*MENU_ADVENTURE, after=6000)
    click(*RESTART_DONT, after=1500)
    click(*MAP_MENU, after=900)
    click(*QUIT_OK, after=2500)
    click(*MENU_ADVENTURE, after=6000)
    click(*RESTART_BOX, after=900)
    click(*RESTART_NO, after=900)
    click(*RESTART_BOX, after=900)
    click(*RESTART_YES, after=3000)
    click(*RESTART_DONT, after=1500)
    click(*MAP_MENU, after=900)
    click(*QUIT_OK, after=2500)
    click(*MENU_QUIT, after=3000)


def classic_moves(sol):
    return [l.split() for l in open(os.path.join(sol, 'classic.txt')) if l[0].isdigit() or l[0] == '-']


def classic_start(resume):
    """The classic list's middle preview of the first page, Play (a resumed game answers Yes to the question)"""
    click(*MENU_CLASSIC, after=2000)
    click(*SLOTS[4], after=500)
    click(*CL_PLAY, after=3000 if not resume else 1500)
    if resume:
        click(*RESUME_YES, after=4000)
    else:
        click(*TIP_OK, after=1500)


def classic_pairs(moves):
    for m in moves:
        click(int(m[0]), int(m[1]), after=300)
        click(int(m[2]), int(m[3]), after=1250)


def classic_save(sol, count):
    """A classic game of a pinned deal (the original's seed patched, docs/ORIGRUN.md), played for <count> moves of the
    port's solution and left by the board's menu, Main Menu, answered Yes: the game is saved (<player>_<layout>.sav)"""
    new_player('Cls')
    click(*MAP_MENU, after=900)
    click(*QUIT_OK, after=2500)
    classic_start(False)
    classic_pairs(classic_moves(sol)[:count])
    click(*B_MENU, after=900)
    click(*GM_MAIN, after=900)
    click(*YES, after=3000)
    click(*MENU_QUIT, after=3000)


def classic_play(sol, count):
    """The saved classic game resumed (the question after Play answered Yes) and played to its end with the rest of the
    solution; the result box's second button plays the layout again, and the board's menu leaves it, saving"""
    classic_start(True)
    moves = classic_moves(sol)[count:]
    classic_pairs(moves)
    wait(20000 + 300 * 2 * len(moves))    # the tiles left fly off, then the result box
    click(*RESULT_AGAIN, after=3000)
    click(*B_MENU, after=900)
    click(*GM_MAIN, after=900)
    click(*YES, after=3000)
    click(*MENU_QUIT, after=3000)


RESULT_AGAIN = (402, 275)


if GAME == 'mahjongchina':
    # Ancient China's buttons (the port's `state` lists them): the menu and map sit a few pixels elsewhere, the board's
    # buttons are 5 pixels lower, and its tips come on other stages (adventure.py of games/mahjongchina)
    MENU_ADVENTURE, MENU_CLASSIC, MENU_OPTIONS, MENU_HELP, MENU_QUIT = (405, 265), (404, 363), (235, 544), (407, 544), (579, 544)
    PLAYERS_LABEL = (400, 498)
    END_CONTINUE, END_MENU, END_BUTTON = (86, 565), (723, 565), (404, 566)
    LEVEL_DONE_OK = (401, 392)
    MAP_MENU, MAP_PLAY = (73, 566), (729, 566)
    START_TIP = {1: (400, 407, 0), 2: (312, 433, 0), 4: (192, 314, 300), 8: (327, 255, 300), 13: (392, 341, 300)}
    TIMED_TIP = {1: (187, 330), 7: (566, 440), 5: (400, 245)}
    HELP_NEXT, HELP_CLOSE = (566, 441), (403, 443)
    CL_PREV, CL_NEXT, CL_PLAY = (133, 550), (665, 550), (491, 571)
    B_FREE, B_UNDO, B_HINT, B_SHUFFLE, B_MENU, B_PAUSE = (228, 563), (317, 566), (514, 566), (607, 566), (736, 547), (736, 582)
    E_UNDO, E_HINT, E_SHUFFLE, E_MENU, E_PAUSE = (317, 566), (514, 566), (607, 566), (736, 547), (736, 582)


def main():
    kind = sys.argv[1]
    wait(9000)    # the two logos and the fades to the main menu
    if kind == 'escprobe':
        escape_probe(sys.argv[2])
        wait(1000)
        emit('quit')
        print('\n'.join(out))
        return
    if kind == 'endbox':
        end_box()
        wait(1000)
        emit('quit')
        print('\n'.join(out))
        return
    if kind == 'resume':
        resume_probe(sys.argv[2])
        wait(1000)
        emit('quit')
        print('\n'.join(out))
        return
    if kind == 'classicsave':
        classic_save(sys.argv[2], int(sys.argv[3]))
        print('\n'.join(out))
        return
    if kind == 'classicplay':
        classic_play(sys.argv[2], int(sys.argv[3]))
        print('\n'.join(out))
        return
    if kind == 'players':
        players()
        print('\n'.join(out))
        return
    if kind == 'tour':
        tour()
        print('\n'.join(out))
        return
    if kind == 'adventure':
        sol, first, last = sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
        name = sys.argv[sys.argv.index('--new') + 1] if '--new' in sys.argv else None
        if name:
            new_player(name)
        else:
            click(*MENU_ADVENTURE, after=6000)    # the saved player's map
        limit = int(sys.argv[sys.argv.index('--moves') + 1]) if '--moves' in sys.argv else None
        for st in range(first, last + 1):
            stage(sol, st, limit)
        if '--ending' in sys.argv:
            # after the last level: the ending's pictures (about 16 s), Continue to the map, the map's menu (the quit note) and Quit
            wait(25000)
            click(*END_CONTINUE, after=3000)
            click(*MAP_MENU, after=900)
            click(*QUIT_OK, after=2000)
            click(*MENU_QUIT, after=3000)
        wait(2000)
        emit('quit')
    print('\n'.join(out))


if __name__ == '__main__':
    main()
