"""Marathon from the loading screen: a new player named Test, Marathon unlocked (the original through a patched copy whose
HasFewRestartPoints, 0x41f290, answers no; the port through `unlock marathon`), and the bot's clicks from the first level's
introduction on."""

SEED = 1
END = 28000

PATCHES = [(0x41f290, '568bf1', '32c0c3', 'HasFewRestartPoints (0x41f290) answers no: xor al, al; ret')]

PREFIX = [
    (1000, 'move 397 532', 'move 397 532'),
    (1005, 'click 397 532', 'click 397 532'),                  # the loading screen's "Click to Play"
    (1500, 'text T', 'char T'),
    (1510, 'text e', 'char e'),
    (1520, 'text s', 'char s'),
    (1530, 'text t', 'char t'),
    (1600, 'move 310 392', 'move 310 392'),
    (1605, 'click 310 392', 'click 310 392'),                  # OK in the new-name box
    (1700, '', 'unlock marathon'),
    (2000, 'move 668 325', 'move 668 325'),
    (2005, 'click 668 325', 'click 668 325'),                  # Marathon Bash
    (2500, 'move 300 550', 'move 300 550'),
    (2505, 'click 300 550', 'click 300 550'),                  # "Click here to play" on the how-to-play page
]

BOT_FROM = 3000
BOT_STEP = 10
BOT_SEED = 32
