#!/usr/bin/env python3
"""Plays Typer Shark natively with real input only: the native twin of games/typershark/tools/web/onegame.js.

    python games/typershark/tools/realplay.py [--mode adventure|abyss|tutor] [options]

The rule behind it (the user, 2026-11-05 and 2026-11-07): -script, -autoplay and Node runs never pass the window's input path,
and a port can pass every comparison and still not take a click or a key. So code plays, as a hand would: this starts
build/native/typershark.exe under rundesk (invisible, silent, below normal priority) with -fast, -hold, -commands=<file> and an
empty -realinput file (docs/REAL_INPUT.md), reads the game's state with the test command `state`, gets the move from the port's own
bot with `move` (src/testhooks/Bot.cpp), and sends the move as -realinput lines: SDL events pushed into SDL's own queue, which take
the path a hand's events take (HandleEvent, the window-to-game mapping, the widget manager, Alt+Enter, the focus events). It uses
tools/playbymove/playbymove.py's Game class (starts the exe, appends lines to the commands file and reads each one's answer from
the log), as it plays by its loop, one batch a move (the steps one update apart, "wait 1" between them; the next state and move in
the same batch). A screenshot is taken only to check what is drawn, never to choose a move.

From a clean users folder (the first run of the game) it goes through: the title, the new-user dialog (the name typed with real
keys, a wrong letter and a Backspace first), the WPM question, the menu, the mode's button, and then the whole mode:
  adventure  the difficulty dialog, hints (dismissed by Enter, Space, a click and Escape in turn), the board, the map (Space, or a
             chest's letter at a fork), boss and treasure levels, to --levels N levels won, or to the CONGRATULATIONS box
             (RETURN HOME or, with --win keep, KEEP GOING into the endless levels), or the game over
  abyss      the board and its hints to the game over (or --moves N); the CONTINUE? list when the profile has a deepest depth
  tutor      the lessons (--lessons N, the Next button between them), the VIEW STATS? question (YES once, the statistics screen with
             its buttons and Close; NO after), and Back To Menu with its QUIT? question
The buttons the bot's `move` does not give (it clicks only a one-button box) are in the tables below, in the game's 640x480 pixels
(`click game x y` maps them as the window shows the game, so the same lines work windowed and in full screen).

Options:
  --mode M --diff D --levels N --win home|keep --retry N --depth N --lessons N --wpm yes|no --name NAME --moves N --seconds N
                          as the web driver's ?mode= ?diff= ?levels= ?win= ?retry= ?depth= ?lessons= ?wpm= ?name= ?moves= ?seconds=
  --endless N             with --win keep: stop after N endless levels (3)
  --seed N                the game's seed (-seed=N; 1)
  --focus                 lose and regain the focus once, a few moves into the first game (focus lost / focus gained lines)
  --fullscreen            Alt+Enter (keydown Left Alt, key Return, keyup Left Alt) before the game is played, and again when it is over;
                          the window's pixels are read before, after the first and after the second (screenshotfull: here 2560x1920
                          windowed, 1280x960 full screen under rundesk) and must change each time; the play between is the check that
                          clicks and keys land in full screen
  --steps N               the same as --moves (the number of bot moves; playbymove's name for it)
  --users DIR             a users folder to start from (copied), instead of an empty one: a profile that has played
  --exe PATH              another build of the port (a frozen baseline)
  --gamedir DIR           the game's files (-gamedir=; assets/typershark). The run stops at once with a message when the folder lacks
                          them: a game that cannot find its files waits in a message box on rundesk's desktop and never answers
  --dir DIR               the run's folder (build/ts-p2c/<mode>-<diff>-s<seed>); it holds commands.txt, realinput.txt, game.log, driver.txt
                          (every line sent and its answer), ud/ (the game's user folder) and shots/
  --realtime              without -fast (real time; the game's own speed)
  --timeout N             real seconds to wait for one answer (120); a game that gives none is stopped, with its rundesk, so it frees its
                          game slot, and the run ends with its log's path
  --no-shots              no screenshots
  --level N               start somewhere else by the test command `level N` (docs/TEST_COMMANDS.md), once, when the first board or lesson is
                          up: an adventure site 0 to 35 (35: the last boss and the end), an abyss depth in feet, a tutor lesson 0 to 18;
                          the rest of the run is the bot's play from there
  --lose N                after N bot moves of the first game, the test command `lose` (the last life goes): the game-over box
The last line printed is the summary: realplay typershark: mode, moves, levels won, stop, updates (lines sent and their waits, not
the game's own count), seconds, the screens seen, notes; the exit code is 0 when the run ended as asked (levels, won, gameover,
lessons or moves) and 1 on an error, a screen it does not know, a button that changes nothing, no progress, or an exit of the game.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "playbymove"))
import playbymove as pbm  # noqa: E402

# the game's 640x480 pixels (from the sources' layouts and a shot of each screen)
MENU_BUTTON = {"adventure": (458, 82), "abyss": (458, 186), "tutor": (458, 290)}
DIFF_BUTTON_Y = [164, 209, 254, 299, 344]
DIFF_X = 320
MESSAGE_YES_X, MESSAGE_NO_X = 248, 392
MESSAGE_Y = {"ASSESS WPM?": 341, "VIEW STATS?": 272, "QUIT?": 293}
GAMEOVER = {"adventure": {"again": (238, 333), "menu": (401, 333)}, "abyss": {"again": (238, 374), "menu": (394, 374)}}
WIN_HOME, WIN_KEEP = (237, 434), (416, 434)
ABYSS_OK = (235, 221)
TUTOR_NEXT, TUTOR_BACK_TO_MENU = (612, 395), (551, 459)
STATS_BUTTONS = [(145, 359), (145, 396), (145, 323)]
STATS_CLOSE = (530, 433)
HINT_WAYS = ["Enter", " ", "click", "Escape"]
STALL_UPDATES = 20000
DONE_STOPS = ("levels", "won", "gameover", "lessons", "moves", "endless")


class Fail(Exception):
    pass


def find_ci(base, rel):
    """the path of a '/'-separated rel under base, each part matched without regard to case (as the game finds its files), or None"""
    cur = base
    for part in rel.split("/"):
        try:
            names = os.listdir(cur)
        except OSError:
            return None
        hit = next((n for n in names if n.lower() == part.lower()), None)
        if hit is None:
            return None
        cur = os.path.join(cur, hit)
    return cur


def game_dir_problem(d):
    """why d is not a folder the game can start from (the check of src/main.cpp's LooksLikeGameDir), or None. The game shows a
    message box when it fails that check; on rundesk's private desktop nobody closes it, and the run would wait for ever."""
    if not os.path.isdir(d):
        return ("no game folder %s: it holds the game's images, data, sounds, music and properties folders and WinTS.exe "
                "(tools/setup_worktree.sh stages them in a worktree; --gamedir names another)" % d)
    if not ((find_ci(d, "wints.exe") or find_ci(d, "typershark.exe")) and find_ci(d, "data/twords.txt")
            and find_ci(d, "images/titlescreen.jpg")):
        return ("%s is not Typer Shark's game folder: it needs WinTS.exe or TyperShark.exe, data/twords.txt and "
                "images/titlescreen.jpg (--gamedir names another)" % d)
    return None


def kill_tree(proc):
    """stops the game's process and everything it started (its rundesk and the game under it): a game that no longer answers must
    not keep one of the machine's three game slots"""
    if proc.poll() is not None:
        return
    try:
        subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)], capture_output=True, timeout=30)
    except (OSError, subprocess.SubprocessError):
        pass
    try:
        proc.wait(10)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


