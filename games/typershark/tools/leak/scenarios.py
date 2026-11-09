"""scenarios.py: writes the leak scenarios (build/ts-leak/scripts/*.txt) for the counting build (docs/LEAKS.md). Each flow is repeated
N times, then `leak <flow><k> sites`; the scripts run with run.py / runall.sh. `route` is made from a recorded whole-adventure input
(build/ts-p1/rt1_s1.port.txt, lines `<update> <command>`) with markers added; it is skipped when that file is not there."""
import os
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
OUT = os.path.join(ROOT, 'build', 'ts-leak', 'scripts')
os.makedirs(OUT, exist_ok=True)

class S:
    def __init__(self): self.u = 4000; self.l = []
    def at(self, cmd, gap=20):
        self.l.append('%d %s' % (self.u, cmd)); self.u += gap
    def leak(self, tag, gap=30): self.at('leak %s sites' % tag, gap)
    def done(self, name, quit_extra=60):
        self.u += quit_extra
        self.l.append('%d quit' % self.u)
        open(os.path.join(OUT, name + '.txt'), 'w', newline='\n').write('\n'.join(self.l) + '\n')

# ui: menu, high scores, stats, options, credits
s = S()
s.at('goto menu', 200); s.leak('u0')
for k in range(1, 5):
    s.at('goto highscores', 60); s.at('goto menu', 60)
    s.at('goto stats', 60); s.at('goto menu', 60)
    s.at('goto options', 60); s.at('goto credits', 60); s.at('goto menu', 60)
    s.leak('u%d' % k)
s.done('ui')

# tutor: typing tutor start/leave, with the lessons paged
s = S()
s.at('goto menu', 200); s.leak('t0')
for k in range(1, 5):
    s.at('goto tutor', 80); s.at('level 3', 40); s.at('level 8', 40); s.at('level 0', 40); s.at('goto menu', 80)
    s.leak('t%d' % k)
s.done('tutor')

# adv: adventure level 1 (site 0), lose, back to the menu
s = S()
s.at('goto menu', 200); s.leak('a0')
for k in range(1, 5):
    s.at('goto adventure 0', 300)
    s.at('lose', 300)
    s.at('goto menu', 100)
    s.leak('a%d' % k)
s.done('adv')

# map: adventure, the map screen entered and left, a few levels
s = S()
s.at('goto menu', 200); s.leak('m0')
for k in range(1, 4):
    s.at('goto adventure 0', 300)
    s.at('level 1', 300)
    s.at('win', 600)
    s.at('goto map', 300)
    s.at('level 2', 300)
    s.at('lose', 300)
    s.at('goto menu', 100)
    s.leak('m%d' % k)
s.done('map')

# abyss
s = S()
s.at('goto menu', 200); s.leak('b0')
for k in range(1, 5):
    s.at('goto abyss', 300)
    s.at('level 1200', 300)
    s.at('lose', 300)
    s.at('goto menu', 100)
    s.leak('b%d' % k)
s.done('abyss')

# idle: nothing after the menu, to see whether the count grows by itself
s = S()
s.at('goto menu', 200); s.leak('i0')
for k in range(1, 5):
    s.at('state', 2000); s.leak('i%d' % k)
s.done('idle')

# user: the select-user dialog opened and cancelled, a user switched to and back
s = S()
s.at('goto menu', 200); s.at('press 0x69', 40); s.at('dlist 0x44e 0', 40); s.at('dname Bob', 40); s.at('dyes 0x44d', 60); s.at('press 0xdac', 60)
s.at('goto menu', 100); s.leak('c0')
for k in range(1, 6):
    s.at('press 0x69', 40); s.at('press 0x1006', 40)                        # opened and cancelled
    s.at('press 0x69', 40); s.at('dlist 0x44e 1', 30); s.at('dyes 0x44e', 60)  # first name chosen
    s.at('press 0x69', 40); s.at('dlist 0x44e 2', 30); s.at('dyes 0x44e', 60)  # second name chosen
    s.leak('c%d' % k)
s.done('user')

# usernew: new users made (each adds one profile, which is not a leak)
s = S()
s.at('goto menu', 200); s.leak('n0')
for k in range(1, 5):
    s.at('press 0x69', 40); s.at('dlist 0x44e 0', 40); s.at('dname Bob%d' % k, 40); s.at('dyes 0x44d', 60); s.at('press 0xdac', 60)
    s.leak('n%d' % k)
