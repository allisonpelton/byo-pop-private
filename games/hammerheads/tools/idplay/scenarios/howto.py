"""The how-to-play movie, from the Options box, for the frame comparison of the looks pass."""

SEED = 1
END = 12000


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
INPUTS += click(2500, 400, 348)                                # How To Play
