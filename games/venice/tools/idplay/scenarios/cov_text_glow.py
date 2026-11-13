# a coverage run (docs/COVERAGE.md) for the original in its 3D mode (Direct3D 7 over dgVoodoo2 on rundesk's desktop, docs/ORIGRUN.md,
# "Hardware acceleration"): the pointer comes onto the texts of the main menu and goes off them, which in 3D makes them glow (the glow
# reads the text's bounds: TextObject::GetBounds 0x425220, its only caller)
ACCELERATED = True
PROFILE_COMMANDS = ['unlock all', 'goto menu']
INPUTS = []
frame = 200
for x, y in ((400, 232), (400, 302), (400, 330), (400, 232), (100, 100), (400, 232), (400, 302), (150, 500)):
    INPUTS.append((frame, 'move %d %d' % (x, y), 'move %d %d' % (x, y)))
    frame += 120
END = frame + 600
