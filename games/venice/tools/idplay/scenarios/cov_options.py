# a coverage run (docs/COVERAGE.md): the options screen's two sliders, dragged and clicked on their tracks (the music's at y 240, the
# sound's at y 305, x 275 to 525; the knobs start in the middle), then Done. The two check boxes (hardware acceleration, full
# screen) change the display, so a run on the host leaves them: they are for a run inside the sandbox, at night (docs/COVERAGE.md).
PROFILE_COMMANDS = ['unlock all', 'goto menu']
INPUTS = []


def click(frame, x, y):
    INPUTS.extend([(frame, 'move %d %d' % (x, y), 'move %d %d' % (x, y)), (frame + 50, 'down %d %d' % (x, y), 'down %d %d' % (x, y)),
                   (frame + 52, 'up %d %d' % (x, y), 'up %d %d' % (x, y))])


def drag(frame, track_y, start_x, stops):
    """a press on the knob, moves along the track (a move a few updates apart) and the release"""
    INPUTS.append((frame, 'move %d %d' % (start_x, track_y), 'move %d %d' % (start_x, track_y)))
    INPUTS.append((frame + 30, 'down %d %d' % (start_x, track_y), 'down %d %d' % (start_x, track_y)))
    at = frame + 40
    for x in stops:
        INPUTS.append((at, 'move %d %d' % (x, track_y), 'move %d %d' % (x, track_y)))
        at += 10
    INPUTS.append((at + 20, 'up %d %d' % (stops[-1], track_y), 'up %d %d' % (stops[-1], track_y)))
    return at + 20


click(100, 400, 302)  # Options
frame = drag(700, 240, 400, [430, 470, 520, 540, 350, 290, 270])  # the music slider, to both ends
click(frame + 100, 300, 240)  # a click on the track
click(frame + 300, 500, 240)
frame = drag(frame + 600, 305, 412, [450, 500, 530, 300, 260])  # the sound slider
click(frame + 100, 280, 305)
click(frame + 300, 520, 305)
click(frame + 700, 399, 442)  # Done
END = frame + 1500
