#!/usr/bin/env python3
"""Autosplit-block tests for Typer Shark (games/typershark/docs/GAME_NOTES.md, "The autosplit block"): scripted games with the "livesplit"
mod on, the events and fields that the block and its -livesplitlog show checked against what each game does, and the proof that the block is
inert (the regression games' state logs with the block on are the ones with no mod and in the frozen baseline).

    python games/typershark/tools/livesplit/run_tests.py <out dir>          every run and every check (a few minutes)
    CHECK_ONLY=1 python ... <out dir>                                       the checks only, on the runs an earlier call left there
    INERT=0 python ...                                                      leave out the state-log runs (regress.py, about 2 minutes)

Every game runs under rundesk with -fast, one at a time (tools/gamecount.py's room is waited for). A run's folder <out dir>/<run>/ holds
block.log (-livesplitlog: a line for each write that changes what a splitter acts on), stderr.txt (the "livesplit: game ..." line of each
event) and script.txt. The scripts use the standard test commands (docs/TEST_COMMANDS.md): goto, win, lose, level and press (char SPACE
on the level-won screen returns to the map; press 100 is the menu's Adventure button, `dyes 500` the message dialog's yes).
Needs build/native/typershark.exe, build/native/rundesk.exe, assets/typershark and build/ts-p1/fixtures/port-clean (regress.py makes it).
"""
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
OUT = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else sys.exit(__doc__)
EXE = os.path.join(ROOT, "build", "native", "typershark.exe")
RUNDESK = os.path.join(ROOT, "build", "native", "rundesk.exe")
FIXTURE = os.path.join(ROOT, "build", "ts-p1", "fixtures", "port-clean")
GAMEDIR = os.path.join(ROOT, "assets", "typershark")
failures = []
WON_WAIT = 700


def fail(text):
    failures.append(text)
    print("FAIL: " + text, flush=True)


def room():
    while subprocess.call([sys.executable, os.path.join(ROOT, "tools", "gamecount.py"), "--limit", "6"], stdout=subprocess.DEVNULL) != 0:
        time.sleep(5)


class Script:
    """A script of the standard commands, one at each of the updates that follow 4000 (the game is loaded long before)."""

    def __init__(self):
        self.u = 4000
        self.lines = []

    def at(self, command, gap=300):
        self.lines.append("%d %s" % (self.u, command))
        self.u += gap

    def text(self):
        self.lines.append("%d quit" % (self.u + 100))
        return "\n".join(self.lines) + "\n"


def play(name, script, userdir=None, mods=True):
    """One game: <out>/<name>/. userdir names a users folder to keep (a saved game), else the port-clean fixture is copied."""
    if os.environ.get("ONLY") and name not in os.environ["ONLY"].split(","):
        return
    folder = os.path.join(OUT, name)
    shutil.rmtree(folder, ignore_errors=True)
    os.makedirs(folder)
    ud = userdir or os.path.join(folder, "user")
    if not userdir:
        shutil.copytree(FIXTURE, ud)
    open(os.path.join(folder, "script.txt"), "w", newline="\n").write(script)
    args = [RUNDESK, EXE, "-fast", "-seed=1", "-userdir=" + ud, "-gamedir=" + GAMEDIR, "-script=" + os.path.join(folder, "script.txt"),
            "-modsdir=" + os.path.join(OUT, "mods" if mods else "mods_off"), "-livesplitlog=" + os.path.join(folder, "block.log")]
    room()
    rc = subprocess.call(args, stdout=open(os.path.join(folder, "stdout.txt"), "w"), stderr=open(os.path.join(folder, "stderr.txt"), "w"), cwd=folder)
    print("%s: exit %d" % (name, rc), flush=True)


def adventure(difficulty):
    s = Script()
    s.at("goto menu", 200)
    s.at("goto adventure %d" % difficulty, 200)
    s.at("dyes 0x3f3", 100)            # the level's hint box (a new profile has hints on; it opens 138 updates after a board starts), which pauses the board
    s.at("win", WON_WAIT)              # level 1 won; the treasure and the fade take about 560 updates, then the space bar leaves it
    s.at("char SPACE", 100)            # and the map comes back
    s.at("level 1", 200)               # site 1, a level of its own
    s.at("dyes 0x3f3", 100)
    s.at("win", WON_WAIT)
    s.at("char SPACE", 100)
    s.at("level 2", 200)
    s.at("dyes 0x3f3", 100)
    s.at("lose", 600)                  # the last life: the game over
    return s.text()


