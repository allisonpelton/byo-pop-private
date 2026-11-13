# Level 1's boss: a new player's campaign, with every level 1200 updates long (SHORT_LEVELS: a copy of data/levels.xml
# that both games read), so the boss comes 600 updates in; then the bot until its last tank is lost or the frames run
# out (a boss it beats brings the next level, as short).
SHORT_LEVELS = 1200
SEED = 101
END = 20000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (100, 'char B'), (103, 'char o'), (106, 'char b'), (120, 'key RETURN'),   # the new player's name
    (200, 'move 100 250'), (200, 'click 100 250'),              # the main menu's campaign
]
BOT_FROM = 260
