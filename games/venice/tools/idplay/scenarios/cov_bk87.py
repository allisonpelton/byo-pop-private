# coverage run: Trick Shot from level 87 with the basket patch of run.sh (cov_trick.py): the first jewel that falls lands in the basket
import os
LEVEL = 87
FRAMES = 3000
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'cov_trick.py')).read())
