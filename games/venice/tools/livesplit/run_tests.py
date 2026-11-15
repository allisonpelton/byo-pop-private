#!/usr/bin/env python3
"""Autosplit-block tests for Venice Deluxe (games/venice/docs/GAME_NOTES.md, "The autosplit block"): scripted games with the "livesplit"
mod on, the events and fields that the block and its -livesplitlog show checked against what each game does, and the proof that the block
is inert (the tour of tools/tour_test.txt with the block on gives the screens, state lines and player file that it gives with no mod).

    python games/venice/tools/livesplit/run_tests.py <out dir>          every run and every check (a few minutes)
    CHECK_ONLY=1 python ... <out dir>                                   the checks only, on the runs an earlier call left there
    ONLY=journey,flood python ...                                       only those runs (and no inert proof unless "inert" is named)

Every game runs under rundesk with -fast, one at a time (tools/gamecount.py's room is waited for). A run's folder <out dir>/<run>/ holds
block.log (-livesplitlog: a line for each write that changes what a splitter acts on), stderr.txt (the "livesplit: board ..." line of each
event) and script.txt. A new player is made first (its name typed, then the standard test commands of docs/TEST_COMMANDS.md: unlock all,
set mode, set level), the game's own menus are clicked with posted-like input (move, down, up three frames apart) at the places that the
`buttons` command reports, and the port's bot plays the tutorial and the levels (`bot <frames> continue`). The frames are the script's own:
a frame is one update, and the game is up by update 2000.
Needs build/native/venice.exe, build/native/rundesk.exe and assets/venice.
"""
import json
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
OUT = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else sys.exit(__doc__)
EXE = os.path.join(ROOT, "build", "native", "venice.exe")
RUNDESK = os.path.join(ROOT, "build", "native", "rundesk.exe")
GAMEDIR = os.path.join(ROOT, "assets", "venice")
failures = []
FRAME_BASE = 900          # the first command's frame: after the game is up and the first scripted frame has run

PLAY_NOW = (400, 232)     # the main menu's button
MODE_BUTTON = {1: (400, 220), 0: (401, 270), 3: (398, 320), 2: (400, 370)}   # Journey, Flood, Trick Shot, Survival (mode menu)
CONTINUE = (330, 330)     # the continue prompt's buttons
NEW_GAME = (470, 330)
MAP_CLICK = (400, 300)


def fail(text):
    failures.append(text)
    print("FAIL: " + text, flush=True)


def room():
    while subprocess.call([sys.executable, os.path.join(ROOT, "tools", "gamecount.py"), "--limit", "6"], stdout=subprocess.DEVNULL) != 0:
        time.sleep(5)


def wanted(name):
    only = os.environ.get("ONLY")
    return (not only) or (name in only.split(","))


def click(lines, frame, point):
    lines.append("1 after %d move %d %d" % (FRAME_BASE + frame, point[0], point[1]))
    lines.append("1 after %d down %d %d" % (FRAME_BASE + frame + 3, point[0], point[1]))
    lines.append("1 after %d up %d %d" % (FRAME_BASE + frame + 6, point[0], point[1]))


def make_player(name, commands):
    """A player Test with the profile the commands make (the port makes it: the name typed, the commands, a save and a quit)."""
    folder = os.path.join(OUT, name)
    user = os.path.join(folder, "user")
    shutil.rmtree(folder, ignore_errors=True)
    os.makedirs(user)
    lines = ["3000 key T", "3010 key E", "3020 key S", "3030 key T", "3200 keydown RETURN", "3204 keyup RETURN"]
    lines += ["%d %s" % (3900 + 10 * index, command) for index, command in enumerate(commands)]
    lines += ["%d save" % (3900 + 10 * len(commands)), "5000 quit"]
    script = os.path.join(folder, "profile.txt")
    open(script, "w", newline="\n").write("\n".join(lines) + "\n")
    room()
    subprocess.call([RUNDESK, EXE, "-gamedir=" + GAMEDIR, "-userdir=" + user, "-fast", "-seed=1", "-nomods", "-script=" + script],
                    stdout=open(os.path.join(folder, "profile.out"), "w"), stderr=subprocess.STDOUT, cwd=ROOT)
    return folder, user


