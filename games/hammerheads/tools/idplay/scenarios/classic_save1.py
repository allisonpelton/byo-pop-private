"""Classic Bash from the loading screen as `classic1`, left in the middle of the first level (the in-game menu, "Main Menu", "Yes" of
"Leave Game?": the game is saved) and resumed from the main menu ("Classic Bash", "Continue" of the box that offers the saved
game, "Go!" of the "Ready?" box, which the bot presses), and played on by the bot."""

SEED = 1
END = 16000

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
    (8000, 'move 750 30', 'move 750 30'),
    (8005, 'click 750 30', 'click 750 30'),                    # the in-game menu's button
    (8100, 'move 400 235', 'move 400 235'),
    (8105, 'click 400 235', 'click 400 235'),                  # "Main Menu"
    (8200, 'move 300 365', 'move 300 365'),
    (8205, 'click 300 365', 'click 300 365'),                  # "Yes", leave the game (the game is saved)
    (9000, 'move 140 320', 'move 140 320'),
    (9005, 'click 140 320', 'click 140 320'),                  # Classic Bash again
    (9500, 'move 285 390', 'move 285 390'),
    (9505, 'click 285 390', 'click 285 390'),                  # "Continue" of the box that offers the saved game
]

BOT_FROM = 3500
BOT_STEP = 10
BOT_SEED = 68
