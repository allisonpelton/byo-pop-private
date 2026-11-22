"""The inputs of the level-introduction scenarios (scr_lv1 ... scr_lv5, SCREENS.md): the Classic Bash of a player whose restart points
start the game on each of a run of levels (mkprofile.py `lv1` ... `lv5`). Each cycle chooses a point, shoots the level's introduction,
the Ready box and the board a second into the level, then leaves the game through the game menu (the game is saved, so the next
Classic Bash offers it and its New Game goes back to the choice of points).

A resurrect screen with `count` buttons lays them three to a row: with up to three they are in one row in the middle of the window,
with more the first row is on top and the rest under it (ResurrectScreenWidget::Resize), centred."""

CYCLE = 11000
ROW_ONE = 200                       # the middle of the buttons of the top row of two, and of the lower row
ROW_TWO = 450
ROW_ALONE = 274                     # the middle of a single row


def click(count, x, y):
    return [(count, 'move %d %d' % (x, y), 'move %d %d' % (x, y)),
            (count + 5, 'click %d %d' % (x, y), 'click %d %d' % (x, y))]


def button_centres(count):
    """Where the buttons of a resurrect screen with count points are, in order."""
    def row(n):
        return {1: [400], 2: [275, 525], 3: [150, 400, 650]}[n]
    if count <= 3:
        return [(x, ROW_ALONE) for x in row(count)]
    return [(x, ROW_ONE) for x in row(3)] + [(x, ROW_TWO) for x in row(count - 3)]


def make(first_level, last_level):
    count = last_level - first_level + 1
    inputs = click(1000, 397, 532)                                   # the loading screen's "Click to Play"
    names = {}
    centres = button_centres(count)
    for k in range(count):
        level = first_level + k
        t = 2000 + k * CYCLE
        inputs += click(t, 140, 320)                                 # Classic Bash
        if k > 0:
            inputs += click(t + 1500, 490, 390)                      # "New Game" on the box that offers the saved game
            names[t + 1200] = 'continue_%02d' % level if k == 1 else None
        if k == 0 or k == 1:
            names[t + 2400] = 'points_%d' % count if k == 0 else None
        inputs += click(t + 3000, *centres[k])                       # the point that starts level `level`
        names[t + 4500] = 'intro_level%02d' % level
        inputs += click(t + 5000, 400, 438)                          # Go, on the introduction
        names[t + 5500] = 'ready_level%02d' % level
        inputs += click(t + 5600, 400, 344)                          # Go, on the Ready box
        names[t + 6600] = 'board_level%02d' % level
        inputs += click(t + 7500, 750, 30)                           # the in-game menu's button
        inputs += click(t + 8500, 400, 235)                          # "Main Menu"
        inputs += click(t + 9500, 300, 365)                          # "Yes": leave the game
    names = {u: n for u, n in names.items() if n}
    return inputs, names, 2000 + count * CYCLE
