# coverage run: Journey from level 46 for 14000 frames (cov_journey.py)
import os
LEVEL = 46
FRAMES = 14000
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'cov_journey.py')).read())
