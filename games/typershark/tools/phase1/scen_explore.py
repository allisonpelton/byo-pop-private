import sys
sys.path.insert(0, 'games/typershark/tools/phase1')
from tsp import *
from scen_menu import menu_steps, BASE
s = menu_steps()
s.append((BASE + 250, 'click', 320, 381))     # the HINT dialog: "Click here or press space"
for u in range(BASE + 100, BASE + 1400):
    s.append((u, 'peek', [board(0x64), board(0xd78), board(0xd7c), board(0xd88), board(0xd8c), board(0xd98), board(0xd9c), board(0xdc8), board(0xdcc), app(0x624), board(0x1c4)]))
open(sys.argv[1], 'w').write(orig_script(s))
open(sys.argv[2], 'w').write(port_script(s, [f"{BASE + 1500} quit"]))
