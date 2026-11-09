"""The coverage runs' input, written once for the port (a -realinput file, to check what each click does by `state`) and
for the original (an origrun script, steps of the game's counter gFrame at 0x460a1c: 1 step = 1 ms, 10 to a port update).

    python -I games/waterbugs/tools/coverage/scripts.py <name> <out dir>

writes <out dir>/port-<name>.txt and orig-<name>.txt. The runs are listed in RUNS below; each is a list of (step, what, ...):
('click', x, y) a left click held 5 steps; ('rclick', x, y); ('key', vk, 'SDL name') a key held 5 steps; ('move', x, y); ('wait'); and
('text', 'abc'); ('focus', 'lose' or 'regain'); ('state',), which only the port's file gets.
"""
import os
import sys

LOGO = 2200      # the main menu is up after the logo's 2.15 s of steps

# the screens' buttons, from the port's `state` (docs/GAME_NOTES.md, "The standard test commands")
START, HELP, OPTIONS, SCORES, USER, QUIT = (400, 303), (400, 350), (400, 397), (400, 444), (400, 491), (400, 537)
HELLO, NOTYOU = (234, 363), (234, 373)
O_VIDEO, O_LCD, O_SOUND, O_MUSIC, O_SHOOTER, O_DIFF, O_CREDITS, O_BACK = (400, 77), (400, 137), (400, 197), (400, 257), (400, 317), (400, 377), (400, 437), (400, 537)
HELP_NEXT = (400, 576)
CREDITS_BACK, SCORES_BACK = (400, 527), (400, 552)
STORY_GO, RESULT_GO = (400, 467), (400, 425)
ESC, RETURN = (0x1b, 'Escape'), (0x0d, 'Return')


def seq(start, gap, events):
    """events one after another from step `start`, `gap` steps apart"""
    out, t = [], start
    for e in events:
        out.append((t,) + tuple(e))
        t += gap
    return out


def clicks(*points):
    return [('click',) + p for p in points]


def probe(start, gap, events):
    """seq with a `state` (port only) 150 steps after each event"""
    out = []
    for e in seq(start, gap, events):
        out.append(e)
        out.append((e[0] + 150, 'state'))
    return out


def tour():
    """the menus: options (every button), credits, help, high scores, change user, quit from the main menu"""
    ev = []
    ev += probe(LOGO + 300, 300, clicks(OPTIONS, O_LCD, O_LCD, O_SOUND, O_SOUND, O_MUSIC, O_MUSIC, O_SHOOTER, O_SHOOTER,
                                      O_DIFF, O_DIFF, O_DIFF, O_CREDITS, CREDITS_BACK, HELP, HELP_NEXT, HELP_NEXT,
                                      SCORES, SCORES_BACK, USER, (484, 452)))
    ev.append((LOGO + 300 + 300 * 20 + 400, 'key') + ESC)
    return ev


def users():
    """the player list: pick a player, rename, delete, done"""
    OKAY, CANCEL = (340, 330), (460, 330)
    ev = probe(LOGO + 300, 300, clicks(USER, (400, 202)) + [('text', 'Bob')] + clicks(OKAY, (316, 452)))
    ev += probe(LOGO + 3100, 300, [('text', 'Ann')] + clicks(CANCEL, (316, 452)) + [('text', 'Ann')] + clicks(OKAY, (400, 452), CANCEL,
                                                                                                         (400, 452), OKAY, (484, 452)))
    return ev


def spots(seed, start, n, gap=700, lead=350, manual=False, area=(24, 24, 776, 496)):
    """shots at random places of the board: the ship is sent there (`move`), and fires `lead` steps later; with `manual` a right
    click first turns the ship; every ninth shot is a click on the story screen's and the results requester's buttons, which
    a shot on a board never mistakes (a click on empty board is a shot)"""
    import random
    r = random.Random(seed)
    ev, t = [], start
    for i in range(n):
        x, y = r.randint(area[0], area[2]), r.randint(area[1], area[3])
        ev.append((t, 'move', x, y))
        if manual and i % 2 == 0:
            ev.append((t + lead - 100, 'rclick', x, y))
        ev.append((t + lead, 'click', x, y))
        if i % 9 == 8:
            ev.append((t + lead + 150, 'click') + STORY_GO)
            ev.append((t + lead + 300, 'click') + RESULT_GO)
        t += gap
    return ev, t


