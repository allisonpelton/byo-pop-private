# coverage run: Trick Shot from level 41 (cov_trick.py)
import os
LEVEL = 41
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'cov_trick.py')).read())
