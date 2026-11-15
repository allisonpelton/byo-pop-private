"""A probe of the original's sound at a button: the loading screen, the name, then the OK of the new-name box is moved to, moved to
again at the same place, pressed and let go at different updates, to see which message plays the mouse-over sound."""

SEED = 1
END = 1800

INPUTS = [
    (1000, 'move 397 532', 'move 397 532'),
    (1005, 'click 397 532', 'click 397 532'),
    (1500, 'text T', 'char T'),
    (1510, 'text e', 'char e'),
    (1520, 'text s', 'char s'),
    (1530, 'text t', 'char t'),
    (1600, 'move 310 392', 'move 310 392'),
    (1620, 'move 310 392', 'move 310 392'),
    (1640, 'down 310 392', None),
    (1660, 'up 310 392', None),
]