def play(seed=1, n=60, manual=False, extra=()):
    """a game from the main menu's Start: the story screen, then shots at random places until the game or the shots end, then Esc
    on the board (the quit requester) and its answer"""
    ev = seq(LOGO + 300, 500, clicks(START, STORY_GO))
    shots, t = spots(seed, LOGO + 300 + 500 + 2200, n, manual=manual)
    ev += shots
    return ev, t


# the world map's buttons (the port's `state`) and the first level of each world's set
WORLDS = [(0, (76, 263)), (10, (101, 486)), (20, (320, 493)), (50, (521, 524)), (100, (711, 371)), (200, (719, 150)),
          (300, (356, 119)), (400, (311, 295)), (500, (516, 256))]
MAP_START = (400, 571)      # the level selection's start button
OKAY, CANCEL, YES, NO = (340, 330), (460, 330), (340, 330), (460, 330)


def world_of(level):
    return [w for w in WORLDS if w[0] <= level][-1][1]


def options(t, manual=False, difficulty=0, gap=400):
    """the Options screen's two buttons (the shooter mode toggles from auto, the difficulty cycles 0, 1, -1), then back"""
    ev = [(t, 'click') + OPTIONS]
    t += gap
    if manual:
        ev.append((t, 'click') + O_SHOOTER)
        t += gap
    for _ in range({0: 0, 1: 1, -1: 2}[difficulty]):
        ev.append((t, 'click') + O_DIFF)
        t += gap
    ev.append((t, 'click') + O_BACK)
    return ev, t + gap


def finish(t):
    """Esc on a board asks to quit: cancel it, Esc again, okay (the main menu), then Esc there leaves the game"""
    return [(t, 'key') + ESC, (t + 500, 'click') + CANCEL, (t + 1000, 'key') + ESC, (t + 1500, 'click') + OKAY,
            (t + 2500, 'key') + ESC]


def deep(reached, seed=1, n=45, manual=False, difficulty=0, focus=False, gap=500):
    """a game from level reached+1 (the save: `scripts.py save <reached>`): Start, the world's button, the level selection's start,
    then shots at random places; with `focus` the focus is lost and regained in the middle of the shots"""
    ev, t = [], LOGO + 300
    if manual or difficulty:
        o, t = options(t, manual, difficulty, gap)
        ev += o
    # each button is hovered 300 steps before its click: a click that comes with its move in one update is sometimes lost on the
    # world map (deep120, held74)
    for k, p in enumerate((START, world_of(reached + 1), MAP_START)):
        ev += [(t + k * gap - 300, 'move') + p, (t + k * gap, 'click') + p]
    t += 2 * gap + 2500
    shots, end = spots(seed, t, n, manual=manual)
    ev += shots
    if focus:
        mid = t + (end - t) // 2
        ev += [(mid, 'focus', 'lose'), (mid + 1500, 'focus', 'regain')]
    ev += finish(end + 300)
    return ev


def resume(answer_yes, n=12, seed=3):
    """Start with a game in progress asks to resume it: yes plays it on (a few shots), no goes to the world map"""
    ev, t = [(LOGO + 300, 'click') + START], LOGO + 300 + 600
    if answer_yes:
        ev.append((t, 'click') + YES)
        shots, end = spots(seed, t + 1500, n)
        ev += shots + finish(end + 300)
    else:
        ev += [(t, 'click') + NO, (t + 800, 'click') + (750, 575), (t + 1600, 'key') + ESC]
    return ev


