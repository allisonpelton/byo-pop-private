"""Classic Bash from the loading screen, as classic1, but the game starts at level 15: the original through a patched copy (the
constructor of GameScreen loads the level the cheat word jumpto15 would; idplay.patches_file), the port through the `cheat`
command. The bot's clicks from the first level's introduction on."""

SEED = 1
END = 28000
JUMP_LEVEL = 15

PREFIX = [
    (1000, 'move 397 532', 'move 397 532'),
    (1005, 'click 397 532', 'click 397 532'),                  # the loading screen's "Click to Play"
    (1500, 'text T', 'char T'),
    (1510, 'text e', 'char e'),
    (1520, 'text s', 'char s'),
    (1530, 'text t', 'char t'),
    (1600, 'move 310 392', 'move 310 392'),
    (1605, 'click 310 392', 'click 310 392'),                  # OK in the new-name box
    (1700, '', 'cheat jumpto15'),                              # after the main menu is made (it clears the cheats)
    (2000, 'move 140 320', 'move 140 320'),
    (2005, 'click 140 320', 'click 140 320'),                  # Classic Bash
    (2500, 'move 300 550', 'move 300 550'),
    (2505, 'click 300 550', 'click 300 550'),                  # "Click here to play" on the how-to-play page
]

BOT_FROM = 3000
BOT_STEP = 10
BOT_SEED = 15
