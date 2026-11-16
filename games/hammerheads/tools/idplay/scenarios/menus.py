"""The menus' own boxes and screens, for the looks pass: a new player named Test, then the Options box, its Credits box, the High
Scores screen and the Trophy Room, each shot after it opens and after it closes. No game is played."""

SEED = 1
END = 6500


def click(count, x, y):
    return [(count, 'move %d %d' % (x, y), 'move %d %d' % (x, y)),
            (count + 5, 'click %d %d' % (x, y), 'click %d %d' % (x, y))]


INPUTS = [
    (1000, 'move 397 532', 'move 397 532'),
    (1005, 'click 397 532', 'click 397 532'),                  # the loading screen's "Click to Play"
    (1500, 'text T', 'char T'),
    (1510, 'text e', 'char e'),
    (1520, 'text s', 'char s'),
    (1530, 'text t', 'char t'),
]
INPUTS += click(1600, 310, 392)                                # OK in the new-name box
INPUTS += [(2000, 'move 190 480', 'move 310 480'),                # Options (the original's row has More Games, which the port leaves out)
           (2005, 'click 190 480', 'click 310 480')]
INPUTS += click(2500, 400, 457)                                # Credits
INPUTS += click(3000, 380, 544)                                # Ok on the credits
INPUTS += click(3500, 400, 512)                                # Done
INPUTS += click(4000, 50, 540)                                 # High Scores
INPUTS += click(4500, 400, 570)                                # Main Menu
INPUTS += click(5000, 740, 540)                                # Trophy Room
INPUTS += click(5500, 400, 570)                                # Back to Main Menu