def typing():
    """the player list's rename box, typed with key events (the game reads the typed keys from its key queue, not from character
    messages): shifted and plain letters, backspace, the arrows, home, end, delete, a name longer than the box, and Return"""
    OKAY = (340, 330)
    ev = clicks(USER, (400, 202), (316, 452))
    ev = seq(LOGO + 300, 400, ev)
    t = LOGO + 300 + 400 * 3 + 600
    keys = [('keydown', 0x10, 'Left Shift'), ('key', 0x42, 'B'), ('keyup', 0x10, 'Left Shift'), ('key', 0x4f, 'O'), ('key', 0x42, 'B'),
            ('key', 0x08, 'Backspace'), ('key', 0x25, 'Left'), ('key', 0x24, 'Home'), ('key', 0x2e, 'Delete'), ('key', 0x27, 'Right'),
            ('key', 0x23, 'End'), ('key', 0x20, 'Space'), ('key', 0x31, '1'), ('key', 0xbd, '-'), ('key', 0xe2, 'Left Alt')]
    keys += [('key', 0x57, 'W')] * 14
    keys += [('key', 0x08, 'Backspace')] * 3 + [('key', 0x0d, 'Return')]
    for k in keys:
        ev.append((t,) + k)
        t += 250
    ev += seq(t + 300, 400, clicks((400, 452), OKAY, (484, 452)))
    return ev


def leave():
    """quit from the main menu's button: the question answered no, then yes"""
    return seq(LOGO + 300, 600, clicks(QUIT, CANCEL, QUIT, OKAY))


def name_entry(t):
    """the game over screen's click, then the high score name entry: on-screen keys (letters, digits, a space, the rub-out), the
    typed keys, and "click here when done"; then the table's back button. A click that finds the board instead is a shot."""
    ev = [(t, 'click', 400, 300)]
    t += 1800
    for p in [(100, 300), (100, 350), (100, 400), (650, 450), (700, 450), (700, 450), (450, 300), (550, 350)]:
        ev.append((t, 'click') + p)
        t += 300
    ev += [(t, 'key', 0x57, 'W'), (t + 300, 'key', 0x08, 'Backspace')]
    t += 900
    ev += [(t, 'click', 400, 510), (t + 1500, 'click') + SCORES_BACK]
    return ev, t + 2500


def lost(seed, n=70, gap=1200):
    """a saved game with one life left, played on at random shots until it is lost, then the name entry (three tries, as the moment
    of the loss is not fixed)"""
    ev, t = [(LOGO + 300, 'click') + START, (LOGO + 300 + gap, 'click') + YES], LOGO + 300 + gap + 3000
    shots, t = spots(seed, t, n)
    ev += shots
    for _ in range(3):
        e, t = name_entry(t)
        ev += e
    return ev + finish(t)


def held(reached, seed, keys, n=90, gap=1200):
    """a deep run in which the original's own test keys are held: while one of 0 to 9 and A to D is down, a power-up that appears is
    of that kind (Powerup_Init); each key for a third of the shots. A power-up the ship runs into is collected, and a weapon among them
    goes off at the next shot."""
    ev = deep(reached, seed=seed, n=n, gap=gap)
    t0 = LOGO + 300 + 2 * gap + 2500
    span = n * 700 // len(keys)
    for i, (vk, name) in enumerate(keys):
        ev += [(t0 + i * span, 'keydown', vk, name), (t0 + (i + 1) * span - 50, 'keyup', vk, name)]
    return ev


RUNS = {'tour': tour, 'typing': typing, 'leave': leave, 'users': users, 'play1': lambda: play(1)[0] + finish(play(1)[1] + 300),
        'resumeyes': lambda: resume(True), 'resumeno': lambda: resume(False)}
for _r in (9, 14, 30, 60, 120, 230, 330, 430, 530):
    RUNS['deep%d' % _r] = (lambda r: lambda: deep(r, seed=r))(_r)
RUNS['deepman'] = lambda: deep(60, seed=7, manual=True)
RUNS['deepeasy'] = lambda: deep(14, seed=8, difficulty=-1)
RUNS['deephard'] = lambda: deep(30, seed=9, difficulty=1)
RUNS['deepfocus'] = lambda: deep(9, seed=5, focus=True)
# the two that never reached a board when the machine was busy (the world map's change takes longer than the gaps): slower clicks
RUNS['deep120b'] = lambda: deep(120, seed=120, gap=1200)
RUNS['deepmanb'] = lambda: deep(60, seed=7, manual=True, gap=1200)
# more boards of the later worlds, longer, with other shots: the power-ups and enemy kinds the first runs did not meet
LOST = (149, 399, 519)
for _r in LOST:
    RUNS['over%d' % _r] = (lambda r: lambda: lost(r))(_r)
