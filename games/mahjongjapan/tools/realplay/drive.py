"""Plays Mahjong Escape: Ancient Japan or Ancient China with real input only (CLAUDE.md, "Done means identical"): every
click and key goes through -realinput into SDL's own queue and takes a hand's path from there (docs/REAL_INPUT.md). The
port's own bot chooses every pair (the `move` test command, docs/TEST_COMMANDS.md); this script sends the input the
move names over the live channel (-commands) and checks what the game's `state` says next.

    build/native/rundesk.exe python games/mahjongjapan/tools/realplay/drive.py <out dir> --game japan|china
        [--levels N] [--classic N] [--fullscreen] [--realtime] [--exe E]

The tour: the launch (splash, main menu), a focus loss and regain, the options and the help pages, a new player's name,
then the adventure from its map: levels 1 to N (default all of them) each on the bot's moves, every tip and level's end
answered by the click `move` names, the board's buttons tried in the first level (hint, undo, shuffle, pause, the
in-game menu), and after the last level the ending; then N classic games (default 2: the first by the bot to the end,
the second ended by the `win` test command after ten real pairs). With --fullscreen the window goes full screen by
Alt+Enter before the games and back after them. The log is <out dir>/drive.log, the game's own log <out dir>/game.log,
its screenshots <out dir>/*.png; the last line of the log, and of stdout, is "RESULT ok" or "RESULT FAILED: <why>".
It runs with -fast unless --realtime.
"""
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(__file__))
from mjgame import Failed, Game, T0  # noqa: E402

GAMES = {'japan': ('build/native/mahjongjapan.exe', 'assets/mahjongjapan'),
         'china': ('build/native/mahjongchina.exe', 'assets/mahjongchina')}

# the main menu's buttons (MainMenu::OnCommand): their commands
MENU_ADVENTURE, MENU_CLASSIC, MENU_OPTIONS, MENU_HELP, MENU_QUIT = 0x8e, 0x84, 0x83, 0x87, 0x86
MAP_MENU, MAP_CONTINUE = 0xe4, 0xe6
# the classic list's buttons (LayoutSelect), the board's (PlayLayer) and the in-game menu's (GameMenu)
CL_PREV, CL_NEXT, CL_MENU, CL_PLAY = 0xc1, 0xc2, 0xc5, 0xc4
BOARD_UNDO, BOARD_HINT, BOARD_SHUFFLE, BOARD_MENU, BOARD_PAUSE, BOARD_FREE = 0x10d, 0x108, 0x10c, 0x109, 0x10a, 0x16f
GM_MAIN, GM_LAYOUT, GM_OPTIONS, GM_TILES, GM_BACKGROUNDS, GM_HELP, GM_RESUME = 11, 12, 15, 16, 17, 14, 18
# the classic list's nine previews, by their centres
SLOTS = [(165, 140), (400, 140), (635, 140), (165, 305), (400, 305), (635, 305), (165, 470), (400, 470), (635, 470)]


def buttons(st):
    return {b['cmd']: (b['x'], b['y']) for b in st['buttons']}


def press(g, st, cmd):
    """Clicks the button with this command, where the state says it is"""
    b = buttons(st)
    if cmd not in b:
        raise Failed('no button 0x%x on %s/%s: %s' % (cmd, st['screen'], st['dialog'], json.dumps(st['buttons'])))
    g.hover(*b[cmd])
    g.click(*b[cmd])


def map_ready(g, timeout=120.0):
    """The map with its Continue button enabled (the marker has stopped walking)"""
    return g.until('the map ready', lambda s: s['screen'] == 'map' and s['dialog'] is None and MAP_CONTINUE in buttons(s), timeout)


def to_screen(g, screen, timeout=120.0):
    return g.until('screen ' + screen, lambda s: s['screen'] == screen and s['dialog'] is None and not s['busy'], timeout)


def note(g, st, tag):
    g.say('%s: screen %s dialog %s level %s score %s timer %s moves %s tiles %s buttons %s' % (
        tag, st['screen'], st['dialog'], st.get('level'), st.get('score'), st.get('timer'), st.get('moves'),
        st.get('tiles'), [hex(b['cmd']) for b in st['buttons']]))