def abyss_list(i):
    return (285, 115 + 19 * i)


def click_lines(p):
    """a hand's click: the pointer arrives (a hover, an update), moves on, then presses and releases"""
    x, y = p
    return ["move game %d %d" % (x - 6, y - 6), "wait 2", "move game %d %d" % (x, y), "wait 2", "click game %d %d" % (x, y), "wait 2"]


def key_line(k):
    """playbymove's line for a key; a character that is no letter or digit is typed in quotes: in a commands file a # starts a
    comment, so `type #` typed nothing and the typing tutor's lesson 10 (the first with # in it) never went on"""
    line = pbm.key_line(k)
    if line.startswith("type ") and not k.isalnum():
        return 'type "%s"' % k
    return line


def quoted(line):
    """a `type` line of playbymove's with one character that is no letter or digit, in quotes (see key_line)"""
    if line.startswith("type ") and len(line) == 6 and not line[5].isalnum():
        return 'type "%s"' % line[5]
    return line


def key_lines(k):
    return [key_line(k)]


def type_lines(text):
    out = []
    for c in text:
        out += [key_line(c), "wait 1"]
    return out


class Run:
    def __init__(self, a):
        self.a = a
        self.mode = a.mode
        self.started = time.time()
        self.updates = 0
        self.made = 0
        self.wins = 0
        self.levels_seen = []
        self.screens = {}
        self.notes = {}
        self.stop = None
        self.s = None
        self.m = None
        self.pending = []
        self.tries = {}
        self.hint_count = 0
        self.focus_done = False
        self.focus_after = False
        self.level_sent = False
        self.lose_sent = False
        self.lessons_done = 0
        self.retried = 0
        self.was_won = False
        self.viewed_stats = False
        self.last_sig = ""
        self.last_change = 0
        self.fullscreen = False
        self.shots_taken = set()
        self.shot_n = 0

    # ---- the channel
    def go(self, lines):
        """sends the lines, then the state and the move (one batch: the bot reads the state of the same update)"""
        for l in lines:
            self.updates += int(l.split()[1]) if l.startswith("wait ") else 0
        self.updates += 1
        answers = self.game.send(*(list(lines) + ["state", "move"]))
        self.s = json.loads(answers[-2][len("STATE "):])
        self.m = json.loads(answers[-1][len("MOVE "):])
        return self.s

    def fail(self, what):
        raise Fail("%s | state: %s" % (what, json.dumps(self.s)))

    def attempt(self, key, limit=6):
        self.tries[key] = self.tries.get(key, 0) + 1
        if self.tries[key] > limit:
            self.fail("no change after clicking %s %d times" % (key, limit))

    def done(self, *keys):
        for k in keys:
            self.tries[k] = 0

    def shot(self, name, force=False):
        if self.a.no_shots:
            return
        if not force and name in self.shots_taken:
            return
        self.shots_taken.add(name)
        os.makedirs(self.shots_dir, exist_ok=True)
        # a few updates first: the game draws every 10 updates under -fast, and a screenshot shows the last drawn frame
        self.game.send("wait 10")
        self.game.send("screenshot " + os.path.join(self.shots_dir, "%s-%s.png" % (self.mode, name)).replace("\\", "/"))

    def first_shot(self, name):
        self.shot(name)

    def signature(self, s):
        b = s.get("board") or {}
        tu = s.get("tutor") or {}
        mp = s.get("map") or {}
        return "|".join(str(x) for x in (s.get("screen"), s.get("base"), s.get("dialogHeader"), s.get("level"), s.get("wave"), s.get("score"),
                                         s.get("lives"), s.get("depth"), s.get("won"), b.get("typed"), b.get("clear"), s.get("diver"),
                                         tu.get("state"), tu.get("task"), tu.get("correct"), mp.get("chosen"), mp.get("arrived"),
                                         s.get("endless")))

    def play_lines(self, m):
        """a move's steps as -realinput lines, one update apart (tools/playbymove: "wait 1" between two steps that are not waits)"""
        lines = [quoted(l) for l in pbm.move_lines(m)]
        batch = []
        for line in lines:
            if batch and not batch[-1].startswith("wait ") and not line.startswith("wait "):
                batch.append("wait 1")
            batch.append(line)
        updates = m.get("updates", 1)
        batch.append("wait %d" % (updates if isinstance(updates, int) and updates > 0 else 1))
        return batch

    def alt_enter(self):
        return ["keydown Left Alt", "wait 1", "key Return", "wait 1", "keyup Left Alt", "wait 10"]

    def window_size(self, name):
        """the window's pixels as it shows the game now (screenshotfull saves what the window shows), read from the PNG's header"""
        os.makedirs(self.shots_dir, exist_ok=True)
        path = os.path.join(self.shots_dir, "%s-%s.png" % (self.mode, name)).replace("\\", "/")
        self.game.send("wait 10")
        self.game.send("screenshotfull " + path)
        try:
            with open(path, "rb") as f:
                head = f.read(24)
            w, h = int.from_bytes(head[16:20], "big"), int.from_bytes(head[20:24], "big")
            return "%dx%d" % (w, h)
        except OSError:
            return "no shot"

    def focus_probe(self):
        """the focus lost and regained, a few moves into a game: the game's own pause, and it runs again after"""
        self.focus_done = True
        ans = self.game.send("focus lost", "wait 20", "state", "wait 60", "state")
        a = json.loads(ans[2][len("STATE "):])
        b = json.loads(ans[4][len("STATE "):])
        self.shot("focus-lost", force=True)
        ans = self.game.send("focus gained", "wait 60", "state")
        c = json.loads(ans[2][len("STATE "):])
        self.shot("focus-gained", force=True)
        self.updates += 140
        f = {"lost": {"paused": a.get("paused"), "timer": a.get("timer"), "typed": (a.get("board") or {}).get("typed")},
             "later": {"paused": b.get("paused"), "timer": b.get("timer")}, "gained": {"paused": c.get("paused"), "timer": c.get("timer")}}
        self.notes["focus"] = f
        if c.get("paused"):
            # paused still after the focus came back: a click on the board is what a player does next
            ans = self.game.send(*(click_lines((320, 240)) + ["wait 30", "state"]))
            d = json.loads(ans[-1][len("STATE "):])
            f["afterClick"] = {"paused": d.get("paused"), "timer": d.get("timer")}
            if d.get("paused"):
                self.fail("the game is still paused after the focus came back and a click")

    # ---- the whole run
    def run(self):
        a = self.a
        self.dir = os.path.abspath(a.dir)
        self.shots_dir = os.path.join(self.dir, "shots")
        ud = os.path.join(self.dir, "ud")
        shutil.rmtree(self.dir, ignore_errors=True)
        os.makedirs(self.dir)
        if a.users:
            shutil.copytree(a.users, os.path.join(ud, "users"))
        else:
            os.makedirs(ud)
        gargs = ["-seed=%d" % a.seed, "-userdir=" + ud, "-gamedir=" + os.path.abspath(a.gamedir)]
        pa = argparse.Namespace(dir=self.dir, exe=a.exe, game_args=gargs, realtime=a.realtime, timeout=a.timeout)
        self.game = pbm.Game(pa)
        try:
            self.game.send("state")        # the first answer comes when the game has loaded
            self.loop()
        except (pbm.Stop, Fail) as e:
            self.stop = "error: " + str(e)
        except (ValueError, KeyError, TypeError, IndexError) as e:
            self.stop = "error: an answer or a state the driver cannot read (%r) | state: %s" % (e, json.dumps(self.s))
        finally:
            self.finish()
        return self.report()

    def finish(self):
        if self.stop and self.stop.startswith("error: timeout"):
            # no answer in --timeout seconds: the game is stuck (a message box, a hang), so it will not read a screenshot or a quit
            # either. Stop it and its rundesk now, and say where its log is.
            kill_tree(self.game.proc)
            self.stop += " | the game was stopped; its log: " + self.game.logname
        try:
            if self.fullscreen and self.stop in DONE_STOPS and self.game.proc.poll() is None:
                self.game.send(*self.alt_enter())
                size = self.window_size("window-fullscreen-off")
                ans = self.game.send("state")
                self.notes["fullscreen"] += ", Alt+Enter again: %s (state %s)" % (size, json.loads(ans[0][len("STATE "):]).get("screen"))
        except (pbm.Stop, ValueError, KeyError):
            pass
        os.makedirs(self.shots_dir, exist_ok=True)
        self.game.finish(None if self.a.no_shots else os.path.join(self.shots_dir, "%s-end.png" % self.mode).replace("\\", "/"))

    def report(self):
        a = self.a
        secs = int(round(time.time() - self.started))
        stop = self.stop
        expected = {"adventure": ("levels", "won", "gameover", "endless"), "abyss": ("gameover",), "tutor": ("lessons",)}[a.mode]
        ok = stop == "moves" or stop in expected
        if stop == "gameover" and a.mode == "adventure" and a.levels:
            ok = False       # the levels asked for were not reached
        print("realplay typershark: mode %s diff %d seed %d: %d moves, %d levels won, levels %s, stop %s, ~%d updates, %d s; screens %s; notes %s" % (
            a.mode, a.diff, a.seed, self.made, self.wins, self.levels_seen, stop, self.updates, secs, json.dumps(self.screens, sort_keys=True),
            json.dumps(self.notes, sort_keys=True)))
        if not ok and stop in DONE_STOPS:
            print("realplay: the run ended on '%s', not as asked" % stop)
        return 0 if ok else 1

    def loop(self):
        a = self.a
        if a.fullscreen:
            # the window's pixels (screenshotfull) before, after the first Alt+Enter and, when the game is over, after the second: each
            # must differ from the one before, and the clicks go on landing (the run's own play is the check of that)
            before = self.window_size("window-before")
            self.game.send(*self.alt_enter())
            self.fullscreen = True
            after = self.window_size("window-fullscreen")
            self.notes["fullscreen"] = "window %s, Alt+Enter: %s" % (before, after)
            if before == after:
                self.fail("Alt+Enter did not change the window (%s)" % before)
        max_moves = a.moves or a.steps
        while True:
            self.go(self.pending)
            self.pending = []
            if self.focus_after:
                self.focus_after = False
                self.focus_probe()
                self.go([])
            s, m = self.s, self.m
            scr = s["screen"]
            self.screens[scr] = self.screens.get(scr, 0) + 1
            sig = self.signature(s)
            if sig != self.last_sig:
                self.last_sig, self.last_change = sig, self.updates
            elif self.updates - self.last_change > STALL_UPDATES:
                self.fail("no progress for %d updates" % STALL_UPDATES)
            if time.time() - self.started > a.seconds:
                self.stop = "time"
                break
            if max_moves and self.made >= max_moves:
                self.stop = "moves"
                break
            if self.handle(scr, s, m):
                break

    # one screen's step: appends the next lines to self.pending; True when the run is over
    def handle(self, scr, s, m):
        a = self.a
        if scr in ("title", "none"):
            self.first_shot("title")
            self.pending = ["wait 20"]
            return False

        if scr == "newuser":
            self.first_shot("newuser")
            # a wrong letter and a Backspace first, so that the dialog's text keys are the real thing
            self.pending = type_lines(a.name + "Z") + key_lines("Backspace") + ["wait 1"] + key_lines("Enter") + ["wait 10"]
            self.tries["newuser"] = self.tries.get("newuser", 0) + 1
            if self.tries["newuser"] > 3:
                self.fail("the new-user dialog did not take the name")
            return False
        if s.get("user") is not None and self.tries.get("newuser"):
            if s["user"] != a.name:
                self.fail("the player is %r, not %r" % (s["user"], a.name))
            self.notes["user"] = s["user"]
            self.tries["newuser"] = 0

        if scr == "message":
            head = s.get("dialogHeader")
            y = MESSAGE_Y.get(head)
            self.notes.setdefault("messages", []).append(head)
            self.first_shot("message-" + "".join(c for c in head if c.isalpha()))
            if head == "ASSESS WPM?":
                self.pending = click_lines((MESSAGE_YES_X if a.wpm == "yes" else MESSAGE_NO_X, y)) + ["wait 10"]
                self.notes["wpm"] = a.wpm
                return False
            if head == "VIEW STATS?":
                self.lessons_done += 1
                self.notes.setdefault("lessons", []).append(s.get("level"))
                view = not self.viewed_stats
                self.pending = click_lines((MESSAGE_YES_X if view else MESSAGE_NO_X, y)) + ["wait 20"]
                self.viewed_stats = True
                return False
            if head == "QUIT?" and self.mode == "tutor":
                self.pending = click_lines((MESSAGE_YES_X, y)) + ["wait 20"]
                self.stop = "lessons"
                return False
            # a box with one button: the bot's click (the typing test's "Test Completed!", a notice)
            if m and m.get("what") == "dialog":
                self.pending = self.play_lines(m) + ["wait 10"]
                return False
            self.fail("a message box this driver does not know: %s" % head)

        if scr == "menu":
            if self.stop:
                return True
            self.attempt("menu " + self.mode, 12)
            self.first_shot("menu")
            self.pending = click_lines(MENU_BUTTON[self.mode]) + ["wait 20"]
            return False

        if scr == "difficulty":
            self.done("menu " + self.mode)
            self.attempt("difficulty")
            self.first_shot("difficulty")
            self.pending = click_lines((DIFF_X, DIFF_BUTTON_Y[a.diff])) + ["wait 20"]
            return False

        if scr == "abyssmode":
            if self.mode != "abyss":
                self.fail("the CONTINUE? list outside the abyss")
            self.done("menu " + self.mode)
            self.attempt("abyss list")
            self.first_shot("abyssmode")
            if a.depth > 4:
                self.fail("--depth is the list's visible lines 0 to 4")
            self.pending = (click_lines(abyss_list(a.depth)) + ["wait 5"] if a.depth > 0 else []) + click_lines(ABYSS_OK) + ["wait 20"]
            return False

        if scr == "hint":
            way = HINT_WAYS[self.hint_count % len(HINT_WAYS)]
            self.hint_count += 1
            hints = self.notes.setdefault("hints", [])
            if len(hints) < 12:
                hints.append("%s:%s:%s" % (s.get("base") or "", way, " ".join((s.get("dialogLines") or "")[:40].split())))
            self.first_shot("hint")
            if way == "click":
                if not m or m.get("what") != "dialog":
                    self.fail("the bot has no click for the hint box")
                self.pending = self.play_lines(m) + ["wait 10"]
            else:
                self.pending = key_lines(way) + ["wait 10"]
            self.tries["hint"] = self.tries.get("hint", 0) + 1
            if self.tries["hint"] > 40:
                self.fail("the hint box does not close")
            return False
        self.done("hint")

        if scr in ("options", "credits", "selectuser", "deleteuser", "nowindowed", "highscores"):
            self.fail("a screen this driver does not play: " + scr)

        if scr in ("board", "tutor") and a.level >= 0 and not self.level_sent:
            self.level_sent = True
            ans = self.game.send("level %d" % a.level, "wait 100")
            self.notes["level"] = ans[0]
            self.updates += 100
            self.pending = []
            return False

        if scr == "board":
            self.done("menu " + self.mode, "difficulty", "abyss list")
            if not self.levels_seen or self.levels_seen[-1] != s.get("level"):
                self.levels_seen.append(s.get("level"))
                self.was_won = False
                self.shot("level%s-start" % s.get("level"))
            if s.get("won") and not self.was_won:
                self.was_won = True
                self.wins += 1
                self.shot("level%s-won" % s.get("level"))
            if s.get("level") == -1 and self.mode == "adventure":
                # the endless levels, after KEEP GOING
                if a.win != "keep":
                    self.fail("the endless levels began without the CONGRATULATIONS box")
                if s.get("endless", 0) > a.endless:
                    self.stop = "endless"
                    return True
            if m is None:
                self.pending = ["wait 10"]
                return False
            if m.get("what") == "dialog":
                # a box that opened since the state was read: the next state names it
                self.tries["boxBetween"] = self.tries.get("boxBetween", 0) + 1
                if self.tries["boxBetween"] > 20:
                    self.fail("a dialog the state does not name: " + json.dumps(m))
                self.pending = ["wait 2"]
                return False
            self.tries["boxBetween"] = 0
            self.pending = self.play_lines(m)
            if m.get("what") == "continue":
                # the level is won and the space bar is pressed
                if not self.was_won:
                    self.was_won = True
                    self.wins += 1
                return False
            self.made += 1
            if a.lose and self.made >= a.lose and not self.lose_sent:
                self.lose_sent = True
                self.game.send(*self.pending)
                ans = self.game.send("lose", "wait 60")
                self.notes["lose"] = ans[0]
                self.updates += 60
                self.pending = []
                return False
            if self.made == 6 and a.focus and not self.focus_done:
                self.focus_after = True        # after this move's lines are played
            if self.made % 200 == 0:
                self.shot("move%d" % self.made, force=True)
            return False

        if scr == "map":
            self.done("menu " + self.mode, "difficulty")
            mp = s.get("map") or {}
            if mp.get("chests") and not mp.get("arrived"):
                self.notes["forks"] = self.notes.get("forks", 0) + 1
            if a.levels and self.wins >= a.levels and mp.get("arrived"):
                self.shot("map-stop", force=True)
                self.stop = "levels"
                return True
            if mp.get("arrived"):
                self.first_shot("map-arrived")
            self.was_won = False
            if mp.get("final") and self.mode == "adventure" and s.get("level") != -1:
                # the last site: the map scrolls on its own to the end and the CONGRATULATIONS box opens; the space bar before that
                # starts the endless levels instead, so a player who wants the end waits for it
                self.notes["finalWaits"] = self.notes.get("finalWaits", 0) + 1
                if self.notes["finalWaits"] > 400:
                    self.fail("no CONGRATULATIONS box on the final map")
                self.pending = ["wait 30"]
                return False
            if m is None:
                self.pending = ["wait 20"]
                return False
            self.pending = self.play_lines(m)
            return False

        if scr == "gameover":
            self.first_shot("gameover")
            lay = GAMEOVER["abyss" if s.get("mode") == "abyss" else "adventure"]
            if self.retried < a.retry:
                self.retried += 1
                self.pending = click_lines(lay["again"]) + ["wait 30"]
                self.was_won = False
                return False
            self.stop = "gameover"
            self.attempt("game over main menu", 4)
            self.pending = click_lines(lay["menu"]) + ["wait 30"]
            return False

        if scr == "wingame":
            self.first_shot("wingame")
            if s.get("base") in ("board", "hint"):
                # RETURN HOME started a new game; the box stays in the app's dialog list under the new board (src/WinWSAppButtons.cpp, case 0x85d)
                self.notes["wingameLingers"] = True
                self.stop = "won"
                return True
            if a.levels and self.wins >= a.levels and a.win == "keep":
                self.stop = "levels"
                return True
            if a.win == "keep":
                self.notes["won"] = "keep going"
                self.attempt("keep going", 3)
                self.pending = click_lines(WIN_KEEP) + ["wait 30"]
                return False
            self.notes["won"] = "return home"
            self.game.send(*(click_lines(WIN_HOME) + ["wait 60"]))
            ans = self.game.send("state")
            e = json.loads(ans[0][len("STATE "):])
            self.notes["afterReturnHome"] = {"screen": e.get("screen"), "base": e.get("base"), "level": e.get("level")}
            self.stop = "won"
            return True

        if scr == "tutor":
            self.done("menu " + self.mode)
            if not s.get("over"):
                if m is None:
                    self.pending = ["wait 10"]
                    return False
                self.first_shot("tutor-typing")
                self.pending = self.play_lines(m)
                self.made += 1
                return False
            # the lesson is finished and its questions are answered
            self.first_shot("tutor-finished")
            if self.lessons_done >= a.lessons:
                self.attempt("tutor back to menu", 4)
                self.pending = click_lines(TUTOR_BACK_TO_MENU) + ["wait 20"]
                return False
            self.attempt("tutor next", 4)
            self.pending = click_lines(TUTOR_NEXT) + ["wait 30"]
            self.done("tutor next")
            return False

        if scr == "stats":
            # the statistics after a lesson: the graph buttons, then Close
            self.first_shot("stats")
            lines = []
            for p in STATS_BUTTONS:
                lines += click_lines(p) + ["wait 5"]
            self.game.send(*lines)
            self.shot("stats-wpm")
            self.pending = click_lines(STATS_CLOSE) + ["wait 20"]
            self.attempt("stats close", 3)
            return False

        self.fail("an unknown screen: " + scr)


