# mkscript.py <out> <start update> <rng seed> <part>...: writes a port -script for a case. A part is
#   menu:<mode>[:<choice>]   from the main menu: New Game, the mode (stomped, endless, fossil, timetrial), its page's
#                            choice (stomped: the pack list's first pack; fossil: strategic or panic; timetrial: easy,
#                            normal or hard) and Next / Let's Play
#   title                    the title screen's "click here to play"
#   shots:<n>:<gap>          n shots at random points of the board (never the side bar), one every gap updates
#   wait:<n>                 n updates
#   click:<x>:<y>            a move and a click 10 updates later, then 50 updates
#   click@port:<x>:<y>       the same for the port only (the line ends "# port", which toorig.py leaves out); with
#   click@orig:<x>:<y>       @orig, for the original only (a whole-line comment "# orig <line>", which only toorig.py
#                            reads); a pair of them takes the time of one click (where a button sits elsewhere)
#   text:<chars>             typed a character every 5 updates, then 25 updates
#   key:<vk>                 a key, then 30 updates
#   focus:<lose|regain>      the window loses or regains the focus, then 50 updates
#   save:<name>              the side bar's save button, the first slot, Save, the name, Save and the saved dialog's OK
#                            (SLOT0.SAV; the game goes back to the main menu)
#   cont                     from the main menu: Continue Game, the first saved game and Let's Play
#   shot:<name>              a screenshot (the port only; toorig.py leaves it out)
#   quit
# The input never touches a link button.
import random
import sys

out, u, seed = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
r = random.Random(seed)
L = []
shotdir = out.rsplit('/', 1)[0] if '/' in out else '.'


def add(cmd, gap):
    global u
    L.append("%d %s" % (u, cmd))
    u += gap


def click(x, y, gap=60, side=None):
    if side == 'orig':
        L.append("# orig %d move %d %d" % (u, x, y))
        L.append("# orig %d click %d %d" % (u + 10, x, y))
        return
    tail = " # port" if side == 'port' else ""
    add("move %d %d%s" % (x, y, tail), 10)
    add("click %d %d%s" % (x, y, tail), gap)


MODES = {'endless': 160, 'stomped': 230, 'fossil': 295, 'timetrial': 360}
CHOICES = {'pack': 162, 'strategic': 200, 'panic': 280, 'easy': 195, 'normal': 265, 'hard': 335}
for part in sys.argv[4:]:
    f = part.split(':')
    if f[0] == 'title':
        click(320, 450, 300)
    elif f[0] == 'menu':
        click(486, 213)
        click(465, MODES[f[1]])
        click(488, 432)
        if len(f) > 2:
            click(465, CHOICES[f[2]])
            click(488, 432, 500)
    elif f[0] == 'shots':
        for i in range(int(f[1])):
            x = r.randint(180, 620)
            y = r.randint(40, 330)
            add("move %d %d" % (x, y), 5)
            add("click %d %d" % (x, y), int(f[2]) - 5)
    elif f[0] == 'wait':
        u += int(f[1])
    elif f[0] == 'click':
        click(int(f[1]), int(f[2]))
    elif f[0] in ('click@port', 'click@orig'):
        click(int(f[1]), int(f[2]), side=f[0][6:])
    elif f[0] == 'text':
        for c in f[1]:
            add("char %s" % c, 5)
        u += 25
    elif f[0] == 'save':
        click(45, 297)
        click(284, 207)
        click(354, 399)
        for c in f[1]:
            add("char %s" % c, 5)
        u += 25
        click(356, 360)
        click(403, 290, 200)
    elif f[0] == 'cont':
        click(486, 283)
        click(488, 432)
        click(379, 175)
        click(488, 432, 300)
    elif f[0] == 'focus':
        add("focus %s" % f[1], 50)
    elif f[0] == 'key':
        add("key %s" % f[1], 30)
    elif f[0] == 'shot':
        add("screenshot %s/%s.png" % (shotdir, f[1]), 1)
    elif f[0] == 'quit':
        add("quit", 1)
open(out, 'w').write("\n".join(L) + "\n")
