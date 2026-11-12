# phase two: a player with every mode unlocked on the main menu, and every screen the menu leads to (options, help, credits, the
# high scores, the mode select, the exit confirmation, the player's menu), each shot while its fade runs and once it has settled.
# A click is a move, a press 50 frames later and the release two frames after that (the game reads the button once an update);
# the places are those of the `buttons` command on each screen.
PROFILE_COMMANDS = ['unlock all', 'goto menu']
INPUTS = []
SHOTS = [(0, 'menu_start')]


def click(frame, x, y):
    INPUTS.extend([(frame, 'move %d %d' % (x, y), 'move %d %d' % (x, y)), (frame + 50, 'down %d %d' % (x, y), 'down %d %d' % (x, y)),
                   (frame + 52, 'up %d %d' % (x, y), 'up %d %d' % (x, y))])


def visit(frame, name, open_at, close_at, settle=300, extra=()):
    """open a screen with a click, shot it while it fades in (`name`_fade) and settled (`name`), at the extra frames after the
    opening click too (`name`_<frames>), and close it with a click; returns the frame after its closing shot"""
    click(frame, *open_at)
    SHOTS.append((frame + 100, name + '_fade'))
    SHOTS.append((frame + settle, name))
    for after in extra:
        SHOTS.append((frame + after, '%s_%d' % (name, after)))
    close_frame = frame + max(settle, *extra, 0) + 100
    click(close_frame, *close_at)
    SHOTS.append((close_frame + 100, name + '_closing'))
    SHOTS.append((close_frame + 350, name + '_closed'))
    return close_frame + 450


frame = 100
frame = visit(frame, 'options', (400, 302), (399, 442))
frame = visit(frame, 'help', (400, 337), (741, 580), settle=400)
frame = visit(frame, 'credits', (400, 372), (741, 580), extra=(900, 1500))
frame = visit(frame, 'scores', (400, 267), (96, 587))
frame = visit(frame, 'modes', (400, 232), (400, 500))
frame = visit(frame, 'exit', (400, 407), (470, 330))
frame = visit(frame, 'player', (99, 585), (530, 442))
MOUSE_AT = (790, 300)
END = frame + 100
