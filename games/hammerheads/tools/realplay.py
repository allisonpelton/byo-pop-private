#!/usr/bin/env python3
"""Plays Hammer Heads natively with real input only: a hand's path (docs/REAL_INPUT.md), driven by code.

    python games/hammerheads/tools/realplay.py [--mode classic|marathon|tough] [options]

Starts build/native/hammerheads.exe under rundesk (invisible, silent, below normal priority) with -fast, -hold, -commands=<file> and an
empty -realinput file, reads the game's state with the test command `state`, gets each smash from the port's own bot with `move` and
sends everything as -realinput lines through tools/playbymove's Game class: SDL events pushed into SDL's own queue. From a clean user
folder it clicks the loading screen's button, types the name Test into the new-player box and clicks OK, clicks the mode's button on the
main menu and the how-to-play page's button, then the introduction's and the Ready box's Go buttons, the level summaries, the trophy
room's buttons and the bonus round's, and plays every level with the bot's moves. A screenshot is taken only to check what is drawn,
never to choose a move. The state (the screen and the dialogs' ids) says which button is up; where it does not say (a level's
summary, the trophy award screen) the driver tries the buttons the screens of the first run showed in turn and counts a click that
changes the state; a screen where none does stops the run with the screen named.

Options:
  --mode M          classic (default), marathon (the profile gets `unlock marathon`) or tough (`unlock all`, then the main menu's
                    Tough Cookie box once the game is won; with --levels it is started by `goto tough`)
  --levels N        stop when the level reaches N (default: until the game is won or lost)
  --moves N         stop after N bot moves
  --seed N          the game's seed (1)
  --fullscreen      Alt+Enter after the first level starts, and again at the end: the clicks and moves between land in full screen
  --focus           lose and regain the focus once, a few moves into a level ("focus lost" / "focus gained")
  --heal            the test command `set health 5` whenever the hearts are down to 1 (the bot is not a strong player), to reach later levels
  --lose            the test command `lose` after the first level's moves, to reach the game over box
  --dir DIR         the run's folder (default build/logs/hh/realplay/<mode>-s<seed>)
The last line printed is the summary; the exit code is 0 when the run ended as asked.
"""
import argparse
import json
import os
import shutil
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "playbymove"))
import playbymove as pbm  # noqa: E402

LOADING_PLAY = (397, 532)
NAME_OK = (310, 392)
MENU_BUTTON = {"classic": (140, 320), "marathon": (668, 325)}
HOWTO_PLAY = (300, 550)
INTRO_GO, READY_GO, MARATHON_GO = (400, 438), (400, 344), (400, 388)
# the buttons of the screens that come after a level, tried in turn when the state does not say which one is up
SHOP_BACK = (690, 405)
TRY_BUTTONS = [SHOP_BACK, HOWTO_PLAY, INTRO_GO, READY_GO, MARATHON_GO, (400, 575), (400, 540), (400, 510), (400, 470), (400, 300)]
DIALOG_GAME_OVER, DIALOG_GAME_WON_SCREEN = 15, "gamewon"


