"""Writes the origrun scripts of Bejeweled's coverage check (docs/COVERAGE.md; GAME_NOTES.md, "Coverage check") into a
folder, one <name>.txt a run. They run on the host, where origrun posts input and takes no shot, so every position comes
from the port's layout (the original's 640x480 client area):

    python games/bejeweled/tools/coverage/scripts.py <out dir>

On the virtual clock a `wait` is game time. A yes/no question's height follows its text, so its button is hit by a
column of clicks at the button's x (Yes at 338, No at 478, the dialogs at DIALOG_X 234, width 348): the first that lands
answers it and the rest fall on the board. No click goes near the original's one link, the Options dialog's "Play More
Great Games!" row (between its Help row at y 216-248 and its Done at 299); board clicks keep out of row 4 (y 244) for it.
"""
import os
import random
import sys

PLAY_LINK = (320, 398)
NEW_GAME = (96, 162)
NORMAL_MODE = (57, 200)
TIMED_MODE = (135, 200)
OPTIONS = (96, 261)
QUIT = (96, 425)
HINT = (90, 340)
OPTIONS_HELP = (331, 230)     # the original's row: dialog top 50 + 166, 32 high
OPTIONS_HIGH_SCORES = (484, 230)
OPTIONS_DONE = (408, 299)
MUSIC_SLIDER = (450, 126)
SFX_SLIDER = (500, 158)
CUSTOM_CURSORS = (396, 190)
HELP_DONE = (320, 399)
PLAY_AGAIN = (415, 401)
YES_X = 338
LOADED = 1 << 24
NO_X = 478


def gem(col, row):
    return (227 + 51 * col, 40 + 51 * row)


class Script:
    def __init__(self, seed):
        self.lines = []
        self.rand = random.Random(seed)

    def add(self, line):
        self.lines.append(line)

    def wait(self, ms):
        self.add('wait %d' % ms)

    def click(self, p, after=600):
        self.add('click %d %d' % p)
        self.wait(after)

    def answer(self, x, top=110, bottom=340):
        """A column of clicks at a yes/no button's x."""
        y = top
        while y <= bottom:
            self.add('click %d %d' % (x, y))
            self.wait(150)
            y += 12
        self.wait(800)

    def swaps(self, count, pause=900):
        """Random swaps of neighbours (a swap that makes no set swaps back)."""
        n = 0
        while n < count:
            col = self.rand.randrange(8)
            row = self.rand.choice([0, 1, 2, 3, 5, 6, 7])
            if self.rand.random() < 0.5 and col < 7:
                other = (col + 1, row)
            elif row < 7 and row != 3:
                other = (col, row + 1)
            else:
                continue
            self.add('click %d %d' % gem(col, row))
            self.wait(80)
            self.add('click %d %d' % gem(*other))
            self.wait(pause)
            n += 1

    def title(self):
        """Waits for the title's loading to finish: on the virtual clock a loading thread that waits on the disk is not
        seen, and a `wait` alone can click Play before the link is there. origrun's counter is the int at 0x53ba60
        (run.sh), whose top byte is the app's loaded flag (0x53b570 + 0x4f3, read by TitleScreen::Update 0x4d3c6a)."""
        self.add('at %d move 320 300' % LOADED)
        self.wait(1500)

    def write(self, path):
        with open(path, 'w', newline='\n') as f:
            f.write('\n'.join(self.lines) + '\n')


def play():
    """A fresh Normal game: the first move's hint, the Hint button, a long stretch of swaps (sets, cascades, level-ups),
    the focus lost and regained, New Game answered No and then Yes, and the window closed during play."""
    s = Script(1)
    s.title()
    s.click(PLAY_LINK, 3000)
    s.click(HINT, 2000)
    s.swaps(150)
    s.click(HINT, 2000)
    s.swaps(150)
    s.click(HINT, 2000)
    s.add('focus lose')
    s.wait(4000)
    s.add('focus regain')
    s.wait(1500)
    s.click(NEW_GAME, 1000)
    s.answer(NO_X)
    s.click(NEW_GAME, 1000)
    s.answer(YES_X)
    s.wait(2000)
    s.swaps(40)
    s.add('quit')
    return s


