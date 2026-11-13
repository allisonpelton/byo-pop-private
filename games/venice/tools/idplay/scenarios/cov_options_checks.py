# a coverage run (docs/COVERAGE.md) for the night (22:00 to 08:00): the options screen's two check boxes, which change the display of the
# original on the host. Options, hardware acceleration on and off again (SetAcceleratedDisplay 0x4418a0), full screen on and off again
# (SetWindowedDisplay 0x441930), each toggle through Checkbox::Toggle (0x4379f0), then Done. The boxes are at x 281, y 353 and 393 (their
# text and `right_area` take the click too); a toggle that remakes the display gets 400 frames before the next input.
PROFILE_COMMANDS = ['unlock all', 'goto menu']
INPUTS = []


def click(frame, x, y):
    INPUTS.extend([(frame, 'move %d %d' % (x, y), 'move %d %d' % (x, y)), (frame + 50, 'down %d %d' % (x, y), 'down %d %d' % (x, y)),
                   (frame + 52, 'up %d %d' % (x, y), 'up %d %d' % (x, y))])


click(100, 400, 302)   # Options
click(700, 281, 353)   # hardware acceleration on
for frame, x, y in ((950, 399, 442), (1050, 400, 300), (1150, 399, 442), (1250, 330, 160)):
    INPUTS.append((frame, 'move %d %d' % (x, y), 'move %d %d' % (x, y)))  # with 3D on the pointer comes onto the text DONE, goes off and on again:
    # its glow reads the text's bounds (TextObject::GetBounds 0x425220)
click(1300, 281, 353)  # and off
click(1900, 281, 393)  # full screen on
click(2500, 281, 393)  # and off
click(3100, 399, 442)  # Done
END = 3700
