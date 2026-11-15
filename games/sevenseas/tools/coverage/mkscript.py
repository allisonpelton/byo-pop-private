"""mkscript.py <difficulty 1..3 | tutorial | menus> <seed> <clicks> > script: an origrun script for a coverage run of
Seven Seas (docs/COVERAGE.md): the title's Play link, the menu's wheel and Play, then clicks over the board's 11 x 11 cells in a
seeded random order (a click sails the ship or fires its broadside; one that does nothing is ignored), with now and then a click
where the demo's, the map's and the high score's button sit. The game's coordinates are 640 x 480; the original's loading takes
varying time, so the first click comes late (GAME_NOTES section 8)."""
import random, sys
mode, seed, n = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
r = random.Random(seed)
out = []; t = [1]
def at(dt, s):
    t[0] += dt; out.append("at %d %s" % (t[0], s))
def click(x, y, dt=100):
    at(dt // 2, "move %d %d" % (x, y)); at(dt // 2, "click %d %d 1" % (x, y))
if mode.startswith("save"):
    # a run that starts from a saved game (run.sh's SAVE): the title's Play link by a spot that is empty in the menu
    # (a click that comes before the original has loaded is ignored), the menu's Play, the Continue dialog's Yes (or No:
    # saveno), the Treasure Island nag's button, then the bonus level's digs or the board's cells
    for i in range(6): click(360, 385, 400 if i else 1000)
    click(507, 397, 600)
    click(330 if mode != "saveno" else 455, 200, 500)
    click(405, 391, 400)
    for i in range(n):
        if i % 12 == 11: click(390, 440, 100)
        elif i % 12 == 5: click(405, 391, 100)
        elif i % 7 == 6: click(330, 445, 100)
        elif i % 15 == 3: click(87, 169, 100)                   # Undo
        elif i % 2: click(173 + 42 * r.randrange(1, 10) + 26, 42 * r.choice([1, 2, 3, 4, 5, 6, 7, 9]) + 39, 100)
        else: click(173 + 42 * r.randrange(11) + 21, 42 * r.choice([0, 1, 2, 3, 4, 5, 6, 7, 9, 10]) + 33, 100)
    print(chr(10).join(out)); print("wait 3000"); print("quit"); sys.exit(0)
click(320, 385, 3000)                      # title's Play link
if mode in ("1", "2", "3"):
    click({"1": 250, "2": 370, "3": 500}[mode], 300, 100)   # the wheel of the difficulty
    click(507, 397, 600)                    # Play
    click(390, 440, 300)
    for i in range(n):
        if i % 10 == 9: click(390, 440, 100)
        elif i % 10 == 4: click(330, 445, 100)
        elif i % 15 == 3: click(87, 169, 100)                   # Undo
        elif i % 25 == 24: click(320, 411, 100)
        else: click(173 + 42 * r.randrange(11) + 21, 42 * r.choice([0, 1, 2, 3, 4, 5, 6, 7, 9, 10]) + 33, 100)
elif mode == "tutorial":
    click(256, 398, 600)                    # the Tutorial button
    for i in range(n):
        click(r.choice([404, 320, 390]), r.choice([243, 440, 300]), 120)
elif mode == "ui":
    # the panel's buttons and every dialog: the positions are the port's screenshots' (640 x 480)
    click(250, 300, 100); click(507, 397, 1300); click(390, 440, 700)
    def options(*steps):
        click(87, 263, 150)
        for st in steps: click(st[0], st[1], 150)
    options((450, 188), (450, 220), (401, 254), (401, 254), (296, 281), (487, 440))       # sliders, cursors, Help, back
    options((399, 281), (395, 332), (407, 354))                                         # High Scores, Done, Done
    click(87, 441, 150); click(455, 200, 150)                                           # New Game: No
    options((296, 281), (295, 440))                                                     # Help: Play the Tutorial
    for i in range(30): click(r.choice([404, 320, 390]), r.choice([243, 440, 300]), 150)
    click(87, 441, 150); click(330, 200, 300)                                           # New Game: Yes
    click(404, 452, 300); click(455, 200, 150)                                          # Quit: No
    click(404, 452, 300); click(330, 200, 300)                                          # Quit: Yes
print(chr(10).join(out))
print("wait 3000")
print("quit")
