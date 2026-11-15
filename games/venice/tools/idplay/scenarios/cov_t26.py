# coverage run: Trick Shot from level 26 (cov_trick.py)
import os
LEVEL = 26
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'cov_trick.py')).read())
