# gentours.py <out dir>: writes Water Bugs' screen tours (games/waterbugs/test/tours/*.txt, in the format tour.py reads: a
# time in runner ms, then click/move/rclick/key/shot/end). The buttons are the ones the port's `state` lists
# (games/waterbugs/tools/coverage/scripts.py has the same table). A shot is taken 500 ms after the input that brings a screen up,
# so the picture has settled, and never right at an input (the engine reads the mouse at an update, so a click comes with its
# move 10 ms before).
import os
import sys

START, HELP, OPTIONS, SCORES, USER, QUIT = (400, 303), (400, 350), (400, 397), (400, 444), (400, 491), (400, 537)
HELLO = (234, 363)
O_VIDEO, O_LCD, O_SOUND, O_MUSIC, O_SHOOTER, O_DIFF, O_CREDITS, O_BACK = (400, 77), (400, 137), (400, 197), (400, 257), (400, 317), (400, 377), (400, 437), (400, 537)
HELP_NEXT, CREDITS_BACK, SCORES_BACK = (400, 576), (400, 527), (400, 552)
STORY_GO, RESULT_GO = (400, 467), (400, 425)
RENAME, DELETE, DONE = (316, 452), (400, 452), (484, 452)
OKAY, CANCEL = (340, 330), (460, 330)
MAP_BACK, MAP_START = (750, 575), (400, 571)
WORLDS = [(0, (76, 263)), (10, (101, 486)), (20, (320, 493)), (50, (521, 524)), (100, (711, 371)), (200, (719, 150)),
          (300, (356, 119)), (400, (311, 295)), (500, (516, 256))]
