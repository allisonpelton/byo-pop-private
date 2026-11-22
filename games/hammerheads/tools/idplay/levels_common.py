"""The inputs of the level-introduction scenarios (scr_lv1 ... scr_lv5, SCREENS.md): the Classic Bash of a player whose restart points
start the game on each of a run of levels (mkprofile.py `lv1` ... `lv5`). Each cycle chooses a point (with one heart), shoots the level's
introduction, the Ready box, the board a second into the level and, when the heart is gone, the game over box, whose Main Menu leads
back to the menu for the next point.

A resurrect screen with `count` buttons lays them three to a row: with up to three they are in one row in the middle of the window,
with more the first row is on top and the rest under it (ResurrectScreenWidget::Resize), centred."""

CYCLE = 13000
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


AWARD_LEVELS = (6, 11)             # levels that begin with a trophy won (the player of lv1 and lv2 has none; the rest have them all)


def make(first_level, last_level):
    count = last_level - first_level + 2                              # the points, and the New Game that heads the list
    inputs = click(1000, 397, 532)                                   # the loading screen's "Click to Play"
    names = {}
    centres = button_centres(count)
    for k in range(count - 1):
        level = first_level + k
        t = 2000 + k * CYCLE
        inputs += click(t, 140, 320)                                 # Classic Bash
        if k == 0:
            names[t + 2400] = 'points_%d' % count                    # the choice of points
        inputs += click(t + 3000, *centres[k + 1])                   # the point that starts level `level`
        names[t + 4500] = 'intro_level%02d' % level
        inputs += click(t + 5000, 400, 438)                          # Go, on the introduction
        if level in AWARD_LEVELS:
            names[t + 5500] = 'award_level%02d' % level             # the page of a trophy won
            inputs += click(t + 5700, 400, 550)                      # "Click to Continue"
            names[t + 6500] = 'trophyroom_level%02d' % level        # the Trophy Room, with its Back to Game button
            inputs += click(t + 7000, 400, 570)                      # "Back to Game"
            t += 2500
        if level in AWARD_LEVELS:                                    # the Ready box comes back with the game from the Trophy Room
            names[t + 5500] = 'ready_level%02d' % level
            inputs += click(t + 5600, 400, 344)                      # Go, on the Ready box
        names[t + 6600] = 'board_level%02d' % level
        names[t + 10600] = 'over_level%02d' % level                  # the one heart is gone and the game over box is up
        inputs += click(t + 11000, 522, 470)                         # "Main Menu"
    return inputs, names, 2000 + (count - 1) * CYCLE + 2500 * len([n for n in range(first_level, last_level + 1) if n in AWARD_LEVELS])
