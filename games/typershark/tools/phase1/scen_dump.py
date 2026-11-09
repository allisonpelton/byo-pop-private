import sys
sys.path.insert(0, 'games/typershark/tools/phase1')
from tsp import *
from scen_menu import menu_steps, BASE
s = menu_steps()
s.append((BASE + 250, 'click', 320, 381))
U = BASE + 290
for i in (0, 1):
    # element i of the sharks list: the slot (first + 4*i) holds the creature's pointer
    s.append((U, 'peek', [f"[{board(0xd78)}]+0x{4*i:x}"]))
    ex = [ind(f"[{board(0xd78)}]+0x{4*i:x}", 4 * k) for k in range(0, 132)]
    s.append((U, 'peek', ex))
open(sys.argv[1], 'w').write(orig_script(s))
