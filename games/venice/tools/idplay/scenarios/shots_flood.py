# phase two: the `flood` scenario of phase one with a shot every 1000 frames (the bot's play is given to both games as input)
import os
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'flood.py')).read())
SHOTS = [(frame, 'flood_%05d' % frame) for frame in range(500, END, 1000)]
MOUSE_AT = (790, 300)
