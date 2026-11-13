"""Tough Cookie from the loading screen: a new player named Test, Tough Cookie unlocked (the original through a patched copy whose
IsToughUnlocked, 0x41f2d0, answers yes; the port through `unlock all`), the how-to-play page, and the bot's clicks from the first
level's introduction on."""

SEED = 1
END = 28000

PATCHES = [(0x41f2d0, '568bf1', 'b001c3', 'IsToughUnlocked (0x41f2d0) answers yes: mov al, 1; ret')]

PREFIX = [
    (1000, 'move 397 532', 'move 397 532'),
    (1005, 'click 397 532', 'click 397 532'),                  # the loading screen's "Click to Play"
    (1500, 'text T', 'char T'),
    (1510, 'text e', 'char e'),
    (1520, 'text s', 'char s'),
    (1530, 'text t', 'char t'),
    (1600, 'move 310 392', 'move 310 392'),
    (1605, 'click 310 392', 'click 310 392'),                  # OK in the new-name box
    (1700, '', 'unlock all'),                                  # after the main menu is made
    (2000, 'move 196 215', 'move 196 215'),
    (2005, 'click 196 215', 'click 196 215'),                  # Tough Cookie (the button's top middle, clear of Classic Bash's)
    (2500, 'move 300 550', 'move 300 550'),
    (2505, 'click 300 550', 'click 300 550'),                  # "Click here to play" on the how-to-play page
]

BOT_FROM = 3000
BOT_STEP = 10
BOT_SEED = 31
