#!/usr/bin/env python3
"""A short scripted game for the autosplit block's tests (src/LiveSplitObserver.cpp), on the shared driver loop
(tools/playbymove/playbymove.py's Game: the live -commands channel, -hold, -fast, real input under rundesk).

    python games/bejeweled2/tools/livesplit/drive.py <scenario> <out dir> [game args...]

The scenario names, each a game or two with the port's bot playing and the test commands (docs/TEST_COMMANDS.md) laying out the
rest (win: the level's goal is met, or the puzzle solved; lose: the game's own ending; goto: another mode from wherever the game is):
    classic     Classic: the bot's moves, level 1 won, level 2 won, a loss (no moves)
    action      Action: the same; the loss is the time running out
    hyper       Hyper: the same
    endless     Endless: the same
    puzzle      Puzzle: planet 1's five puzzles solved (the bot picks the pods) and the warp; stops at 6 puzzles solved
    puzzle20    Puzzle: the first four planets, twenty puzzles, each planet's warp (the 25% category)
    foursquare  four levels of every mode (Classic, Action, Puzzle's puzzles, Endless, Hyper), each left by the next's goto
    switcharoo  Action and Endless taken in turns, a level each, from Action's: eight levels (the Category Extension's Switcharoo)
    left        Classic, some moves, the main menu (the game is left), Classic again (the saved game is continued: no GO) and a loss
    cognito     Cognito: three puzzles solved
    hypercube   Classic: the board laid out (gem) so that one move makes a line of five, which makes a hyper cube
The game is build/native/bejeweled2.exe, or the exe in $BJ2_EXE (a copy, so that a build can go on beside the tests).
<out dir> gets the game's folder: driver.txt, game.log (the game's "livesplit:" lines are in it), block.log (-livesplitlog).
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "playbymove"))
import playbymove as pbm  # noqa: E402


class Run:
    def __init__(self, out, game_args, steps_limit=100000):
        args = pbm.parse_args(["--dir", out, "--timeout", "120", os.environ.get("BJ2_EXE", os.path.join(ROOT, "build", "native", "bejeweled2.exe"))] + game_args)
        self.game = pbm.Game(args)
        self.g = self.game
        self.moves = 0
        self.load_answer = "yes"    # the "Load Game?" question of a mode left in the middle of a game: yes continues it, no starts again

    def cmd(self, *lines):
        return self.g.send(*lines)

    def ok(self, line):
        a = self.cmd(line)[-1]
        if not a.startswith("OK"):
            raise RuntimeError('"%s": %s' % (line, a))

    def state(self):
        return json.loads(self.cmd("state")[-1][len("STATE "):])

    def wait(self, n):
        self.cmd("wait %d" % n)

    def move(self):
        """One bot move (or a wait when it has none)"""
        answers = self.cmd("state", "move")
        m = json.loads(answers[-1][len("MOVE "):])
        if m is None:
            self.cmd("wait 5")
            return None
        lines = pbm.move_lines(m)
        batch = []
        for line in lines:
            if batch and not batch[-1].startswith("wait ") and not line.startswith("wait "):
                batch.append("wait 1")
            batch.append(line)
        updates = m.get("updates", 1)
        batch.append("wait %d" % (updates if isinstance(updates, int) and updates > 0 else 1))
        self.cmd(*batch)
        self.moves += 1
        return m

    def bot(self, n):
        for _ in range(n):
            self.move()

    def until(self, what, ok, limit=3000):
        for _ in range(limit):
            s = self.state()
            if ok(s):
                return s
            if 17 in (s.get("dialogs") or []) and self.load_answer in (s.get("buttons") or {}):
                x, y = s["buttons"][self.load_answer]
                self.cmd("move game %d %d" % (x, y), "wait 2", "click game %d %d" % (x, y), "wait 5")
                continue
            self.move()
        raise RuntimeError("timeout waiting for " + what + ": " + json.dumps(self.state()))

    def board_idle(self, mode=None):
        """The bot plays (dialogs, the how-to-play screen, tips) until the board is idle in the mode"""
        return self.until("an idle board", lambda s: s.get("screen") == "game" and s.get("phase") == "idle" and
                          (mode is None or s.get("mode") == mode) and not s.get("dialogs"))

    def start(self, mode):
        self.ok("goto " + mode)
        self.board_idle(mode)

    def win(self):
        """The level's goal (or the puzzle) is won once the board idles; then the level change is played through"""
        for _ in range(40):
            self.board_idle()
            try:
                self.cmd("win")
                break
            except pbm.Stop as e:   # an ERROR answer is a Stop of the driver loop; the game goes on
                if "busy" not in str(e):
                    raise
            self.wait(30)   # the bot's own moves filled the level's bar (the board changed between state and win): its change plays first
        self.wait(2)

    def level_done(self, level):
        self.until("level %d" % level, lambda s: s.get("screen") == "game" and s.get("phase") == "idle" and
                   s.get("level", 0) >= level and not s.get("dialogs"))

    def lose(self):
        self.board_idle()
        self.ok("lose")
        self.wait(1)
        self.until("the game over", lambda s: s.get("over") is True or s.get("phase") in ("game over", "game over fall"), 600)

    def finish(self):
        self.g.finish(None)


