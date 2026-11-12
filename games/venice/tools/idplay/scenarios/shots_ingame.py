# phase two, the screens of a level in play: the in-game menu (its fade and settled), the help, options and exit-confirmation
# screens opened from it, and the game going on after Resume. The first clicks are those of `flood` (a player with every mode
# unlocked: Play Now, Flood and its new game); a click is a move, a press 50 frames later and the release two frames after that.
PROFILE_COMMANDS = ['unlock all', 'goto menu']
INPUTS = []
SHOTS = []


def click(frame, x, y):
    INPUTS.extend([(frame, 'move %d %d' % (x, y), 'move %d %d' % (x, y)), (frame + 50, 'down %d %d' % (x, y), 'down %d %d' % (x, y)),
                   (frame + 52, 'up %d %d' % (x, y), 'up %d %d' % (x, y))])


click(100, 400, 232)
click(700, 401, 270)
click(1300, 470, 330)
SHOTS.append((1800, 'level_explained'))
click(1900, 400, 300)
SHOTS.append((2400, 'level_playing'))
frame = 2500
click(frame, 24, 590)
SHOTS.extend([(frame + 100, 'menu_fade'), (frame + 400, 'menu')])
frame += 500


def visit(frame, name, open_at, close_at, settle=300):
    click(frame, *open_at)
    SHOTS.append((frame + 100, name + '_fade'))
    SHOTS.append((frame + settle, name))
    click(frame + settle + 100, *close_at)
    SHOTS.append((frame + settle + 200, name + '_closing'))
    SHOTS.append((frame + settle + 450, name + '_closed'))
    return frame + settle + 550


frame = visit(frame, 'help', (398, 250), (741, 580), settle=400)
frame = visit(frame, 'options', (398, 290), (399, 442))
frame = visit(frame, 'confirm', (399, 330), (470, 330))
click(frame, 399, 400)
SHOTS.extend([(frame + 100, 'resume_fade'), (frame + 400, 'resume')])
MOUSE_AT = (790, 300)
END = frame + 500