def abyss():
    s = Script()
    s.at("goto menu", 200)
    s.at("goto abyss", 200)
    s.at("dyes 0x3f3", 1400)
    s.at("lose", 600)
    return s.text()


def abyss_deep():
    s = Script()
    s.at("goto menu", 200)
    s.at("goto abyss", 200)
    s.at("dyes 0x3f3", 300)
    s.at("level 1200", 400)            # a dive that starts at 1,200 feet, as the "CONTINUE?" dialog's yes does
    s.at("lose", 600)
    return s.text()


def left():
    s = Script()
    s.at("goto menu", 200)
    s.at("goto adventure 1", 200)
    s.at("dyes 0x3f3", 100)
    s.at("win", WON_WAIT)
    s.at("char SPACE", 100)
    s.at("level 1", 200)
    s.at("dyes 0x3f3", 100)
    s.at("press 0xc9", 100)            # the options' "end this game" question
    s.at("dyes 500", 600)              # yes: the game is saved in the profile and the menu comes back
    return s.text()


def resume():
    s = Script()
    s.at("goto menu", 200)
    s.at("press 100", 300)             # the Adventure button: the saved game's "CONTINUE?" question
    s.at("dyes 500", 300)              # yes
    s.at("dyes 0x3f3", 100)
    s.at("lose", 600)
    return s.text()


def won():
    s = Script()
    s.at("goto menu", 200)
    s.at("goto adventure 0", 200)
    s.at("dyes 0x3f3", 100)
    s.at("level 35", 200)              # the last site
    s.at("dyes 0x3f3", 100)
    s.at("win", WON_WAIT)
    s.at("char SPACE", 4000)           # the map's final scroll, then the win dialog
    return s.text()


def events(run):
    p = os.path.join(OUT, run, "stderr.txt")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def count(run, pattern):
    return len(re.findall(r"livesplit: game \d+ " + pattern, events(run)))


def block_lines(run):
    p = os.path.join(OUT, run, "block.log")
    return open(p, errors="replace").read().splitlines() if os.path.exists(p) else []


def field(line, name):
    m = re.search(r"\b" + name + r"=(\S+)", line)
    return m.group(1) if m else None


def own_time(line):
    h = field(line, "own")
    return int.from_bytes(bytes.fromhex(h[:8]), "little") if h else None


def own_depth(line):
    h = field(line, "own")
    return int.from_bytes(bytes.fromhex(h[8:16]), "little") if h else None