def dialogs():
    """The focus lost and regained on the title; Options with both sliders, the custom cursors box twice, Help and its
    Done, High Scores and its Done, Options' Done; Timed mode, Normal mode with its question; the Quit button answered No;
    then the window closed with the Options dialog open."""
    s = Script(2)
    s.title()
    s.add('focus lose')
    s.wait(3000)
    s.add('focus regain')
    s.wait(1500)
    s.click(PLAY_LINK, 3000)
    s.click(OPTIONS, 1500)
    s.click(MUSIC_SLIDER, 500)
    s.click(SFX_SLIDER, 500)
    s.click(CUSTOM_CURSORS, 500)
    s.click(CUSTOM_CURSORS, 500)
    s.click(OPTIONS_HELP, 1500)
    s.click(HELP_DONE, 1000)
    s.click(OPTIONS_HIGH_SCORES, 1500)
    y = 300
    while y <= 372:
        s.add('click 408 %d' % y)
        s.wait(150)
        y += 12
    s.wait(800)
    s.click(OPTIONS_DONE, 1000)
    s.click(TIMED_MODE, 1500)
    s.answer(YES_X)
    s.swaps(40)
    s.click(NORMAL_MODE, 1000)
    s.answer(YES_X)
    s.swaps(20)
    s.click(QUIT, 1000)
    s.answer(NO_X)
    s.click(OPTIONS, 1500)
    s.add('quit')
    return s


def timed():
    """A Timed game played a little and then left until its time runs out: the game over, the name typed and Enter, play
    again, and the window closed on the new game's start."""
    s = Script(3)
    s.title()
    s.click(PLAY_LINK, 3000)
    s.click(TIMED_MODE, 1500)
    s.answer(YES_X)
    s.swaps(60)
    s.wait(240000)
    s.add('text Cover')
    s.wait(500)
    s.add('key 13')
    s.wait(3000)
    s.click(PLAY_AGAIN, 3000)
    s.swaps(10)
    s.add('quit')
    return s


def nomoves():
    """A saved Normal game with one move left (nomoves.py --one, the swap of columns 2 and 3 in the top row): the load
    question answered Yes, the move, and if the refill leaves no move (the original's RNG is not pinned on the host, so
    only some runs do), "no more moves", the game over, the name entry, then the window closed on the game-over screen."""
    s = Script(4)
    s.title()
    s.click(PLAY_LINK, 2000)
    s.answer(YES_X)
    s.wait(3000)
    s.click(gem(2, 0), 80)
    s.click(gem(3, 0), 15000)
    s.add('text Cover')
    s.wait(500)
    s.add('key 13')
    s.wait(5000)
    s.add('quit')
    return s


def levelup():
    """A saved Normal game whose score has reached the level's target (nomoves.py with a score): the level-up, then
    swaps on the next level, and the Quit button answered Yes."""
    s = Script(5)
    s.title()
    s.click(PLAY_LINK, 2000)
    s.answer(YES_X)
    s.wait(12000)
    s.swaps(40)
    s.click(QUIT, 1000)
    s.answer(YES_X)
    s.wait(3000)
    s.add('quit')
    return s


def declined():
    """A saved game whose load question is answered No (a new game instead), then the Quit button and Yes from the
    title-less first move."""
    s = Script(6)
    s.title()
    s.click(PLAY_LINK, 2000)
    s.answer(NO_X)
    s.wait(2000)
    s.click(QUIT, 1000)
    s.answer(YES_X)
    s.wait(3000)
    s.add('quit')
    return s


def title_quit():
    """The window closed on the title screen."""
    s = Script(7)
    s.title()
    s.add('quit')
    return s


def realclock():
    """For the run on the real clock with the silent stand-in (the sound code): the title, play and a few swaps."""
    s = Script(8)
    s.wait(8000)
    s.click(PLAY_LINK, 3000)
    s.swaps(25)
    s.add('quit')
    return s


def extra():
    """What the others missed: a key typed on the board during play, and Timed mode clicked with a Normal game in
    progress (its question answered Yes)."""
    s = Script(9)
    s.title()
    s.click(PLAY_LINK, 3000)
    s.swaps(10)
    s.add('text abc')
    s.wait(1000)
    s.click(TIMED_MODE, 1000)
    s.answer(YES_X)
    s.swaps(10)
    s.add('quit')
    return s


def timedconfirm():
    """A saved Normal game in progress (not new) loaded, then Timed mode clicked: its question answered Yes."""
    s = Script(10)
    s.title()
    s.click(PLAY_LINK, 2000)
    s.answer(YES_X)
    s.wait(3000)
    s.click(TIMED_MODE, 1000)
    s.answer(YES_X)
    s.swaps(10)
    s.add('quit')
    return s


RUNS = [play, dialogs, timed, nomoves, levelup, declined, title_quit, realclock, extra, timedconfirm]


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    for run in RUNS:
        run().write(os.path.join(out, run.__name__ + '.txt'))
        print(run.__name__)


if __name__ == '__main__':
    main()