def main():
    ap = argparse.ArgumentParser(description="Plays Typer Shark natively with real input (-realinput), by the port's own bot.")
    ap.add_argument("--mode", default="adventure", choices=("adventure", "abyss", "tutor"))
    ap.add_argument("--diff", type=int, default=0, choices=range(5))
    ap.add_argument("--levels", type=int, default=0)
    ap.add_argument("--win", default="home", choices=("home", "keep"))
    ap.add_argument("--retry", type=int, default=0)
    ap.add_argument("--endless", type=int, default=3)
    ap.add_argument("--depth", type=int, default=0)
    ap.add_argument("--lessons", type=int, default=1)
    ap.add_argument("--wpm", default="no", choices=("yes", "no"))
    ap.add_argument("--name", default="Driver")
    ap.add_argument("--moves", type=int, default=0)
    ap.add_argument("--steps", type=int, default=0)
    ap.add_argument("--seconds", type=float, default=3600)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--focus", action="store_true")
    ap.add_argument("--fullscreen", action="store_true")
    ap.add_argument("--users")
    ap.add_argument("--exe", default=os.path.join(ROOT, "build", "native", "typershark.exe"))
    ap.add_argument("--gamedir", default=os.path.join(ROOT, "assets", "typershark"))
    ap.add_argument("--dir")
    ap.add_argument("--realtime", action="store_true")
    ap.add_argument("--timeout", type=float, default=120)
    ap.add_argument("--no-shots", action="store_true")
    ap.add_argument("--level", type=int, default=-1)
    ap.add_argument("--lose", type=int, default=0)
    a = ap.parse_args()
    if not a.dir:
        a.dir = os.path.join(ROOT, "build", "ts-p2c", "%s-%d-s%d" % (a.mode, a.diff, a.seed))
    if not os.path.exists(a.exe):
        print("realplay: no exe " + a.exe)
        return 1
    problem = game_dir_problem(a.gamedir)
    if problem:
        print("realplay: " + problem)
        return 1
    return Run(a).run()


if __name__ == "__main__":
    sys.exit(main())
