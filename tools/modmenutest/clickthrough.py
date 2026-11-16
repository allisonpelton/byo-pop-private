#!/usr/bin/env python3
"""Clicks through the open mod menu and checks that nothing reaches the game (docs/MOD_MENU_DESIGN.md 5.4).

    python -I tools/modmenutest/clickthrough.py [options] <game> [game arguments...]

The user, 2026-11-13: clicks in Venice's mod menu went through it to the game below. This driver shows whether they
still do, in any port built on SexyAppBase. It starts build/native/<game>.exe under rundesk (as tools/playbymove does:
the live channel, an empty -realinput file, -hold and -fast), with a fresh user folder and a mods folder that holds a
copy of games/<game>/mods (and, with --fixtures, the two fixture mods of the menu's own tests). Then:

  1. the --first lines put the game where it is played (a level, a board), each answered in turn;
  2. F2 opens the menu, as a hand's key does (a -realinput key, through HandleEvent);
  3. "state" is asked, the game's own test command (docs/TEST_COMMANDS.md);
  4. keys are pressed on the list page, as a hand presses them (letters, digits and the keys that only move the
     menu's focus), and every place of the menu that a click only looks at is clicked, as a hand clicks: a move, the
     press, three updates, the release. The places come from "modmenu targets" (the menu's own layout): each mod's
     row, each setting's label, each mod's Settings or About button (its page), the page's Back, and a grid of points
     (over the whole screen on the list page, the dimmed game and the panel; over the panel on a mod's page) that
     lies on no control that changes a value, a mod or the game (a switch, the order buttons, a value's parts,
     Restart, Reset, Close). Each page is visited in turn, and PageDown scrolls a long one: the rows it brings into
     view are clicked too;
  5. "state" again: it must be the state of step 3, key for key (--ignore leaves keys out): the game is paused while
     the menu is open, and no click reached it;
  6. the menu's Close button closes it (a click too), and after --after updates "state" once more: the --same keys
     (none by default; Venice's moves and lives, a game's shots or turns) must be those of step 5, so the click that
     closed the menu reached nothing either.

With --points x,y;x,y... the clicks are those game points instead of step 4's (a build without "modmenu targets",
before the fix), and F2 closes the menu in step 6.

Everything goes into --dir (build/modmenutest/clickthrough/<game> by default): the game's log (game.log) and the
transcript (driver.txt). The last line printed is the summary:
    clickthrough <game>: <n> clicks and <k> keys on <p> pages; paused <ok|CHANGED: ...>; after close <ok|CHANGED: ...>
The exit code is 0 when both checks hold, 1 when one fails, 2 when the run could not get there.

One game process. Count first: python -I tools/gamecount.py --limit.
"""

import argparse
import json
import os
import shutil
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tools", "playbymove"))
import playbymove  # noqa: E402

# The controls a click on changes nothing but the menu's focus: a mod's row, a setting's label. Every other control
# changes a value, a mod, the page, the scroll or the game, so the grid's points avoid them; the driver clicks a page's
# Settings or About button and its Back itself, to visit the pages
LOOK_KINDS = ("row", "label")
GRID_STEP = 40
# Keys pressed on the list page (SDL's names): none changes a mod there (Space would switch the focused one, Enter
# open its page, Esc and F2 close the menu)
KEYS = [chr(c) for c in range(ord("A"), ord("Z") + 1)] + [str(d) for d in range(10)] + [
    "Up", "Down", "Home", "End", "PageUp", "PageDown", "Tab", "Left", "Right"]


def collect(game, line, prefix):
    """Sends one line and returns the answer line that starts with prefix (before its OK), or None"""
    with open(game.commands, "ab") as f:
        f.write(line.encode("utf-8") + b"\n")
    game.transcript.write("> " + line + "\n")
    word = line.split()[0]
    found = None
    start = time.time()
    while True:
        for raw in game.lines():
            text = raw[len(playbymove.ANSWER_PREFIX):] if raw.startswith(playbymove.ANSWER_PREFIX) else raw
            if text.startswith(prefix):
                found = text[len(prefix):]
                game.transcript.write("< " + text[:300] + "\n")
            elif text.startswith("OK " + word) or text.startswith("ERROR " + word):
                game.transcript.write("< " + text + "\n")
                game.transcript.flush()
                if text.startswith("ERROR "):
                    raise playbymove.Stop("error: " + text)
                return found
        if game.proc.poll() is not None:
            raise playbymove.Stop("exited (code %d) waiting for '%s'" % (game.proc.returncode, line))
        if time.time() - start > game.timeout:
            raise playbymove.Stop("timeout: no answer to '%s' in %d s" % (line, game.timeout))
        time.sleep(0.002)


