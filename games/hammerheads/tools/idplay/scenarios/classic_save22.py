"""As `classic_nd22` (the two damage cheats on, the game started at level 22 with its many kinds of creature: the multiplier gnome,
silvers, strongs, switchers, napoleons, bubble heads), left in the middle of the level (the in-game menu, "Main Menu", "Yes" of
"Leave Game?": the game is saved) and resumed from the main menu ("Classic Bash", "Continue" of the box that offers the saved
game). The main menu clears the cheats when it is made again, so the port is typed them again (the original's patches stay on);
the bot presses the "Ready?" box and plays on."""

SEED = 1
END = 22000
JUMP_LEVEL = 22

PATCHES = [
    (0x44a2c5, '84c0', '0c01', 'the cheat "nomissdamage" is on: test al, al becomes or al, 1, so the jne after it is always taken'),
    (0x44a39c, '84c0', '0c01', 'the cheat "noawaydamage" is on: the same'),
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
    (1710, '', 'startlevel 22'),
    (2000, 'move 140 320', 'move 140 320'),
    (2005, 'click 140 320', 'click 140 320'),                  # Classic Bash
    (2500, 'move 300 550', 'move 300 550'),
    (2505, 'click 300 550', 'click 300 550'),                  # "Click here to play" on the how-to-play page
    (9000, 'move 750 30', 'move 750 30'),
    (9005, 'click 750 30', 'click 750 30'),                    # the in-game menu's button
    (9100, 'move 400 235', 'move 400 235'),
    (9105, 'click 400 235', 'click 400 235'),                  # "Main Menu"
    (9200, 'move 300 365', 'move 300 365'),
    (9205, 'click 300 365', 'click 300 365'),                  # "Yes", leave the game (the game is saved)
    (9700, '', 'cheat nomissdamage'),                          # the main menu is made again and clears the cheats
    (9705, '', 'cheat noawaydamage'),
    (10000, 'move 140 320', 'move 140 320'),
    (10005, 'click 140 320', 'click 140 320'),                 # Classic Bash again
    (10500, 'move 285 390', 'move 285 390'),
    (10505, 'click 285 390', 'click 285 390'),                 # "Continue" of the box that offers the saved game
]

BOT_FROM = 3000
BOT_STEP = 10
BOT_SEED = 69