def levels(r, mode, count, first=1):
    """count levels won in the mode (a level each: win, then the next level's board)"""
    for i in range(count):
        r.bot(6)
        r.win()
        r.level_done(first + i + 1)


def scenario_modeloss(mode, kind_moves=12):
    def f(r):
        r.start(mode)
        r.bot(kind_moves)
        levels(r, mode, 2)
        r.bot(6)
        r.lose()
        r.wait(300)
    return f


def solve_puzzles(r, count, stop_after=None):
    """Puzzle mode: the bot picks the pods and warps; win solves the puzzle on an idle board"""
    solved = 0
    for _ in range(8000):
        s = r.state()
        if s.get("screen") not in ("game", "dialog"):
            raise RuntimeError("left the game: " + json.dumps(s))
        if solved >= count:
            return
        if s.get("dialogs"):
            r.move()
            continue
        if s.get("phase") == "idle":
            r.ok("win")
            solved += 1
            r.wait(3)
            continue
        r.move()
    raise RuntimeError("the puzzles did not go on")


def sc_puzzle(r):
    r.start("puzzle")
    solve_puzzles(r, 6)
    r.bot(30)


def sc_puzzle20(r):
    r.start("puzzle")
    solve_puzzles(r, 20)
    r.bot(150)  # the fourth planet's warp


def sc_foursquare(r):
    for mode in ("classic", "action", "endless", "hyper"):
        r.start(mode)
        levels(r, mode, 4)
    r.start("puzzle")
    solve_puzzles(r, 4)
    r.wait(100)


def sc_switcharoo(r):
    r.start("action")
    for lap in range(4):
        r.bot(6)
        r.win()
        r.wait(50)
        r.start("endless")
        r.bot(6)
        r.win()
        r.wait(50)
        if lap < 3:
            r.start("action")


def sc_left(r):
    r.start("classic")
    r.bot(12)
    r.ok("goto menu")
    r.wait(300)
    r.ok("goto classic")
    r.until("the saved game's board or its question", lambda s: s.get("screen") in ("game", "dialog"))
    r.board_idle("classic")
    r.bot(6)
    r.lose()
    r.wait(300)


def sc_cognito(r):
    r.start("cognito")
    solve_puzzles(r, 3)
    r.bot(30)


def triples(board):
    """The cells of any three in a row or column of one colour"""
    cells = set()
    for r in range(8):
        for c in range(8):
            ch = board[r][c]
            if ch == ".":
                continue
            if c <= 5 and board[r][c + 1] == ch and board[r][c + 2] == ch:
                cells |= {(r, c), (r, c + 1), (r, c + 2)}
            if r <= 5 and board[r + 1][c] == ch and board[r + 2][c] == ch:
                cells |= {(r, c), (r + 1, c), (r + 2, c)}
    return cells


def lay_out(r, cells):
    """gem <row> <col> <colour> for each (row, col, colour); the board finds any set on its next update"""
    for row, col, colour in cells:
        r.ok("gem %d %d %d" % (row, col, colour))