def state(game):
    return json.loads(game.send("state")[0][len("STATE "):])


def targets(game):
    answer = collect(game, "modmenu targets", "MODMENU ")
    return None if answer is None else json.loads(answer)


def click(game, x, y):
    # a hand's click: the move, the press held for three updates (a game that polls the buttons reads them once an
    # update), the release
    game.send("move game %d %d" % (x, y), "wait 1", "down game %d %d" % (x, y), "wait 3", "up game %d %d" % (x, y),
              "wait 2")


def inside(x, y, r):
    return r[0] <= x < r[0] + r[2] and r[1] <= y < r[1] + r[3]


def centre(r):
    return r[0] + r[2] // 2, r[1] + r[3] // 2


def grid(menu, whole_screen):
    """Points a GRID_STEP apart that lie on no control a click changes something with: over the whole screen (the
    list page), or over the panel only (a mod's page: the dimmed game around it is as it was on the list)"""
    w, h = menu["screen"]
    area = (0, 0, w, h) if whole_screen else menu["panel"]
    avoid = [t["rect"] for t in menu["targets"] if t["kind"] not in LOOK_KINDS]
    points = []
    for y in range(GRID_STEP // 2, h, GRID_STEP):
        for x in range(GRID_STEP // 2, w, GRID_STEP):
            # 4 pixels around a control too: a click at its edge is still meant for it
            if inside(x, y, area) and not any(inside(x, y, (r[0] - 4, r[1] - 4, r[2] + 8, r[3] + 8)) for r in avoid):
                points.append((x, y))
    return points


def differences(a, b, keys=None, ignore=()):
    names = keys if keys is not None else sorted(set(a) | set(b))
    return [k for k in names if k not in ignore and a.get(k) != b.get(k)]


def make_mods(args, mods_dir):
    shutil.rmtree(mods_dir, ignore_errors=True)
    os.makedirs(mods_dir)
    sources = []
    own = os.path.join(ROOT, "games", args.game, "mods")
    if os.path.isdir(own):
        sources += [os.path.join(own, d) for d in sorted(os.listdir(own))]
    if args.fixtures:
        sources += [os.path.join(ROOT, "tools", "fixtures", "mods", "settings-demo"),
                    os.path.join(ROOT, "tools", "modmenutest", "mods", "plain-demo")]
    for s in sources:
        if os.path.isfile(os.path.join(s, "mod.json")):
            shutil.copytree(s, os.path.join(mods_dir, os.path.basename(s)))


def parse_args(argv=None):
    ap = argparse.ArgumentParser(description="Clicks through the open mod menu and checks that the game did not move.")
    ap.add_argument("--dir", help="the run's folder (default build/modmenutest/clickthrough/<game>)")
    ap.add_argument("--first", action="append", default=[], help="a line sent before the menu opens (repeatable)")
    ap.add_argument("--ignore", action="append", default=[],
                    help="a state key the paused check leaves out (repeatable)")
    ap.add_argument("--same", action="append", default=[], help="a state key that must not change once the menu has "
                    "closed (repeatable)")
    ap.add_argument("--after", type=int, default=30, help="updates after the close before the last state (30)")
    ap.add_argument("--points", help="game points x,y;x,y... clicked instead of the menu's own places; F2 closes")
    ap.add_argument("--fixtures", action="store_true",
                    help="also the menu tests' fixture mods (settings-demo, plain-demo)")
    ap.add_argument("--max-clicks", type=int, default=2000, help="at most this many clicks (2000)")
    ap.add_argument("--shot", help="a screenshot (PNG) with the menu open, after the clicks")
    ap.add_argument("--timeout", type=float, default=60, help="real seconds to wait for one answer (60)")
    ap.add_argument("--realtime", action="store_true", help="without -fast")
    ap.add_argument("game")
    ap.add_argument("game_args", nargs=argparse.REMAINDER)
    args = ap.parse_args(argv)
    if not args.dir:
        args.dir = os.path.join(ROOT, "build", "modmenutest", "clickthrough", args.game)
    return args


def main(argv=None):
    args = parse_args(argv)
    run_dir = os.path.abspath(args.dir)
    os.makedirs(run_dir, exist_ok=True)
    user_dir = os.path.join(run_dir, "user")
    shutil.rmtree(user_dir, ignore_errors=True)
    os.makedirs(user_dir)
    mods_dir = os.path.join(run_dir, "mods")
    make_mods(args, mods_dir)
    exe = os.path.join(ROOT, "build", "native", args.game + ".exe")
    game_args = ["-gamedir=" + os.path.join(ROOT, "assets", args.game), "-userdir=" + user_dir,
                 "-modsdir=" + mods_dir] + args.game_args
    run = argparse.Namespace(dir=run_dir, timeout=args.timeout, exe=exe, game_args=game_args, realtime=args.realtime)
    game = playbymove.Game(run)
    clicks, keys, pages, paused, closed, stop = 0, 0, set(), None, None, None
    try:
        for line in args.first:
            game.send(line)
        game.send("key F2", "wait 2")
        before = state(game)
        if args.points:
            for p in args.points.split(";"):
                x, y = (int(v) for v in p.split(","))
                click(game, x, y)
                clicks += 1
            pages.add("")
        else:
            menu = targets(game)
            if menu is None:
                raise playbymove.Stop("error: the menu did not open (F2)")
            # Keys first, on the list page: letters, digits and the keys that only move the menu's focus or scroll
            # (the menu takes them all; a game that reads keys must see none)
            for name in KEYS:
                game.send("key " + name, "wait 1")
                keys += 1
            # The pages: the list, then each mod's (its Settings or About button); on each, its rows and labels, then
            # the grid; PageDown scrolls a long page, and the rows it brings into view are clicked too; Back (the
            # header's) ends a mod's page
            mods = [t for t in targets(game)["targets"] if t["kind"] == "open"]
            for page in [None] + mods:
                if clicks >= args.max_clicks:
                    break
                if page is not None:
                    menu = targets(game)
                    button = next((t for t in menu["targets"] if t["kind"] == "open" and t["item"] == page["item"]),
                                  None)
                    if button is None:
                        continue
                    click(game, *centre(button["rect"]))
                    clicks += 1
                menu = targets(game)
                pages.add(menu["page"])
                points = [{"rect": (x, y, 1, 1)} for x, y in grid(menu, page is None)]
                seen = set()
                while True:
                    looks = [t for t in menu["targets"]
                             if t["kind"] in LOOK_KINDS and (t["kind"], t["item"]) not in seen]
                    seen |= set((t["kind"], t["item"]) for t in looks)
                    for t in looks + points:
                        if clicks >= args.max_clicks:
                            break
                        click(game, *centre(t["rect"]))
                        clicks += 1
                    points = []
                    # the clicks only moved the focus: the page is as it was
                    if targets(game)["page"] != menu["page"]:
                        raise playbymove.Stop("error: a click left the page '%s'" % menu["page"])
                    game.send("key PageDown", "wait 1")
                    keys += 1
                    menu = targets(game)
                    if all((t["kind"], t["item"]) in seen for t in menu["targets"] if t["kind"] in LOOK_KINDS):
                        break
                if page is not None:
                    menu = targets(game)
                    back = next((t for t in menu["targets"] if t["kind"] == "back"), None)
                    if back is not None:
                        click(game, *centre(back["rect"]))
                        clicks += 1
        if args.shot:
            game.send("screenshot " + os.path.abspath(args.shot))
        during = state(game)
        changed = differences(before, during, ignore=args.ignore)
        paused = "ok" if not changed else "CHANGED: " + ", ".join(
            "%s %s -> %s" % (k, json.dumps(before.get(k))[:60], json.dumps(during.get(k))[:60]) for k in changed)
        # the close: the list page's Close button, a click
        if args.points:
            game.send("key F2")
        else:
            menu = targets(game)
            close = next((t for t in menu["targets"] if t["kind"] == "close"), None) if menu else None
            if close is None:
                raise playbymove.Stop("error: no Close button on the menu's page '%s'" % (menu or {}).get("page"))
            click(game, *centre(close["rect"]))
            clicks += 1
            if targets(game) is not None:
                raise playbymove.Stop("error: the menu is still open after its Close button")
        game.send("wait %d" % args.after)
        after = state(game)
        changed = differences(during, after, keys=args.same) if args.same else []
        closed = "ok" if not changed else "CHANGED: " + ", ".join(
            "%s %s -> %s" % (k, json.dumps(during.get(k))[:60], json.dumps(after.get(k))[:60]) for k in changed)
    except playbymove.Stop as e:
        stop = str(e)
    except (ValueError, KeyError, TypeError, IndexError) as e:
        stop = "error: an answer the driver cannot read (%s)" % e
    finally:
        game.finish(None)
    if stop:
        print("clickthrough %s: %d clicks; stop %s" % (args.game, clicks, stop))
        return 2
    print("clickthrough %s: %d clicks and %d keys on %d pages; paused %s; after close %s" % (
        args.game, clicks, keys, len(pages), paused, closed))
    return 0 if paused == "ok" and closed == "ok" else 1


if __name__ == "__main__":
    sys.exit(main())
