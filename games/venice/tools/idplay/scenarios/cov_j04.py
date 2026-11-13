# coverage run: Journey from level 4 (cov_journey.py)
import os
LEVEL = 4
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'cov_journey.py')).read())
