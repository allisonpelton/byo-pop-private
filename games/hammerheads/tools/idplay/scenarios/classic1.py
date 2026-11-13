"""Classic Bash from the loading screen: a new player named Test, the how-to-play page, the first level's introduction, and the
bot's clicks until the level ends (or the game is lost)."""

SEED = 1
END = 30000

# (update count, origrun command, port command); a click is moved to first, and a name is typed a character at a time
PREFIX = [
    (1000, 'move 397 532', 'move 397 532'),
    (1005, 'click 397 532', 'click 397 532'),                  # the loading screen's "Click to Play"
    (1500, 'text T', 'char T'),
    (1510, 'text e', 'char e'),
    (1520, 'text s', 'char s'),
    (1530, 'text t', 'char t'),
    (1600, 'move 310 392', 'move 310 392'),
    (1605, 'click 310 392', 'click 310 392'),                  # OK in the new-name box
    (2000, 'move 140 320', 'move 140 320'),
    (2005, 'click 140 320', 'click 140 320'),                  # Classic Bash
    (2500, 'move 300 550', 'move 300 550'),
    (2505, 'click 300 550', 'click 300 550'),                  # "Click here to play" on the how-to-play page
    (3000, 'move 400 438', 'move 400 438'),
    (3005, 'click 400 438', 'click 400 438'),                  # Go, on the first level's introduction
]

BOT_FROM = 3500
BOT_STEP = 10
BOT_SEED = 1
