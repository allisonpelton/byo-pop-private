# The first run's campaign again with another seed: other boards, waves and drops from the first level on.
SEED = 2
END = 40000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (100, 'char A'), (103, 'char c'), (106, 'char e'), (120, 'key RETURN'),   # the new player's name
    (200, 'move 100 250'), (200, 'click 100 250'),              # the main menu's campaign
]
BOT_FROM = 260
