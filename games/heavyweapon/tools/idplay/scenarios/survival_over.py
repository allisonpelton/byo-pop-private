# The coverage check (GAME_NOTES.md, "Coverage check"): survival.py's game (an unlocked player's survival, the bot until
# its last tank is lost at frame 2656), and then the game-over box's QUIT (its Resize), which the board's ButtonDepress
# answers with the high scores.
PROFILE = 'unlocked'
END = 30000
OPENING = [
    (60, 'move 320 419'), (60, 'click 320 419'),                # the title's "click here to play"
    (150, 'move 230 380'), (150, 'click 230 380'),              # the main menu's survival
    (2900, 'move 395 320'), (2900, 'click 395 320'),            # the game-over box's QUIT
]
BOT_FROM = 200