def hypercube_state(r):
    """A board where one swap makes a line of five, which makes a hyper cube (the site's test state; docs/TEST_COMMANDS.md, gem):
    rows 2 to 4 of columns 0 to 4 are set to
        row 2: 0 1 0 1 0      row 3: 3 3 2 3 3      row 4: 1 0 3 0 1
    (no three of a colour anywhere in them), the rest of the board is made free of three in a row by recolouring, and swapping (row 3,
    column 2) with (row 4, column 2) makes row 3 five 3s. Returns the swap as ((col, row), (col, row))."""
    stencil = {(2, 0): 0, (2, 1): 1, (2, 2): 0, (2, 3): 1, (2, 4): 0,
               (3, 0): 3, (3, 1): 3, (3, 2): 2, (3, 3): 3, (3, 4): 3,
               (4, 0): 1, (4, 1): 0, (4, 2): 3, (4, 3): 0, (4, 4): 1}
    for (row, col), colour in sorted(stencil.items()):
        r.ok("gem %d %d %d" % (row, col, colour))
    for _ in range(40):
        board = r.state()["board"]
        bad = [cell for cell in triples(board) if cell not in stencil]
        if not bad:
            return board
        row, col = bad[0]
        # a colour that none of the four neighbours has, and that the stencil's colours do not meet
        near = {board[a][b] for a, b in ((row - 1, col), (row + 1, col), (row, col - 1), (row, col + 1)) if 0 <= a < 8 and 0 <= b < 8}
        pick = next(k for k in (4, 5, 6, 1, 0, 2, 3) if str(k) not in near)
        r.ok("gem %d %d %d" % (row, col, pick))
    raise RuntimeError("the rest of the board keeps making sets")


def sc_hypercube(r):
    r.start("classic")
    r.wait(50)
    board = hypercube_state(r)
    # the bot's own swap gives the board's pixels: the first gem's centre and one square's distance
    for _ in range(40):
        m = json.loads(r.cmd("state", "move")[-1][len("MOVE "):])
        if m and m.get("kind") == "swap":
            break
        r.wait(5)
    x1, y1 = m["input"][1]["click"]
    x2, y2 = m["input"][3]["click"]
    (c1, r1), (c2, r2) = m["from"], m["to"]
    px = abs(x2 - x1) if c2 != c1 else abs(y2 - y1)

    def pos(col, row):
        return x1 + (col - c1) * px, y1 + (row - r1) * px
    a, b = pos(2, 3), pos(2, 4)
    r.cmd("move game %d %d" % a, "wait 2", "click game %d %d" % a, "wait 2", "move game %d %d" % b, "wait 2", "click game %d %d" % b, "wait 4")
    for _ in range(60):
        s = r.state()
        if any("c" in row for row in s["board"]):
            print("hypercube: made at update %s: %s" % (s.get("tick"), json.dumps(s["board"])))
            r.wait(100)
            return
        r.wait(2)
    raise RuntimeError("no hyper cube: " + json.dumps(r.state().get("board")))


SCENARIOS = {
    "classic": scenario_modeloss("classic"), "action": scenario_modeloss("action"), "hyper": scenario_modeloss("hyper"),
    "endless": scenario_modeloss("endless"), "puzzle": sc_puzzle, "puzzle20": sc_puzzle20, "foursquare": sc_foursquare,
    "switcharoo": sc_switcharoo, "left": sc_left, "cognito": sc_cognito, "hypercube": sc_hypercube,
}


def main():
    if len(sys.argv) < 3 or sys.argv[1] not in SCENARIOS:
        print(__doc__)
        return 2
    name, out = sys.argv[1], os.path.abspath(sys.argv[2])
    extra = sys.argv[3:]
    os.makedirs(out, exist_ok=True)
    r = Run(out, extra)
    status = 0
    try:
        r.cmd("unlock all")
        SCENARIOS[name](r)
        print("drive %s: ok, %d bot moves" % (name, r.moves))
    except Exception as e:  # noqa: BLE001
        print("drive %s: %s" % (name, e))
        status = 1
    finally:
        r.finish()
    return status


if __name__ == "__main__":
    sys.exit(main())