s.done('usernew')

# userdel (after user and usernew: Tester, Bob, Bob1-4 are there): four more make ten, then delete one and make one again
s = S()
s.at('goto menu', 200)
for k in range(1, 5):
    s.at('press 0x69', 40); s.at('dlist 0x44e 0', 40); s.at('dname Dan%d' % k, 40); s.at('dyes 0x44d', 60); s.at('press 0xdac', 60)
s.leak('d0')
for k in range(1, 5):
    s.at('press 0x69', 40); s.at('dlist 0x44e 10', 40); s.at('dlist 0x44f 5', 30); s.at('dyes 0x44f', 40); s.at('dyes 500', 60)
    s.at('dlist 0x44e 0', 40); s.at('dname Cat%d' % k, 40); s.at('dyes 0x44d', 60); s.at('press 0xdac', 60)
    s.leak('d%d' % k)
s.done('userdel')

def combine(name, parts):
    out = []
    base = 4000
    for part in parts:
        lines = [l for l in open(os.path.join(OUT, part + '.txt')).read().splitlines() if l.strip() and not l.endswith(' quit')]
        first = int(lines[0].split()[0])
        for l in lines:
            u, rest = l.split(' ', 1)
            out.append('%d %s' % (int(u) - first + base, rest))
        base = int(out[-1].split()[0]) + 400
    out.append('%d quit' % (base + 60))
    open(os.path.join(OUT, name + '.txt'), 'w', newline=chr(10)).write(chr(10).join(out) + chr(10))


combine('menus', ['ui', 'tutor', 'user', 'usernew', 'userdel', 'idle'])

# tutor-only variants: enter and leave the tutor (tutA), and the lessons paged (tutor above)
s = S()
s.at('goto menu', 200); s.leak('k0', 60)
for k in range(1, 7):
    s.at('goto tutor', 80); s.at('goto menu', 80)
    s.leak('k%d' % k, 60)
s.done('tutA', 30)

# usermk: three new users (Za, Zb, Zc). After it, delete users/user1.dat .. user3.dat of its user directory and run usersel on a copy of
# that directory (RUN_USERDIR): the select dialog's OK for a user whose data file is missing makes a new profile and replaces the old one
s = S()
s.at('goto menu', 200)
for n in ('Za', 'Zb', 'Zc'):
    s.at('press 0x69', 40); s.at('dlist 0x44e 0', 40); s.at('dname ' + n, 40); s.at('dyes 0x44d', 60); s.at('press 0xdac', 60)
s.at('goto menu', 100)
s.done('usermk', 0)
s = S()
s.at('goto menu', 200)
for n in ('Za', 'Zb', 'Zc'):
    s.at('press 0x69', 40); s.at('dlist 0x44e 0', 40); s.at('dname ' + n, 40); s.at('dyes 0x44d', 60); s.at('press 0xdac', 60)
s.at('goto menu', 100); s.leak('s0')
for k in range(1, 6):
    for line in (0, 1, 2):
        s.at('press 0x69', 40); s.at('dlist 0x44e %d' % line, 30); s.at('dyes 0x44e', 60); s.at('dyes 500', 40)
    s.at('goto menu', 80); s.leak('s%d' % k)
s.done('usersel', 0)

# route: the recorded adventure with markers every 20000 updates
rec = os.path.join(ROOT, 'build', 'ts-p1', 'rt1_s1.port.txt')
if os.path.exists(rec):
    lines = [l.rstrip() for l in open(rec) if l.strip() and not l.rstrip().endswith(' quit')]
    out = ['39990 leak r0 sites']
    marks = [60000, 80000, 100000, 120000, 140000, 160000, 180000]
    for l in lines:
        u = int(l.split()[0])
        while marks and u >= marks[0]:
            out.append('%d leak r%d' % (marks[0], marks[0] // 1000))
            marks.pop(0)
        out.append(l)
    end = int(lines[-1].split()[0]) // 100 * 100 + 200
    out += ['%d leak rlast sites' % end, '%d goto menu' % (end + 100), '%d leak rend sites' % (end + 400), '%d quit' % (end + 500)]
    open(os.path.join(OUT, 'route.txt'), 'w', newline=chr(10)).write(chr(10).join(out) + chr(10))
