"""The name boxes' complaints, for the screens pass (SCREENS.md): a name left empty on the first box, a name already taken on the
other box, and a rename to a name already taken."""

SEED = 1
END = 12000


def click(count, x, y):
    return [(count, 'move %d %d' % (x, y), 'move %d %d' % (x, y)),
            (count + 5, 'click %d %d' % (x, y), 'click %d %d' % (x, y))]


def text(count, word):
    return [(count + 10 * i, 'text %s' % c, 'char %s' % c) for i, c in enumerate(word)]


INPUTS = click(1000, 397, 532)                                  # the loading screen's "Click to Play"
INPUTS += click(1500, 310, 392)                                 # OK on the first name box, left empty
INPUTS += click(2500, 400, 360)                                 # OK on the complaint
INPUTS += text(3000, 'Test')
INPUTS += click(3200, 310, 392)                                 # OK
INPUTS += click(3600, 400, 326)                                 # the player list
INPUTS += click(4000, 400, 200)                                 # the row "(Create a New User)"
INPUTS += click(4400, 310, 466)                                 # OK: the other name box
INPUTS += text(4800, 'Test')                                    # the name of the player there already
INPUTS += click(5000, 310, 392)                                 # OK
INPUTS += click(6000, 400, 360)                                 # OK on the complaint (the name stays in the box)
INPUTS += click(6400, 490, 392)                                 # Cancel
INPUTS += click(6800, 400, 200)                                 # the row "(Create a New User)"
INPUTS += click(7200, 310, 466)                                 # OK: the other name box
INPUTS += text(7600, 'Ann')
INPUTS += click(7800, 310, 392)                                 # OK: Ann is the player
INPUTS += click(8200, 400, 326)                                 # the player list, with two players
INPUTS += click(8600, 310, 381)                                 # Rename
INPUTS += text(9000, 'Test')                                    # over Ann's name, with the one that is taken
INPUTS += click(9200, 310, 392)                                 # OK
INPUTS += click(10000, 400, 360)                                # OK on the complaint
NAMES = {1900: 'error_name_needed', 5500: 'error_name_taken', 9700: 'error_rename_taken', 10500: 'rename_box_after_error'}
SHOTS = sorted(NAMES)
END = 12000