def focus(g, where):
    g.ask('focus lost')
    g.wait(10)
    g.shot('%s-focus-lost' % where)
    g.ask('focus gained')
    g.wait(10)
    st = g.state()
    g.say('focus %s: lost and regained, screen %s dialog %s' % (where, st['screen'], st['dialog']))
    return st


def toggle_fullscreen(g, on):
    g.ask('keydown Left Alt')
    g.ask('key Return')
    g.ask('keyup Left Alt')
    g.wait(30)
    time.sleep(2)    # the window's change of mode is the system's, in real time even with -fast
    # the whole window as the display shows it: its size tells which mode the game is in
    name = 'window-fullscreen-%s.png' % ('on' if on else 'off')
    path = os.path.join(g.out, name)
    g.ask('screenshotfull ' + path.replace(os.sep, '/'))
    with open(path, 'rb') as f:
        head = f.read(24)
    w, h = int.from_bytes(head[16:20], 'big'), int.from_bytes(head[20:24], 'big')
    g.say('full screen %s: the window is %dx%d' % ('on' if on else 'off', w, h))


def dialog_ok(g, name, timeout=20.0):
    """A dialog on top: waits until it settles and clicks the button `move` names for it"""
    end = time.time() + timeout
    while time.time() < end:
        st = g.state()
        if st['dialog'] != name:
            return
        if not st['busy']:
            mv = g.ask('move')
            if mv is not None:
                g.send(mv)
                g.wait(10)
                continue
        g.wait(3)
    raise Failed('the %s did not close' % name)


def tour_menu(g):
    st = to_screen(g, 'menu')
    g.shot('menu')
    note(g, st, 'menu')
    focus(g, 'menu')
    # the options need a player, so the tour of the dialogs comes after the name; here the help pages
    press(g, st, MENU_HELP)
    st = g.until('the help', lambda s: s['dialog'] == 'help' and not s['busy'])
    g.shot('help0')
    note(g, st, 'help')
    return st


def new_player(g, name):
    st = to_screen(g, 'menu')
    press(g, st, MENU_ADVENTURE)
    st = g.until('the name box', lambda s: s['dialog'] == 'newplayer' and not s['busy'])
    g.shot('newplayer')
    g.type(name)
    g.wait(5)
    g.key('Return')
    g.say('player %s made' % name)


def play_board(g, tag, win_after=None):
    """Plays the board in front of it to its end by the bot's moves. Returns the number of pairs."""
    pairs = 0
    stuck = 0
    seen = set()
    end = time.time() + 1800
    while time.time() < end:
        st = g.state()
        sig = (st['screen'], st['dialog'])
        if sig not in seen:
            seen.add(sig)
            note(g, st, tag + ' new screen')
            g.shot('%s-%s-%s' % (tag, st['screen'].replace(' ', '_'), st['dialog'] or 'none'))
        if st['screen'] not in ('escape', 'classic'):
            return pairs, st
        if st['dialog'] is not None:
            if st['dialog'] in ('tip', 'no moves', 'result', 'pause'):
                dialog_ok(g, st['dialog'])
                continue
            return pairs, st
        if st['busy']:
            g.wait(2)
            continue
        if win_after is not None and pairs == win_after:
            g.ask('win')
            g.say('%s: won by the test command after %d pairs' % (tag, pairs))
            win_after = None
            g.wait(20)
            continue
        mv = g.ask('move')
        if mv is None:
            stuck += 1
            if 200 < stuck:
                raise Failed('%s: no move for a long time: %s' % (tag, json.dumps(st)))
            g.wait(3)
            continue
        stuck = 0
        g.send(mv)
        if mv['kind'] in ('pair', 'special'):
            pairs += 1
        elif mv['kind'] == 'shuffle':
            g.say('%s: no pair free, shuffled' % tag)
        if pairs % 25 == 0 and mv['kind'] in ('pair', 'special'):
            g.say('%s: %d pairs, %d tiles left, score %d, timer %d' % (tag, pairs, st['tiles'], st['score'], st['timer']))
        g.wait(1)
    raise Failed('%s did not end' % tag)