def play(name, mode, level, clicks, quit_frame, extras=(), bot=None, mods=True, trace=False):
    """One game: a player on `mode` at the building `level` (1 is a new game), the clicks (frame, point), the commands (frame, text) and the
    bot (frame, frames) that plays; the game quits at the update `quit_frame`. `trace` writes the game's trace (a line for each update: the
    random seed and the values of the game's state) to the run's folder, for the proof that the block is inert."""
    if not wanted(name):
        return
    commands = ["unlock all", "set mode 0", "set level 1", "set mode 1", "set level 1"]
    if level > 1:
        commands += ["set mode %d" % mode, "set level %d" % level]
    commands += ["set mode %d" % mode, "goto menu"]
    folder, user = make_player(name, commands)
    lines = ["1 trace %s" % os.path.join(folder, "trace.txt").replace("\\", "/")] if trace else []
    for frame, point in clicks:
        click(lines, frame, point)
    for frame, text in extras:
        lines.append("1 after %d %s" % (FRAME_BASE + frame, text))
    if bot:
        lines.append("1 after %d bot %d continue" % (FRAME_BASE + bot[0], bot[1]))
    lines.append("%d quit" % quit_frame)
    script = os.path.join(folder, "script.txt")
    open(script, "w", newline="\n").write("\n".join(lines) + "\n")
    shutil.rmtree(os.path.join(folder, "block.log"), ignore_errors=True)
    args = [RUNDESK, EXE, "-fast", "-seed=1", "-userdir=" + user, "-gamedir=" + GAMEDIR, "-script=" + script,
            "-modsdir=" + os.path.join(OUT, "mods" if mods else "mods_off"), "-livesplitlog=" + os.path.join(folder, "block.log")]
    room()
    rc = subprocess.call(args, stdout=open(os.path.join(folder, "stdout.txt"), "w"), stderr=open(os.path.join(folder, "stderr.txt"), "w"), cwd=folder)
    print("%s: exit %d" % (name, rc), flush=True)


def events(run):
    p = os.path.join(OUT, run, "stderr.txt")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def count(run, pattern):
    return len(re.findall(r"livesplit: board \d+ " + pattern, events(run)))


def block_lines(run):
    p = os.path.join(OUT, run, "block.log")
    return open(p, errors="replace").read().splitlines() if os.path.exists(p) else []


def field(line, name):
    m = re.search(r"\b" + name + r"=(\S+)", line)
    return m.group(1) if m else None


def deaths(line):
    return [int(x) for x in field(line, "deaths").split(",")]


def own(line):
    h = field(line, "own")
    return int.from_bytes(bytes.fromhex(h[:8]), "little", signed=True), int.from_bytes(bytes.fromhex(h[8:16]), "little", signed=True)


def states(lines, mode=None):
    return sorted(set(int(field(l, "state")) for l in lines if mode is None or field(l, "mode") == str(mode)))


def expect(run, text, ok):
    if not ok:
        fail("%s: %s" % (run, text))


def check_basics(run, mode, go, clears, overs):
    lines = block_lines(run)
    if not lines:
        fail("%s: no block log" % run)
        return None
    expect(run, "GO count %d expected, %d seen" % (go, count(run, r"GO at")), count(run, r"GO at") == go)
    expect(run, "level clears %d expected, %d seen" % (clears, count(run, r"level clear")), count(run, r"level clear") == clears)
    expect(run, "game overs %d expected, %d seen" % (overs, count(run, r"game over")), count(run, r"game over") == overs)
    in_board = [l for l in lines if field(l, "board") not in ("0", None)]
    expect(run, "mode %d never seen on a board" % mode, any(field(l, "mode") == str(mode) for l in in_board))
    expect(run, "a board's skill is not 0", all(field(l, "skill") == "0" for l in in_board))
    expect(run, "progress is not -1 on a board", all(field(l, "progress") == "-1" for l in in_board))
    expect(run, "the last line is not the inactive write", field(lines[-1], "active") == "0")
    if go:
        first_go = next((l for l in lines if (field(l, "go") or "0/0").split("/")[0] != "0"), None)
        expect(run, "inGame is not 1 from the GO", first_go is not None and field(first_go, "ingame") == "1")
    return lines


