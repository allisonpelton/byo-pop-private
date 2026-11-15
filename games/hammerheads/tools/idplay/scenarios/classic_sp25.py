"""Classic Bash from the loading screen with the two damage cheats and `superpower` on (a hammer of level 3 on fire, 7 hearts, 50000
coins), started at level 25, so that the bot's game is not lost and level 25's king (about 3000 health) can be brought down and the
game won. The original runs through a patched copy (GetCheatFlag's three tests, "nomissdamage" at 0x44a2c5, "noawaydamage" at
0x44a39c and "superpower" at 0x42f0d2, are made true: `test al, al` becomes `or al, 1`; the game screen's constructor loads
level 25), the port through the `cheat` command (typed after the main menu is made, which clears the cheats). The bot's clicks
from the first level's introduction on, and the won game's "Main Menu" button."""

SEED = 1
END = 33000
JUMP_LEVEL = 25

PATCHES = [
    (0x44a2c5, '84c0', '0c01', 'the cheat "nomissdamage" is on: test al, al becomes or al, 1, so the jne after it is always taken'),
    (0x44a39c, '84c0', '0c01', 'the cheat "noawaydamage" is on: the same'),
    (0x42f0d2, '84c0', '0c01', 'the cheat "superpower" is on in Stats::Reset: the same'),
]

PREFIX = [
    (1000, 'move 397 532', 'move 397 532'),
    (1005, 'click 397 532', 'click 397 532'),                  # the loading screen's "Click to Play"
    (1500, 'text T', 'char T'),
    (1510, 'text e', 'char e'),
    (1520, 'text s', 'char s'),
    (1530, 'text t', 'char t'),
    (1600, 'move 310 392', 'move 310 392'),
    (1605, 'click 310 392', 'click 310 392'),                  # OK in the new-name box
    (1700, '', 'cheat nomissdamage'),                          # after the main menu is made (it clears the cheats)
    (1705, '', 'cheat noawaydamage'),
    (1710, '', 'cheat superpower'),
    (1715, '', 'cheat jumpto25'),
    (2000, 'move 140 320', 'move 140 320'),
    (2005, 'click 140 320', 'click 140 320'),                  # Classic Bash
    (2500, 'move 300 550', 'move 300 550'),
    (2505, 'click 300 550', 'click 300 550'),                  # "Click here to play" on the how-to-play page
]

BOT_FROM = 3000
BOT_STEP = 10
BOT_SEED = 66
