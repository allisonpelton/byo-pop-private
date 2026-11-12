# a new player: the name Test typed and entered, then the tutorial's first lessons with no shot fired. The original's `text`
# The name box reads key downs (a WM_CHAR is not taken), so the keys go in as key edges.
INPUTS = [
    (0, 'keydown 0x54', 'keydown T'),
    (2, 'keyup 0x54', 'keyup T'),
    (10, 'keydown 0x45', 'keydown E'),
    (12, 'keyup 0x45', 'keyup E'),
    (20, 'keydown 0x53', 'keydown S'),
    (22, 'keyup 0x53', 'keyup S'),
    (30, 'keydown 0x54', 'keydown T'),
    (32, 'keyup 0x54', 'keyup T'),
    (200, 'keydown 0x0D', 'keydown RETURN'),
    (204, 'keyup 0x0D', 'keyup RETURN'),
]
END = 2800  # frames from the start of the inputs
