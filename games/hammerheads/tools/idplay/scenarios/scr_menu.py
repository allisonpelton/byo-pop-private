"""The name and player boxes and the main menu's questions, for the screens pass (SCREENS.md): the first name box empty and with a
name typed, the player list, its "(Create a New User)" row opening the other name box, Rename, Delete's "Are You Sure?", Quit's
question and its No. No game is played. One shot a stage, taken a little after each opens (SHOTS)."""

SEED = 1
END = 14000


def click(count, x, y):
    return [(count, 'move %d %d' % (x, y), 'move %d %d' % (x, y)),
            (count + 5, 'click %d %d' % (x, y), 'click %d %d' % (x, y))]


def text(count, word):
    return [(count + 10 * i, 'text %s' % c, 'char %s' % c) for i, c in enumerate(word)]


INPUTS = click(1000, 397, 532)                                  # the loading screen's "Click to Play"
INPUTS += text(1500, 'Test')                                    # the first name box, typed
INPUTS += click(1600, 310, 392)                                 # OK
INPUTS += click(2000, 400, 326)                                 # "If this is not you, click here": the player list
INPUTS += click(2500, 400, 200)                                 # the row "(Create a New User)"
INPUTS += click(3000, 310, 466)                                 # OK: the other name box
INPUTS += text(3500, 'Ann')
INPUTS += click(3700, 310, 392)                                 # OK: Ann is the player
INPUTS += click(4000, 400, 326)                                 # the player list, with two players
INPUTS += click(4500, 310, 381)                                 # Rename
INPUTS += click(5500, 490, 392)                                 # Cancel
INPUTS += click(6000, 490, 381)                                 # Delete
INPUTS += click(7000, 520, 350)                                 # No
INPUTS += click(7500, 490, 381)                                 # Delete again
INPUTS += click(8000, 310, 350)                                 # Yes: Ann is removed
INPUTS += click(8500, 490, 466)                                 # Cancel on the list
# Quit on the main menu: the original has "More Games" between Options and Quit (never clicked), so Quit is further right than in the port
INPUTS += [(9500, 'move 600 480', 'move 480 480'), (9505, 'click 600 480', 'click 480 480')]
INPUTS += click(10500, 525, 520)                                # No

NAMES = {1400: 'name_empty', 1590: 'name_typed', 1900: 'menu_welcome', 2400: 'players_one', 2900: 'name_other', 3900: 'menu_two_players',
         4400: 'players_two', 5400: 'rename_box', 5900: 'players_after_rename', 6900: 'delete_sure', 7400: 'players_after_no',
         8400: 'players_after_delete', 9000: 'menu_after_delete', 10000: 'quit_box', 11000: 'menu_after_no'}
SHOTS = sorted(NAMES)
END = 12000