VK = {c: ord(c) for c in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'}
VK.update({'Return': 0x0d, 'Esc': 0x1b, 'Back': 0x08, 'Space': 0x20, 'Dot': 0xbe})


class Tour:
    def __init__(self, title):
        self.lines = ['# ' + l for l in title.strip().split('\n')]
        self.t = 0
        self.prefix = ''
        self.lines.append('20 move 5 5')   # the pointer starts in the corner in both, not on a button

    def at(self, t, what):
        self.t = t
        self.lines.append('%d %s' % (t, what))

    def extra(self, ms, name):
        # a shot ms after the last event, without moving the clock on (the game plays on as it would without it)
        self.lines.append('%d shot %s%s' % (self.t + ms, self.prefix, name))

    def wait(self, ms):
        self.t += ms

    def click(self, p, gap=700, shot=None, shot_after=500):
        self.at(self.t + gap, 'click %d %d' % p)
        if shot:
            self.at(self.t + shot_after, 'shot ' + self.prefix + shot)

    def move(self, p, shot=None, gap=300, shot_after=300):
        self.at(self.t + gap, 'move %d %d' % p)
        if shot:
            self.at(self.t + shot_after, 'shot ' + self.prefix + shot)

    def key(self, k, gap=500, shot=None, shot_after=500):
        self.at(self.t + gap, 'key %d' % VK[k])
        if shot:
            self.at(self.t + shot_after, 'shot ' + self.prefix + shot)

    def text(self, word, gap=250):
        for c in word:
            self.key(c, gap=gap)

    def shot(self, name, gap=0):
        self.at(self.t + gap, 'shot ' + self.prefix + name)

    def end(self, gap=500):
        self.at(self.t + gap, 'end')

    def save(self, out, name):
        with open(os.path.join(out, name + '.txt'), 'w', newline='\n') as f:
            f.write('\n'.join(self.lines) + '\n')


def logos(t):
    # the start-up logos, then the main menu settling
    for k in range(11):
        t.at(100 + 250 * k, 'shot %slogo%d' % (t.prefix, k))
    t.t = 100 + 250 * 10


def menus(t):
    # the start-up logos, the main menu with each item hovered, the help pages, Options (every button but Video, which is the
    # full screen: windowed and full screen are the options' own tour), Credits as they roll, the high scores
    t.prefix = 'm_'
    logos(t)
    t.wait(500)
    t.shot('menu')
    for name, p in (('start', START), ('help', HELP), ('options', OPTIONS), ('scores', SCORES), ('user', USER), ('quit', QUIT), ('sign', HELLO)):
        t.move(p, shot='hover_' + name)
    t.move((100, 100))
    t.click(HELP, shot='help1')
    t.click(HELP_NEXT, shot='help2')
    t.click(HELP_NEXT, shot='help_back')
    t.click(OPTIONS, shot='options')
    t.move(O_LCD, shot='options_hover')
    for n, p in (('lcd', O_LCD), ('sound', O_SOUND), ('music', O_MUSIC), ('shooter', O_SHOOTER)):
        t.click(p, shot='options_%s_on' % n)
        t.click(p, shot='options_%s_off' % n)
    for k in range(3):
        t.click(O_DIFF, shot='options_diff%d' % k)
    t.click(O_DIFF, shot='options_diff_back')
    t.click(O_BACK, shot='options_back')
    t.click(OPTIONS, shot='options_again')
    t.click(O_CREDITS, shot='credits0', shot_after=800)
    for k in range(1, 4):
        t.shot('credits%d' % k, gap=3000)
    t.click(CREDITS_BACK, shot='credits_back')
    t.click(SCORES, shot='scores')
    t.click(SCORES_BACK, shot='scores_back')


def users(t):
    # the player list: the list, a slot's name box typed and confirmed, rename (the box starts empty), the delete question
    # (the only player: a message; with two players: the question answered no and yes), and the box's own buttons hovered
    t.prefix = 'u_'
    t.click(USER, shot='users')
    t.move((400, 202), shot='slot_hover')
    t.click((400, 202), shot='newname_box')
    t.text('ANN')
    t.shot('newname_typed', gap=300)
    t.move(CANCEL, shot='newname_cancel_hover')
    t.click(CANCEL, shot='newname_cancelled')
    t.click((400, 202), shot='newname_box2')
    t.text('ANN')
    t.click(OKAY, shot='newname_done')
    t.click(RENAME, shot='rename_box')
    t.text('BOB')
    t.shot('rename_typed', gap=300)
    t.click(OKAY, shot='rename_done')
    t.click((400, 177), shot='pick_first')
    t.click(DELETE, shot='delete_question')
    t.click(CANCEL, shot='delete_no')
    t.click(DELETE, shot='delete_question2')
    t.click(OKAY, shot='delete_yes')
    t.click(DELETE, shot='delete_only')
    t.click((400, 330), shot='delete_only_ok')   # a message's own button is centred
    t.click(DONE, shot='users_done')
    t.click(DONE, shot='users_left')


def play(t):
    # a new game: the story screen, the first board as it starts (its texts, the beam), ten shots at fixed places, Esc's quit
    # question (the game waits under it), cancelled and answered, back to the main menu
    t.prefix = 'p_'
    t.click(START, shot='map', shot_after=900)
    t.click(WORLDS[0][1], shot='story', shot_after=900)
    t.click(STORY_GO, shot='board0', shot_after=300)
    for k in range(1, 8):
        t.shot('board%d' % k, gap=500)
    pts = [(200, 200), (600, 400), (300, 450), (500, 150), (700, 300), (150, 400), (420, 260), (260, 130), (640, 220), (380, 480)]
    for k, p in enumerate(pts):
        t.move(p, gap=300)
        t.click(p, gap=400, shot='shot%d' % k, shot_after=900)
    t.key('Esc', shot='quit_box')
    t.shot('quit_box_later', gap=1500)
    t.move(CANCEL, shot='quit_hover')
    t.click(CANCEL, shot='quit_cancelled')
    t.shot('board_later', gap=2000)
    t.key('Esc', shot='quit_box2')
    t.click(OKAY, shot='left_to_menu', shot_after=1500)


PTS = [(200, 200), (600, 400), (300, 450), (500, 150), (700, 300), (150, 400), (420, 260), (260, 130), (640, 220), (380, 480)]
MORE = [(590, 340), (510, 400), (680, 300), (500, 260)]   # these fill past the quota


def won(t):
    # a new game again, the first board played to its
    # end: the shots of the play tour and one more until the fill passes the quota (85%), the results as they build up, their
    # button, and the next board's start
    t.prefix = 'w_'
    t.click(START, shot='map', shot_after=900)
    t.click(WORLDS[0][1], shot='story', shot_after=900)
    t.click(STORY_GO, shot='board0', shot_after=300)
    t.wait(4500)
    for k, p in enumerate(PTS + MORE):
        t.move(p, gap=300)
        t.click(p, gap=400, shot='shot%d' % k, shot_after=900 if k < len(PTS) else 300)
    for k in range(16):
        t.shot('won%d' % k, gap=400)
    t.click(RESULT_GO, shot='after_results', shot_after=800)
    for k in range(4):
        t.shot('next%d' % k, gap=700)


def end(out):
    t = Tour('''A saved game with one life left (SAVE=build/wbs/save14.dat: games/waterbugs/tools/coverage/scripts.py save 14 <file> 1 14 3000),
resumed from Start's question, played at fixed shots until the life is lost: the game over, the name entry (keys clicked, a key
typed, rubbed out), the hall of fame with the new score, and back to the main menu.''')
    t.prefix = 'e_'
    t.at(3500, 'shot e_menu')
    t.t = 3500
    t.click(START, shot='resume_question', shot_after=900)
    t.move(OKAY, shot='resume_hover')
    t.click(OKAY, shot='resumed', shot_after=1500)
    t.wait(3000)
    for k in range(3):
        p = (150 + (k * 137) % 520, 120 + (k * 89) % 380)
        t.move(p, gap=300)
        t.click(p, gap=400, shot='shot%d' % k, shot_after=700)
        t.extra(250, 'shot%d_b' % k)
    # the life is lost in the third shot's flight: the game over, the hall of fame fading in with the name entry
    for k in range(12):
        t.shot('over%d' % k, gap=300)
    t.click((400, 300), shot='hof0', shot_after=300)   # a click leaves the game over for the hall of fame
    for k in range(1, 9):
        t.shot('hof%d' % k, gap=300)
    t.key('W', shot='typed_w')
    t.key('Back', shot='typed_back')
    for c in 'ANN':
        t.key(c)
    t.shot('typed_ann', gap=500)
    t.key('Return', shot='entered', shot_after=800)
    t.shot('entered_later', gap=1500)
    t.move(SCORES_BACK, shot='scores_back_hover')
    t.click(SCORES_BACK, shot='after_scores', shot_after=1200)
    t.end(gap=500)
    t.save(out, 'end')


def visit(t, first, point, boards, held=None):
    """from the main menu: Start (a game left on a board asks to resume it: answered no), the world's button, the level selection's
    start button, then `boards` boards skipped with the original's own '.' key, each shot as it starts and a moment later; with
    `held` the original's test keys (a power-up that appears is of the kind of the key held) are held on the first board; then Esc
    and the quit question's okay (the main menu)"""
    t.click(START, shot='%d_resume' % first, shot_after=900)
    t.click(CANCEL, shot='%d_map' % first, shot_after=900)      # on the map the click is nowhere on a world
    t.move(point, gap=500, shot='%d_hover' % first, shot_after=300)
    t.click(point, gap=400, shot='%d_select' % first, shot_after=900)
    t.move(MAP_START, gap=400)
    t.click(MAP_START, gap=400, shot='%d_started' % first, shot_after=1500)
    t.shot('%d_started_later' % first, gap=1500)
    if held:
        t.key('Dot', gap=800, shot='%d_held_start' % first, shot_after=1200)      # the story is skipped: the board itself
        for k, key in enumerate(held):
            t.at(t.t + 300, 'hold %d' % VK[key])
            for j in range(3):
                p2 = (160 + (k * 97 + j * 211) % 480, 140 + (k * 61 + j * 127) % 330)
                if j == 0:      # one line a key (more lose the lives to the bugs, and the game ends before the last keys)
                    t.move(p2, gap=300)
                    t.click(p2, gap=400, shot='%d_held%s_%d' % (first, key, j), shot_after=700)
                else:
                    t.shot('%d_held%s_%d' % (first, key, j), gap=900)
            t.at(t.t + 200, 'release %d' % VK[key])
    for b in range(boards):
        t.key('Dot', gap=1500, shot='%d_skip%d_a' % (first, b), shot_after=700)
        t.shot('%d_skip%d_b' % (first, b), gap=1800)
    t.key('Esc', gap=800, shot='%d_quit' % first, shot_after=600)
    t.click(OKAY, shot='%d_left' % first, shot_after=1500)


def worlds(out):
    t = Tour('''A saved profile that has reached every level (SAVE=build/wbs/save539.dat: scripts.py save 539 <file>): the world map with
each world hovered, a later world's level selection (its buttons, the level stepped up and down), then each world visited in
turn: its first boards skipped through with the original's '.' key (every board of the file is reached without playing it), and
on one world the original's test keys held (power-ups of every kind).''')
    t.prefix = 'x_'
    t.at(3500, 'shot x_menu')
    t.t = 3500
    t.click(START, shot='map', shot_after=900)
    for k, (first, p) in enumerate(WORLDS):
        t.move(p, gap=500, shot='hover_world%d' % first, shot_after=400)
    t.move(MAP_BACK, shot='hover_back')
    t.move(WORLDS[5][1], gap=500)
    t.click(WORLDS[5][1], gap=400, shot='select', shot_after=900)
    t.shot('select_later', gap=700)
    LEFT, RIGHT = (110, 312), (688, 312)
    for k in range(3):
        t.move(RIGHT, gap=400)
        t.click(RIGHT, gap=400, shot='step_up%d' % k, shot_after=400)
    t.move(LEFT, gap=400)
    t.click(LEFT, gap=400, shot='step_down', shot_after=400)
    t.move(MAP_BACK, shot='hover_back2')
    t.move(MAP_START, gap=400, shot='hover_start')
    t.click(MAP_START, gap=400, shot='started', shot_after=1500)
    t.shot('started_later', gap=1500)
    for k in range(3):
        t.key('Space', gap=500, shot='space%d' % k, shot_after=300)
    for b in range(6):
        t.key('Dot', gap=1500, shot='200_skip%d_a' % b, shot_after=700)
        t.shot('200_skip%d_b' % b, gap=1800)
    t.key('Esc', gap=800, shot='200_quit', shot_after=600)
    t.click(OKAY, shot='200_left', shot_after=1500)
    # the world whose boards run with the test keys held goes last: its play ends the saved game's lives, and a Start after
    # that begins a new game (the Instructions screens) instead of asking whether to resume
    order = [w for w in WORLDS if w[0] not in (100, 200)] + [w for w in WORLDS if w[0] == 100]
    for first, p in order:
        visit(t, first, p, 3, list('0123456789ABCD') if first == 100 else None)
    t.end(gap=500)
    t.save(out, 'worlds')


if __name__ == '__main__':
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    t = Tour('''The menus of a fresh profile, the player list, a new game quit from its board, the resume question, and a first board played to its end (SAVE: none).''')
    menus(t)
    users(t)
    play(t)
    won(t)
    t.end(gap=500)
    t.save(out, 'fresh')
    end(out)
    worlds(out)