HELD = {74: (('5', 'D', 'A')), 149: ('B', '1', '9'), 189: ('8', '6', '7'), 269: ('C', '4', '7')}
for _r, _k in HELD.items():
    RUNS['held%d' % _r] = (lambda r, k: lambda: held(r, r + 21, [(ord(c), c) for c in k]))(_r, _k)
MORE = (39, 74, 149, 189, 269, 379, 469, 559)
for _r in MORE:
    RUNS['more%d' % _r] = (lambda r: lambda: deep(r, seed=r + 11, n=70, gap=1200))(_r)


def write(name, out):
    events = RUNS[name]()
    events.sort(key=lambda e: e[0])
    port, orig = [], []
    for e in events:
        step, what, args = e[0], e[1], e[2:]
        u = step // 10
        if what == 'click':
            x, y = args
            port.append('%d move game %d %d' % (u, x, y)); port.append('%d down game %d %d left' % (u + 1, x, y))
            port.append('%d up game %d %d left' % (u + 4, x, y)); orig.append('at %d click %d %d 5' % (step, x, y))
        elif what == 'rclick':
            x, y = args
            port.append('%d move game %d %d' % (u, x, y)); port.append('%d down game %d %d right' % (u + 1, x, y))
            port.append('%d up game %d %d right' % (u + 4, x, y)); orig.append('at %d rclick %d %d 5' % (step, x, y))
        elif what == 'move':
            x, y = args
            port.append('%d move game %d %d' % (u, x, y)); orig.append('at %d move %d %d' % (step, x, y))
        elif what == 'key':
            vk, key = args
            port.append('%d keydown %s' % (u, key)); port.append('%d keyup %s' % (u + 2, key)); orig.append('at %d key %d 5' % (step, vk))
        elif what in ('keydown', 'keyup'):
            vk, key = args
            port.append('%d %s %s' % (u, what, key)); orig.append('at %d %s %d' % (step, what, vk))
        elif what == 'focus':
            port.append('%d focus %s' % (u, 'lost' if args[0] == 'lose' else 'gained')); orig.append('at %d focus %s' % (step, args[0]))
        elif what == 'text':
            port.append('%d text %s' % (u, args[0])); orig.append('at %d text %s' % (step, args[0]))
        elif what == 'state':
            port.append('%d state' % u)
    last = events[-1][0] // 10 + 30
    port.append('%d quit' % last)
    os.makedirs(out, exist_ok=True)
    open(os.path.join(out, 'port-%s.txt' % name), 'w', newline='\n').write('\n'.join(port) + '\n')
    open(os.path.join(out, 'orig-%s.txt' % name), 'w', newline='\n').write('\n'.join(orig) + '\n')


def save(reached, lives=0, level=0, score=0, auto=1, difficulty=0, name=b'Player'):
    """a cb.dat (the port's SaveProfiles; 0xbf9c bytes): empty high scores, one player who has reached level `reached` and may have a
    game in progress (lives, level, score)"""
    import struct
    b = b''.join(struct.pack('<32si', b'', 0) for _ in range(8))
    b += struct.pack('<iiii', reached, 3600000, auto, 0)
    for i in range(11):
        if i == 0:
            b += struct.pack('<32siiiiiiiii', name, reached, auto, level, lives, score, 0, 0, 0, 0)
        elif i == 10:
            b += struct.pack('<32siiiiiiiii', b'', 0, 0, 0, 0, 0, 0, difficulty, 0, 0)
        else:
            b += struct.pack('<32siiiiiiiii', b'', 0, 0, 0, 0, 0, 0, 0, 0, 0)
    b += bytes(0x30 * 1000)
    assert len(b) == 0xbf9c, len(b)
    return b


if __name__ == '__main__':
    if sys.argv[1] == 'save':
        out = sys.argv[3]
        open(out, 'wb').write(save(int(sys.argv[2]), *[int(v) for v in sys.argv[4:]]))
    else:
        write(sys.argv[1], sys.argv[2])

