# phase two: the menu tour of `shots_menus` with the original in its 3D mode (Direct3D 7 over dgVoodoo2, inside Windows Sandbox
# with the host's GPU) and the port in its default 3D mode
import os
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'shots_menus.py')).read())
ACCELERATED = True
