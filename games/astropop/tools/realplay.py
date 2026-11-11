#!/usr/bin/env python3
"""Plays AstroPop natively with real input only: a hand's path (docs/REAL_INPUT.md), driven by code (the user, 2026-11-07).

    python games/astropop/tools/realplay.py [--mode classic|survival] [options]

The rule behind it: -script, -autoplay and Node runs never pass the window's input path (SDL's queue, HandleEvent, the
window-to-game mapping, Alt+Enter, the focus events), and a port can pass every comparison and still not take a click or a key.
So this starts build/native/astropop.exe under rundesk (invisible, silent, below normal priority) with -fast, -hold,
-commands=<file> and an empty -realinput file, reads the game's state with the test command `state`, gets the move from the port's
own bot with `move` (src/BoardBot.cpp) and sends it as -realinput lines through tools/playbymove's Game class: SDL events pushed
into SDL's own queue. A screenshot is taken only to check what is drawn, never to choose a move.

From a clean user folder (the game's first run) it goes through the title, the new-player box (a wrong letter and a Backspace
first), the main menu, the mode's button, the pilot selector (and the box of the two gates when the profile has a checkpoint), the
full-screen help, and then the bot plays the game: its moves include the hints, the demo, the stories, the upgrade box, the
weapon boxes and the emergency panel (the kinds of BoardBot.cpp). With --lose N it ends the first game by the test command `lose`
after N moves and clicks the end screen's buttons (Continue, Menu); a survival game ends by itself or, with --moves, after that
many moves and `lose`.

Options:
  --mode M              classic (default) or survival
  --pilot N             0 Vector, 1 Sprocket (2 and 3 need a profile that has earned them: --profile)
  --levels N            classic: stop when the level reaches N (default: stop at --moves)
  --moves N             the bot's moves (150)
  --seed N              the game's seed (-seed=N; 1)
  --focus               lose and regain the focus once, a few moves into the game ("focus lost" / "focus gained")
  --fullscreen          Alt+Enter (keydown Left Alt, key Return, keyup Left Alt) after the first game starts, and again at the
                        end; the window's pixels (screenshotfull) must change size each time; the play between is the check
                        that clicks and keys land in full screen
  --3d                  the registry's Is3D=1 (the 3D drawing; the default is 2D)
  --lose N              after N bot moves the test command `lose`; then the end screen's Continue and Menu
  --profile FILE        a profile file (users/data1.usr) and its registry file (same name, .ini) to start from
  --exe PATH            another build of the port (a frozen baseline)
  --dir DIR             the run's folder (default build/logs/ap/realplay/<mode>-s<seed>): commands.txt, realinput.txt, game.log,
                        driver.txt (every line sent and its answer), user/ and the shots
  --no-shots            no screenshots
The last line printed is the summary: realplay astropop: mode, moves, level, score, stop, updates, the screens seen, notes; the exit
code is 0 when the run ended as asked and 1 on an error, a screen it does not know or a button that changes nothing.
"""
import argparse
import json
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "playbymove"))
sys.path.insert(0, os.path.join(HERE, "origcmp"))
import playbymove as pbm  # noqa: E402
import record  # noqa: E402  (its Recorder: the update accounting of a batch)

# the game's 640x480 pixels (from the sources' layouts and a shot of each screen)
TITLE_PLAY = (320, 447)
MENU_BUTTON = {"classic": (100, 190), "survival": (100, 330)}
PILOT_GO = [(80, 418), (399, 418), (559, 418), (239, 418)]
HELP_CONTINUE = (320, 440)
END_CONTINUE, END_MENU = (278, 449), (490, 449)       # the end screen of a game: Continue / Restart, and Menu
END_RESTART = (279, 449)


class Fail(Exception):
    pass