def main():
    os.makedirs(OUT, exist_ok=True)
    if not os.path.exists(os.path.join(FIXTURE, "users", "user.idx")):
        sys.exit("no %s: run games/typershark/tools/regress.py once" % FIXTURE)
    if not os.environ.get("CHECK_ONLY"):
        for d in ("mods", "mods_off"):
            shutil.rmtree(os.path.join(OUT, d), ignore_errors=True)
        os.makedirs(os.path.join(OUT, "mods_off"))
        shutil.copytree(os.path.join(ROOT, "games", "typershark", "mods", "livesplit"), os.path.join(OUT, "mods", "livesplit"))
        for d in range(5):
            play("adv_%d" % d, adventure(d))
        play("abyss", abyss())
        play("abyss_deep", abyss_deep())
        play("left", left())
        # the saved game that "left" made is continued by "resume"
        shutil.rmtree(os.path.join(OUT, "resume_user"), ignore_errors=True)
        shutil.copytree(os.path.join(OUT, "left", "user"), os.path.join(OUT, "resume_user"))
        play("resume", resume(), userdir=os.path.join(OUT, "resume_user"))
        play("won", won())

    names = ["Easy", "Normal", "Hard", "Expert", "X-Treme"]
    for d in range(5):
        r = "adv_%d" % d
        if not block_lines(r):
            fail("%s: no block log" % r)
            continue
        if count(r, r"GO at") != 1:
            fail("%s: not exactly one GO" % r)
        if count(r, r"level clear") != 2:
            fail("%s: two level clears expected" % r)
        if count(r, r"game over \(kind 0\)") != 1:
            fail("%s: one game over of kind 0 expected" % r)
        lines = block_lines(r)
        if not any((" mode=0 skill=%d ingame=1 " % d) in l for l in lines):
            fail("%s: mode 0 skill %d (%s) never seen in game" % (r, d, names[d]))
        # progress: 0 at the start, 1 after the first return to the map, 2 after the second
        seen = [field(l, "progress") for l in lines if " mode=0 " in l]
        for want in ("0", "1", "2"):
            if want not in seen:
                fail("%s: progress %s never seen (%s)" % (r, want, ",".join(sorted(set(seen)))))
        # the map is state 20, and a level clear comes before progress rises: on the line with the first lc=1 progress is still 0
        first = next((l for l in lines if " lc=1/" in l), None)
        if first is None or field(first, "progress") != "0":
            fail("%s: the first level clear should show progress 0 (the map is not back yet)" % r)
        if not any(" state=20 " in l and field(l, "progress") == "1" for l in lines):
            fail("%s: the map (state 20) with progress 1 never seen" % r)
    # abyss: a GO, a game over of kind 1, mode 1, the game-over time carried in gameOwn
    r = "abyss"
    if count(r, r"GO at") != 1 or count(r, r"game over \(kind 1\)") != 1:
        fail("abyss: one GO and one game over of kind 1 expected")
    lines = block_lines(r)
    if not any(" mode=1 skill=0 ingame=1 " in l and field(l, "progress") == "-1" for l in lines):
        fail("abyss: mode 1 skill 0 progress -1 never seen in game")
    over = [l for l in lines if re.search(r" over=1/", l) and field(l, "state") == "8"]
    if not over or not own_time(over[-1]):
        fail("abyss: the game over line should carry the game-over screen's time in gameOwn")
    else:
        print("abyss: game-over time %d ms, tick %s" % (own_time(over[-1]), field(over[-1], "tick")))
    # a dive that starts below the surface is not a run
    r = "abyss_deep"
    lines = block_lines(r)
    if count(r, r"GO at") != 1:
        fail("abyss_deep: the surface dive's GO expected once (the dive at 1,200 feet is a second game with none)")
    if not any(own_depth(l) == 1200 and (int(field(l, "flags"), 16) & 2) for l in lines):
        fail("abyss_deep: a game started at 1200 feet with RESUMED set never seen")
    # left from the options: a game, a GO, and no game over; the block shows no game after it
    r = "left"
    if count(r, r"GO at") != 1 or count(r, r"game over") != 0:
        fail("left: one GO and no game over expected")
    if not any(" board=0 " in l and " go=1/1 " in l for l in block_lines(r)):
        fail("left: no board-less line after the GO")
    # resume: no GO, RESUMED set
    r = "resume"
    if count(r, r"GO at") != 0 or not re.search(r"continues a saved game", events(r)):
        fail("resume: a continued game must have no GO")
    if not any((int(field(l, "flags"), 16) & 2) and field(l, "board") != "0" for l in block_lines(r)):
        fail("resume: RESUMED flag not seen")
    # the win dialog
    r = "won"
    if count(r, r"win at") != 1:
        fail("won: one win expected")
    if not any(re.search(r" wins=1/", l) for l in block_lines(r)):
        fail("won: wins=1 never seen")

    # the inert proof: the regression games' state logs with the block on (a folder holding the livesplit mod only) against the expected hashes
    if os.environ.get("INERT", "1") != "0" and not os.environ.get("CHECK_ONLY"):
        env = dict(os.environ)
        env["TSMODSDIR"] = os.path.join(OUT, "mods")
        p = subprocess.run([sys.executable, os.path.join(ROOT, "games", "typershark", "tools", "regress.py"), "--quick"], env=env,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        open(os.path.join(OUT, "inert.txt"), "w").write(p.stdout)
        last = p.stdout.strip().splitlines()[-1] if p.stdout.strip() else "no output"
        print("inert (block on): " + last, flush=True)
        if p.returncode != 0:
            fail("inert: " + last)
    print("livesplit tests: " + ("ok" if not failures else "FAILED (%d)" % len(failures)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
