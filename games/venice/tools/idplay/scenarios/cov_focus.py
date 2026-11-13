# a coverage run (docs/COVERAGE.md): the focus tour of phase two (shots_focus.py) without its shots, which only the sandbox takes: the
# window loses the focus and gets it back on the main menu, at a level's explanation and in play.
import os
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'shots_focus.py')).read())
FOCUS = [{'frame': block['frame'], 'steps': [], 'wait': block['wait']} for block in FOCUS]
SHOTS = []