class Run:
    def __init__(self, args):
        self.args = args
        self.dir = args.dir
        shutil.rmtree(self.dir, ignore_errors=True)
        ns = argparse.Namespace(dir=self.dir, timeout=120, exe=os.path.join(ROOT, "build", "native", "hammerheads.exe"),
                                realtime=False, game_args=["-gamedir=" + os.path.join(ROOT, "assets", "hammerheads"),
                                                             "-userdir=" + os.path.join(os.path.abspath(self.dir), "user"),
                                                             "-seed=%d" % args.seed])
        self.game = pbm.Game(ns)
        self.notes = []
        self.moves = 0
        self.screens = []

    def say(self, *lines):
        return self.game.send(*lines)

    def state(self):
        return json.loads(self.say("state")[0][len("STATE "):])

    def click(self, p, wait=30):
        self.say("move game %d %d" % p, "wait 2", "click game %d %d" % p, "wait %d" % wait)

    def shot(self, name):
        self.say("screenshot " + os.path.join(os.path.abspath(self.dir), name + ".png"))

    def key_text(self, text):
        for c in text:
            self.say("type " + c, "wait 5")

    def seen(self, s):
        key = s["screen"] + ":" + ",".join(str(d) for d in s["dialogs"])
        if not self.screens or self.screens[-1] != key:
            self.screens.append(key)

    def boot(self):
        a = self.args
        self.say("wait 700")
        self.shot("loading")
        self.click(LOADING_PLAY, 300)
        s = self.state()
        if 5 in s["dialogs"]:
            self.key_text("X")
            self.say("key Backspace", "wait 5")           # a wrong letter and a Backspace, as a hand's
            self.key_text("Test")
            self.shot("name")
            self.click(NAME_OK, 300)
        s = self.state()
        self.seen(s)
        if s["screen"] != "menu":
            raise pbm.Stop("error: the main menu is not up after the name: " + json.dumps(s))
        if a.mode in ("marathon", "tough"):
            self.say("unlock all" if a.mode == "tough" else "unlock marathon")
        self.shot("menu")
        if a.mode == "tough":
            self.say("goto tough")
        else:
            self.click(MENU_BUTTON[a.mode], 300)

    def play(self):
        a = self.args
        self.boot()
        stuck, waits, last, fs_on, focused = 0, 0, None, False, False
        level_seen = 0
        while True:
            s = self.state()
            self.seen(s)
            if a.levels and s["level"] >= a.levels and s["screen"] == "game":
                raise pbm.Stop("over: level %d" % s["level"])
            if s["screen"] == DIALOG_GAME_WON_SCREEN or DIALOG_GAME_OVER in s["dialogs"]:
                self.shot("end")
                raise pbm.Stop("over: " + ("won" if s["screen"] == DIALOG_GAME_WON_SCREEN else "lost"))
            if a.moves and self.moves >= a.moves:
                raise pbm.Stop("limit")
            if s["level"] != level_seen and s["screen"] == "game":
                level_seen = s["level"]
                if a.fullscreen and not fs_on and level_seen >= 1 and self.moves > 0:
                    self.say("keydown Left Alt", "key Return", "keyup Left Alt", "wait 60")
                    fs_on = True
            if a.focus and not focused and self.moves >= 5:
                self.say("focus lost", "wait 60", "focus gained", "wait 30")
                focused = True
            if a.heal and s["screen"] == "game" and 0 < s["health"] <= 2.5 and s["inplay"]:
                self.say("set health 5")
            if a.lose and self.moves >= 6:
                self.say("lose", "wait 120")
                a.lose = False
                continue
            m = None
            if s["screen"] in ("game", "challenge") and not s["dialogs"] and s["paused"] == 0:
                m = json.loads(self.say("move")[0][len("MOVE "):])
            if m:
                lines = pbm.move_lines(m)
                batch = []
                for line in lines:
                    if batch and not batch[-1].startswith("wait ") and not line.startswith("wait "):
                        batch.append("wait 1")
                    batch.append(line)
                self.say(*batch)
                self.moves += 1
                stuck = 0
                continue
            # no smash: a box, the introduction, a summary or a wait for the game: try the buttons in turn
            key = json.dumps(s, sort_keys=True)
            if key == last:
                stuck += 1
            else:
                stuck, last, waits = 0, key, 0
            if stuck == 0 or (not s["dialogs"] and waits < 12):
                waits += 1
                self.say("wait 60")                       # a wipe or an opening is still going on
                continue
            if stuck > 3 * len(TRY_BUTTONS):
                self.shot("stuck")
                raise pbm.Stop("error: no button changes the screen " + s["screen"] + " " + str(s["dialogs"]))
            self.click(TRY_BUTTONS[(stuck - 1) % len(TRY_BUTTONS)], 40)

    def finish(self):
        if self.args.fullscreen:
            try:
                self.say("keydown Left Alt", "key Return", "keyup Left Alt", "wait 30")
            except pbm.Stop:
                pass
        self.game.finish(None)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mode", default="classic", choices=["classic", "marathon", "tough"])
    ap.add_argument("--levels", type=int)
    ap.add_argument("--moves", type=int)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--fullscreen", action="store_true")
    ap.add_argument("--focus", action="store_true")
    ap.add_argument("--lose", action="store_true")
    ap.add_argument("--heal", action="store_true")
    ap.add_argument("--dir")
    args = ap.parse_args()
    if not args.dir:
        args.dir = os.path.join(ROOT, "build", "logs", "hh", "realplay", "%s-s%d" % (args.mode, args.seed))
    run = Run(args)
    stop, started = None, time.time()
    try:
        run.play()
    except pbm.Stop as e:
        stop = str(e)
    finally:
        run.finish()
    print("realplay hammerheads: mode %s, %d moves, stop %s, %d s, screens %s" % (
        args.mode, run.moves, stop, time.time() - started, " > ".join(run.screens[:40])))
    return 0 if stop and (stop.startswith("over") or stop == "limit") else 1


if __name__ == "__main__":
    sys.exit(main())
