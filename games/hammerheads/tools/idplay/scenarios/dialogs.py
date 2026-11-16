"""The boxes the other scenarios do not open, for the coverage check: the change-player box, the quit box (No, then Yes is left to
the end), the options box's sliders. No game is played."""

SEED = 1
END = 7000


def click(count, x, y, port=None):
    px, py = port or (x, y)
    return [(count, 'move %d %d' % (x, y), 'move %d %d' % (px, py)),
            (count + 5, 'click %d %d' % (x, y), 'click %d %d' % (px, py))]


INPUTS = [
    (1000, 'move 397 532', 'move 397 532'),
    (1005, 'click 397 532', 'click 397 532'),
    (1500, 'text T', 'char T'),
    (1510, 'text e', 'char e'),
    (1520, 'text s', 'char s'),
    (1530, 'text t', 'char t'),
]
INPUTS += click(1600, 310, 392)
INPUTS += click(2000, 400, 326)             # "If this is not you, click here": the change-player box
INPUTS += click(3000, 490, 464)             # Cancel on it
INPUTS += click(4000, 480, 480, (480, 480))  # Quit on the main menu
INPUTS += click(5000, 525, 520)             # No on the quit box
INPUTS += click(5500, 480, 480)             # Quit again
INPUTS += click(6000, 270, 520)             # Yes: the game ends