def main():
    os.makedirs(OUT, exist_ok=True)
    if not CHECK_ONLY:
        for d in ("mods", "mods_off"):
            shutil.rmtree(os.path.join(OUT, d), ignore_errors=True)
        os.makedirs(os.path.join(OUT, "mods_off"))
        shutil.copytree(os.path.join(ROOT, "games", "venice", "mods", "livesplit"), os.path.join(OUT, "mods", "livesplit"))
        # a new game of a mode: the click on Play Now, the mode's button, and (Journey and Flood) the bot that plays the tutorial and the
        # buildings, from the 2400th frame on (the bot waits for the screens); a win command takes the first building
        for suffix, on in (("", True), ("_off", False)):      # the runs of the inert proof are played again with no mod
            play("journey" + suffix, 1, 1, [(100, PLAY_NOW), (700, MODE_BUTTON[1])], 19000, extras=[(8000, "win")], bot=(1500, 18000), mods=on, trace=True)
            play("flood" + suffix, 0, 1, [(100, PLAY_NOW), (700, MODE_BUTTON[0])], 18500, extras=[(14000, "lose")], bot=(1500, 16000), mods=on, trace=True)
            play("survival" + suffix, 2, 1, [(100, PLAY_NOW), (700, MODE_BUTTON[2])], 9500, extras=[(4000, "lose"), (5500, "goto menu")], mods=on, trace=True)
            play("trickshot" + suffix, 3, 1, [(100, PLAY_NOW), (700, MODE_BUTTON[3])], 14000, bot=(1500, 12000), mods=on, trace=True)
        play("continue", 1, 5, [(100, PLAY_NOW), (700, MODE_BUTTON[1]), (1300, CONTINUE)], 8000, bot=(2200, 4000))
        play("newgame", 1, 5, [(100, PLAY_NOW), (700, MODE_BUTTON[1]), (1300, NEW_GAME)], 14000, bot=(2200, 12000))
        play("congratulations", 1, 72, [(100, PLAY_NOW), (700, MODE_BUTTON[1]), (1300, CONTINUE), (2300, MAP_CLICK), (3200, MAP_CLICK), (4200, MAP_CLICK)],
             9000)

    r = "journey"
    lines = check_basics(r, 1, 1, 1, 1)
    if lines:
        expect(r, "the map (state 1), play (2), won (3), lost (4) and the summary (5) all appear", all(s in states(lines, 1) for s in (1, 2, 3, 4, 5)))
        expect(r, "the game over is not of kind 0", any(deaths(l) == [1, 0, 0, 0] for l in lines))
        expect(r, "no RESUMED flag expected", all((int(field(l, "flags"), 16) & 2) == 0 for l in lines))
        before_go = lines[:next(i for i, l in enumerate(lines) if field(l, "ingame") == "1")]
        expect(r, "the world map (state 1) must be seen before the GO", 1 in states(before_go, 1))
    r = "flood"
    lines = check_basics(r, 0, 1, 1, 1)
    if lines:
        expect(r, "the game over is not of kind 1", any(deaths(l) == [0, 1, 0, 0] for l in lines))
    r = "survival"
    lines = check_basics(r, 2, 1, 0, 1)
    if lines:
        expect(r, "the game over is not of kind 2", any(deaths(l) == [0, 0, 1, 0] for l in lines))
        expect(r, "the board did not end with the return to the main menu", any(field(l, "board") == "0" and field(l, "active") == "1" and
               int(l.split()[0][2:]) < 9000 for l in lines))
    r = "trickshot"
    lines = check_basics(r, 3, 1, count("trickshot", r"level clear"), 0)
    if lines:
        expect(r, "no trick shot made (the bot's attempts)", count(r, r"level clear") >= 1)
        expect(r, "the GO must come at the board's first write", (field(next(l for l in lines if field(l, "board") != "0"), "go") or "").startswith("1/"))
    r = "continue"
    lines = check_basics(r, 1, 0, 0, 0)
    if lines:
        expect(r, "the RESUMED flag is missing", any((int(field(l, "flags"), 16) & 2) != 0 for l in lines))
    r = "newgame"
    lines = check_basics(r, 1, 1, 0, 0)
    if lines:
        resumed = [(int(field(l, "flags"), 16) & 2) != 0 for l in lines if field(l, "board") != "0"]
        expect(r, "the RESUMED flag must be there first and gone before the GO", resumed and resumed[0] and not resumed[-1])
    r = "congratulations"
    lines = check_basics(r, 1, 0, 0, 0)
    if lines:
        expect(r, "one win expected, %d seen" % count(r, r"win at"), count(r, r"win at") == 1)
        expect(r, "the win scene (7) and the honor roll (6) never seen", all(s in states(lines, 1) for s in (6, 7)))

    # the block is inert: the tour of the tour test with the block on gives what it gives with no mod
    if wanted("inert") or not os.environ.get("ONLY"):
        inert()
    print("livesplit: %s" % ("FAILED (%d)" % len(failures) if failures else "ok"))
    return 1 if failures else 0