def click_lines(p):
    """a hand's click: the pointer arrives (a hover, an update), then presses and releases"""
    x, y = p
    return ["move game %d %d" % (x - 5, y - 5), "wait 2", "move game %d %d" % (x, y), "wait 2", "click game %d %d" % (x, y), "wait 4"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mode", default="classic", choices=("classic", "survival"))
    ap.add_argument("--pilot", type=int, default=0)
    ap.add_argument("--levels", type=int, default=None)
    ap.add_argument("--moves", type=int, default=150)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--focus", action="store_true")
    ap.add_argument("--fullscreen", action="store_true")
    ap.add_argument("--3d", dest="threed", action="store_true")
    ap.add_argument("--lose", type=int, default=None)
    ap.add_argument("--profile", default=None)
    ap.add_argument("--exe", default=os.path.join(ROOT, "build", "native", "astropop.exe"))
    ap.add_argument("--dir", default=None)
    ap.add_argument("--no-shots", action="store_true")
    a = ap.parse_args()
    out = os.path.abspath(a.dir or os.path.join(ROOT, "build", "logs", "ap", "realplay", "%s-s%d%s%s" % (
        a.mode, a.seed, "-fs" if a.fullscreen else "", "-3d" if a.threed else "")))
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, "shots"))
    userdir = os.path.join(out, "user")
    os.makedirs(userdir)
    if a.profile:
        os.makedirs(os.path.join(userdir, "users"))
        shutil.copy(a.profile, os.path.join(userdir, "users", "data1.usr"))
        shutil.copy(os.path.splitext(a.profile)[0] + ".ini", os.path.join(userdir, "registry.ini"))
    else:
        with open(os.path.join(userdir, "registry.ini"), "w") as f:
            f.write("[SOFTWARE\\PopCap\\AstroPop]\nIs3D=dword:%d\nScreenMode=dword:0\n" % (1 if a.threed else 0))
    ns = argparse.Namespace(dir=out, timeout=180, exe=a.exe, realtime=False,
                            game_args=["-seed=%d" % a.seed, "-trace", "-userdir=" + userdir])
    record.wait_room()
    game = pbm.Game(ns)
    r = record.Recorder(game)
    seen, notes = [], []
    moves, stop, state = 0, None, {}

    def shot(name):
        if not a.no_shots:
            game.send("screenshot " + os.path.join(out, "shots", name + ".png"))
            r.u += 1        # a screenshot runs at the next update

    def play(lines, query=("state",)):
        merged = []     # two waits in a row would cost an update more than their sum: one wait of the sum
        for l in lines:
            if merged and l.startswith("wait ") and merged[-1].startswith("wait "):
                merged[-1] = "wait %d" % (int(merged[-1].split()[1]) + int(l.split()[1]))
            else:
                merged.append(l)
        ans = r.play(merged, query)
        st = json.loads(ans[0][len("STATE "):])
        if st.get("screen") not in seen:
            seen.append(st.get("screen"))
        return st

    def expect(st, key, value, what):
        if st.get(key) != value:
            raise Fail("%s: %s is %r, not %r" % (what, key, st.get(key), value))

    def full_screen_check(label):
        """Alt+Enter as a hand does it: the window's own pixels change size each time"""
        before = os.path.join(out, "shots", "full-%s-before.png" % label)
        after = os.path.join(out, "shots", "full-%s-after.png" % label)
        game.send("screenshotfull " + before)
        r.u += 1
        st = play(["keydown Left Alt", "wait 2", "key Return", "wait 2", "keyup Left Alt", "wait 30"])
        game.send("screenshotfull " + after)
        r.u += 1
        from PIL import Image
        sa, sb = Image.open(before).size, Image.open(after).size
        notes.append("alt+enter %s: window %dx%d -> %dx%d" % (label, sa[0], sa[1], sb[0], sb[1]))
        if sa == sb:
            raise Fail("Alt+Enter changed nothing: the window stayed %dx%d" % sa)
        return st

    try:
        st = game.send("state")
        r.u = json.loads(st[0][len("STATE "):])["appupdates"]
        # the title: the loading is done by the first update of a -fast native run, but give it a moment
        st = play(["wait 60"])
        expect(st, "screen", "title", "start")
        shot("title")
        if not a.profile:
            st = play(click_lines(TITLE_PLAY) + ["wait 40"])
            shot("newuser")
            # the player's name with real keys: a wrong letter and a Backspace first
            st = play(["type X", "wait 4", "key Backspace", "wait 4"] + sum((["type %s" % c, "wait 3"] for c in "Tester"), [])
                      + ["key Return", "wait 60"])
            expect(st, "screen", "menu", "after the name")
            expect(st, "user", "Tester", "the name typed")
        else:
            st = play(click_lines(TITLE_PLAY) + ["wait 60"])
            expect(st, "screen", "menu", "after the title")
        shot("menu")
        st = play(click_lines(MENU_BUTTON[a.mode]) + ["wait 40"])
        shot("pilots")
        st = play(click_lines(PILOT_GO[a.pilot]) + ["wait 40"])
        gate = st.get("checkpoint")
        if gate:
            st = play(click_lines((gate[1], gate[2])) + ["wait 40"])
        st = play(click_lines(HELP_CONTINUE) + ["wait 20"])
        expect(st, "screen", "game", "after the help")
        if a.fullscreen:
            st = full_screen_check("in")
        # the bot, as tools/playbymove's loop plays it
        batch, nulls, focused = [], 0, False
        while True:
            ans = r.play(batch, ("state", "move"))
            state = json.loads(ans[0][len("STATE "):])
            if state.get("screen") not in seen:
                seen.append(state.get("screen"))
            if state.get("over") is True or state.get("over") == 1:
                stop = "over"
                break
            if a.levels is not None and a.mode == "classic" and state.get("level", 0) >= a.levels:
                stop = "levels"
                break
            if moves >= a.moves or (a.lose is not None and moves >= a.lose):
                stop = "limit"
                break
            if a.focus and not focused and moves >= 5:
                focused = True
                st1 = play(["focus lost", "wait 60"])
                notes.append("focus lost: paused=%s" % st1.get("paused"))
                shot("focus-lost")
                st2 = play(["focus gained", "wait 60"])
                notes.append("focus gained: paused=%s" % st2.get("paused"))
                shot("focus-gained")
                if st1.get("paused") != 1:
                    raise Fail("the game did not pause when the focus was lost")
                batch = []
                continue
            m = json.loads(ans[1][len("MOVE "):])
            if m is None:
                nulls += 1
                if nulls >= 40:
                    stop = "no move"
                    break
                batch = ["wait 30"]
                continue
            nulls = 0
            lines = pbm.move_lines(m)
            if lines:
                moves += 1
            batch = []
            for line in lines:
                if batch and not batch[-1].startswith("wait ") and not line.startswith("wait "):
                    batch.append("wait 1")
                batch.append(line)
            upd = m.get("updates", 1)
            batch.append("wait %d" % (upd if isinstance(upd, int) and upd > 0 else 1))
        shot("end-of-play")
        if a.lose is not None and stop == "limit":
            game.send("lose")
            r.u += 1        # a test command runs at the next update
            st = play(["wait 400"])
            if st.get("over") not in (1, True):
                raise Fail("`lose` did not end the game (over is %r)" % st.get("over"))
            stop = "lose"
        if stop in ("over", "lose"):
            # the end screen: Continue (a new game of the mode) and then Menu, by real clicks
            shot("end-screen")
            st = play(click_lines(END_CONTINUE) + ["wait 60"])
            if st.get("screen") != "game":
                raise Fail("Continue on the end screen did not start a game (screen %s)" % st.get("screen"))
            notes.append("Continue: a new game (screen game)")
            shot("new-game")
            st = play(click_lines(HELP_CONTINUE) + ["wait 40"])
            # a game that went deep continues from its checkpoint ("Continue from level 5?"), and the first brick of a new kind opens
            # its hint box, which holds the game (paused) until its OK is clicked: the bot's moves click it, and play on, until it runs
            for k in range(6):
                if st.get("paused") != 1:
                    break
                ans = r.play([], ("state", "move"))
                m = json.loads(ans[1][len("MOVE "):])
                if m is None:
                    st = play(["wait 20"])
                    continue
                batch = []
                for line in pbm.move_lines(m):
                    if batch and not batch[-1].startswith("wait ") and not line.startswith("wait "):
                        batch.append("wait 1")
                    batch.append(line)
                upd = m.get("updates", 1)
                batch.append("wait %d" % (upd if isinstance(upd, int) and upd > 0 else 1))
                st = play(batch)
                notes.append("a box at the start of the new game: the bot's move %d" % (k + 1))
            # out by the options box: Escape, Main Menu, Yes
            st = play(["key Escape", "wait 40"] + click_lines((230, 388)) + ["wait 20"] + click_lines((255, 279)) + ["wait 80"])
            expect(st, "screen", "menu", "after Main Menu, Yes")
            notes.append("Main Menu, Yes: the menu")
            shot("menu-again")
        if a.fullscreen:
            full_screen_check("out")
        print("realplay astropop: %s, %d moves, level %s, score %s, stop %s, %d updates, screens %s; %s" % (
            a.mode, moves, state.get("level"), state.get("score"), stop, r.u, ",".join(str(s) for s in seen), "; ".join(notes)))
        code = 0
    except (Fail, pbm.Stop) as e:
        print("realplay astropop: FAILED after %d moves (%d updates): %s" % (moves, r.u, e))
        code = 1
    finally:
        game.finish(None if a.no_shots else os.path.join(out, "shots", "last.png"))
    return code


if __name__ == "__main__":
    sys.exit(main())