def adventure(g, levels):
    """The adventure from the map: Continue, the level on the bot's moves, its end, the map again"""
    st = map_ready(g)
    note(g, st, 'map')
    g.shot('map-start')
    for n in range(1, levels + 1):
        st = map_ready(g)
        if st['progress'] != n - 1:
            raise Failed('the map shows progress %d, expected %d' % (st['progress'], n - 1))
        press(g, st, MAP_CONTINUE)
        st = g.until('level %d' % n, lambda s: s['screen'] == 'escape' and s['dialog'] in (None, 'tip') and not s['busy'], 60)
        g.say('level %d begins: layout %s, %s tiles' % (n, st['layout'], st['tiles']))
        pairs, st = play_board(g, 'level%d' % n)
        g.say('level %d: %d pairs by the bot, then %s/%s, score %s' % (n, pairs, st['screen'], st['dialog'], st.get('score')))
        # the level's end: answered by the click `move` names, until the map shows (or the ending)
        end = time.time() + 120
        while time.time() < end:
            st = g.state()
            if st['screen'] == 'map' and st['dialog'] is None and MAP_CONTINUE in buttons(st):
                break
            if st['screen'] == 'ending':
                break
            if not st['busy'] and st['screen'] in ('level done', 'result') or st['dialog'] is not None:
                mv = g.ask('move')
                if mv is not None:
                    g.send(mv)
            g.wait(3)
        else:
            raise Failed('level %d: the map did not come back (%s)' % (n, json.dumps(st)))
        g.say('level %d won, now %s, progress %s' % (n, st['screen'], st['progress']))
    return st


def board_ready(g, screen):
    """The board with no dialog and nothing moving; a tip that opens with it is answered"""
    st = g.until('the %s board' % screen, lambda s: s['screen'] == screen and s['dialog'] in (None, 'tip') and not s['busy'], 60)
    if st['dialog'] == 'tip':
        dialog_ok(g, 'tip')
        st = g.until('the %s board' % screen, lambda s: s['screen'] == screen and s['dialog'] is None and not s['busy'], 60)
    return st


def open_game_menu(g):
    """The in-game menu closes itself when one of its dialogs opens: opens it again from the board's Menu button"""
    st = g.until('the board or its menu', lambda s: s['dialog'] in (None, 'game menu') and not s['busy'])
    if st['dialog'] is None:
        press(g, st, BOARD_MENU)


def menu_item(g, cmd, name):
    """An item of the in-game menu: opens its dialog, looks at it, closes it by the click `move` names"""
    open_game_menu(g)
    st = g.until('the game menu', lambda s: s['dialog'] == 'game menu' and not s['busy'])
    press(g, st, cmd)
    st = g.until('the %s' % name, lambda s: s['dialog'] == name and not s['busy'])
    g.shot('classic-%s' % name.replace(' ', '_'))
    note(g, st, name)
    dialog_ok(g, name)
    g.say('the %s closed' % name)


def board_tour(g):
    """Every button of a classic board, before the first pair: the free tiles option, hint, shuffle (its 20 s, which the
    original adds with Panel::AddSeconds in milliseconds when the clock is not running), pause, the in-game menu and
    its dialogs"""
    board_ready(g, 'classic')
    g.wait(60)
    st = board_ready(g, 'classic')    # the game's tip opens a moment after the board is first ready
    note(g, st, 'board')
    press(g, st, BOARD_FREE)
    g.wait(10)
    g.shot('classic-freetiles-off')
    press(g, g.state(), BOARD_FREE)
    g.wait(10)
    press(g, g.state(), BOARD_HINT)
    g.wait(30)
    g.shot('classic-hint')
    st = g.settle()
    before = st['timer']
    press(g, st, BOARD_SHUFFLE)
    g.wait(60)
    st = board_ready(g, 'classic')
    g.say('shuffle before the first pair: the timer shows %s (was %s)' % (st['timer'], before))
    if st['timer'] < 20 or 1000 < st['timer']:
        raise Failed('the shuffle should add 20 s to the clock, it shows %s' % st['timer'])
    press(g, st, BOARD_PAUSE)
    st = g.until('the pause', lambda s: s['dialog'] == 'pause' and not s['busy'])
    g.shot('classic-pause')
    g.hover(400, 300)
    g.click(400, 300)
    g.until('the pause closed', lambda s: s['dialog'] is None)
    press(g, g.state(), BOARD_MENU)
    menu_item(g, GM_OPTIONS, 'options')
    menu_item(g, GM_TILES, 'tile sets')
    menu_item(g, GM_BACKGROUNDS, 'backgrounds')
    menu_item(g, GM_HELP, 'help')
    open_game_menu(g)
    st = g.until('the game menu', lambda s: s['dialog'] == 'game menu' and not s['busy'])
    press(g, st, GM_RESUME)
    board_ready(g, 'classic')