def tour(name, mods, log):
    folder = os.path.join(OUT, name)
    shutil.rmtree(folder, ignore_errors=True)
    os.makedirs(os.path.join(folder, "user"))
    text = open(os.path.join(ROOT, "games", "venice", "tools", "tour_test.txt")).read().replace("@DIR@", folder.replace("\\", "/"))
    open(os.path.join(folder, "script.txt"), "w", newline="\n").write(text)
    args = [RUNDESK, EXE, "-gamedir=" + GAMEDIR, "-userdir=" + os.path.join(folder, "user"), "-fast", "-seed=1", "-script=" + os.path.join(folder, "script.txt"),
            "-modsdir=" + os.path.join(OUT, "mods" if mods else "mods_off")]
    if log:
        args.append("-livesplitlog=" + os.path.join(folder, "block.log"))
    room()
    subprocess.call(args, stdout=open(os.path.join(folder, "stdout.txt"), "w"), stderr=open(os.path.join(folder, "stderr.txt"), "w"), cwd=folder)
    states_text = [l for l in open(os.path.join(folder, "stdout.txt"), errors="replace").read().splitlines() if re.match(r"(OK|ERROR|STATE) ", l)]
    open(os.path.join(folder, "states.txt"), "w", newline="\n").write("\n".join(states_text) + "\n")


def same_files(a, b):
    names = sorted(f for f in os.listdir(os.path.join(OUT, a)) if f.endswith(".png"))
    if names != sorted(f for f in os.listdir(os.path.join(OUT, b)) if f.endswith(".png")):
        return "the screens differ in number"
    for f in names:
        if open(os.path.join(OUT, a, f), "rb").read() != open(os.path.join(OUT, b, f), "rb").read():
            return "%s differs" % f
    if open(os.path.join(OUT, a, "states.txt")).read() != open(os.path.join(OUT, b, "states.txt")).read():
        return "the state lines differ"
    if open(os.path.join(OUT, a, "user", "profiles.dat"), "rb").read() != open(os.path.join(OUT, b, "user", "profiles.dat"), "rb").read():
        return "the player file differs"
    return None


def inert():
    if not CHECK_ONLY:
        tour("tour_off", False, False)
        tour("tour_on", True, True)
    problem = same_files("tour_off", "tour_on")
    if problem:
        fail("inert: the tour with the block on and with no mod: " + problem)
    expect("tour_on", "the block was never activated", len(block_lines("tour_on")) >= 2)
    # the games of the categories go through the menus, which the tour does not: the same game with the block on and with no mod gives the
    # same trace (the updates' random seed and the game's values) and the same player file
    for run in ("journey", "flood", "survival", "trickshot"):
        a, b = os.path.join(OUT, run), os.path.join(OUT, run + "_off")
        if not (os.path.exists(os.path.join(a, "trace.txt")) and os.path.exists(os.path.join(b, "trace.txt"))):
            fail("inert: no trace of %s" % run)
            continue
        if open(os.path.join(a, "trace.txt"), "rb").read() != open(os.path.join(b, "trace.txt"), "rb").read():
            fail("inert: the trace of %s with the block on differs from the one with no mod" % run)
        if open(os.path.join(a, "user", "profiles.dat"), "rb").read() != open(os.path.join(b, "user", "profiles.dat"), "rb").read():
            fail("inert: the player file of %s with the block on differs" % run)
        expect(run, "no block log of the run with the block on", len(block_lines(run)) > 2)
        expect(run + "_off", "the run with no mod wrote a block log", not block_lines(run + "_off"))
    print("inert: the tour and the four games (traces, state lines, player files, screens) are the same with the block on and with no mod", flush=True)


CHECK_ONLY = bool(os.environ.get("CHECK_ONLY"))
if __name__ == "__main__":
    sys.exit(main())
