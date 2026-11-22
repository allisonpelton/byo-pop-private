"""The trophy room of a player who has won some (mkprofile.py `rich`: eleven of the twenty-one) and the close-up page of each of them,
for the screens pass (SCREENS.md). The shelf has seven places to a row along a sine wave (TrophyRoomWidget::Resize): a place's
centre is at x = 100 + 100 * column and y = 242 + wave + 75 * row."""

SEED = 1
END = 30000
PROFILE = 'rich'

EARNED_PLACES = [0, 2, 3, 5, 7, 8, 10, 11, 13, 18, 19]          # the places of the earned trophies in the shelf
WAVE = [0, 20, 34, 40, 34, 20, 0]


def click(count, x, y):
    return [(count, 'move %d %d' % (x, y), 'move %d %d' % (x, y)),
            (count + 5, 'click %d %d' % (x, y), 'click %d %d' % (x, y))]


INPUTS = click(1000, 397, 532)                                  # the loading screen's "Click to Play"
INPUTS += click(2500, 400, 360)                                 # OK on the Tough Cookie notice
INPUTS += click(3500, 740, 540)                                 # Trophy Room
NAMES = {4500: 'trophy_room'}
at = 5000
for place in EARNED_PLACES:
    x, y = 100 + 100 * (place % 7), 242 + WAVE[place % 7] + 75 * (place // 7)
    INPUTS += click(at, x, y)                                   # the trophy: its page
    NAMES[at + 1000] = 'trophy_page_place%02d' % place
    INPUTS += click(at + 1500, 400, 550)                        # "Click to Continue"
    at += 2000
INPUTS += click(at, 400, 570)                                   # Back to Main Menu
NAMES[at + 1000] = 'menu_after_trophies'
SHOTS = sorted(NAMES)