def classic_game(g, n, slot, win_after=None):
    """A classic game from the main menu: the list, the layout's preview, Play Layout, the board to its end"""
    g.ask('goto menu')
    st = to_screen(g, 'menu')
    press(g, st, MENU_CLASSIC)
    st = to_screen(g, 'classic list')
    g.hover(*SLOTS[slot])
    g.click(*SLOTS[slot])
    g.wait(10)
    press(g, g.state(), CL_PLAY)
    st = board_ready(g, 'classic')
    g.say('classic game %d begins: layout %s, %s tiles, %s pairs free' % (n, st['layout'], st['tiles'], st['moves']))
    if n == 1:
        board_tour(g)
    pairs, st = play_board(g, 'classic%d' % n, win_after)
    g.say('classic game %d: %d pairs by the bot, then %s/%s, score %s' % (n, pairs, st['screen'], st['dialog'], st.get('score')))
    # the end: the result box answered by the click `move` names, until a screen without a board
    end = time.time() + 60
    while time.time() < end:
        st = g.state()
        if st['screen'] not in ('classic', 'escape'):
            break
        if not st['busy'] and st['dialog'] is not None:
            mv = g.ask('move')
            if mv is not None:
                g.send(mv)
        g.wait(3)
    g.say('classic game %d over: now %s/%s' % (n, st['screen'], st['dialog']))
    return st


def quit_game(g):
    """Quits by the main menu's own Quit button, as a player does: the game exits on that click (it asks nothing; a
    note, if one ever opened, would be answered by the click `move` names)"""
    g.ask('goto menu')
    st = to_screen(g, 'menu')
    press(g, st, MENU_QUIT)
    end = time.time() + 30
    noted = False
    while g.proc.poll() is None and time.time() < end:
        try:
            st = g.state()
            if st['dialog'] is not None and not st['busy']:
                if not noted:
                    note(g, st, 'quit')
                    g.shot('quit-note')
                    noted = True
                mv = g.ask('move')
                if mv is not None:
                    g.send(mv)
        except Failed:
            break    # the game exited while a call was waiting: the log reader saw its end
        time.sleep(0.1)
    try:
        g.proc.wait(30)
    except Exception:
        raise Failed('the game did not exit after Quit')
    g.say('Quit on the main menu: the game exited with code %s' % g.proc.returncode)
    if g.proc.returncode != 0:
        raise Failed('the game exited with code %s' % g.proc.returncode)


def main():
    args = sys.argv[1:]
    out, which, levels, classic, fullscreen, fast, exe = None, 'japan', 60, 2, False, True, None
    while args:
        a = args.pop(0)
        if a == '--game':
            which = args.pop(0)
        elif a == '--levels':
            levels = int(args.pop(0))
        elif a == '--classic':
            classic = int(args.pop(0))
        elif a == '--fullscreen':
            fullscreen = True
        elif a == '--realtime':
            fast = False
        elif a == '--exe':
            exe = args.pop(0)
        else:
            out = a
    default_exe, gamedir = GAMES[which]
    g = Game(out, os.path.abspath(exe or default_exe), gamedir, fast)
    result = 'ok'
    try:
        tour_menu(g)
        dialog_ok(g, 'help')
        new_player(g, 'Bot')
        if fullscreen:
            to_screen(g, 'map')
            toggle_fullscreen(g, True)
            g.shot('map-fullscreen')
        st = adventure(g, levels)
        g.shot('adventure-end')
        if classic >= 1:
            classic_game(g, 1, 0)
        if classic >= 2:
            classic_game(g, 2, 4, win_after=10)
        if fullscreen:
            toggle_fullscreen(g, False)
        if classic >= 1:
            quit_game(g)
    except Failed as e:
        result = 'FAILED: %s' % e
        try:
            g.shot('failed')
        except Failed:
            pass
    finally:
        g.close()
    g.say('RESULT ' + result)
    print('RESULT ' + result)
    return 0 if result == 'ok' else 1


if __name__ == '__main__':
    sys.exit(main())
