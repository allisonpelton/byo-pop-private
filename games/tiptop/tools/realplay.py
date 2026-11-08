#!/usr/bin/env python
"""realplay.py: TipTop played with real input, natively (games/tiptop/docs/IDENTICAL_PLAY.md, "Real play").

The port runs under rundesk (invisible, silent, low priority) with -fast, -realinput=<an empty file> and
-commands=<file>: every mouse move, click and key a driver sends is an SDL event pushed into the game's queue, the
way a hand's are, and the driver reads the game's state and the bot's next move (docs/TEST_COMMANDS.md) from the log.
Code plays; a session only reads the result lines.

  realplay.py tour <N> [--out DIR]       replays phase-two tour N (games/tiptop/tools/shots/tourN.txt) as real events
                                         and writes its shots to DIR/tourN/ (default build/rt). A line's events are
                                         handled after its update, where -script's run before it, so --shift -1 puts
                                         each event line one update earlier and the two runs' frames line up
  realplay.py play <mode> [options]      a whole game of regular|strategy|speed|timed from the title screen by the
                                         bot's `move`: the title's button, the menu, the mode, the tips, every block,
                                         the level summaries and the high score's name; ends when the game is over
      --fullscreen-at N   Alt+Enter after N moves (and back at the same count again later with --windowed-at M)
      --windowed-at M
      --focus-at N        the window loses the focus after N moves and regains it 60 updates later
      --steps N           give up after N moves (default 20000)
      --seed N            the game's -seed (the game's own random numbers are reseeded by `seed` anyway)
      --botseed N         the bot's own dice (set botseed)
      --real-time         run without -fast (the game's own clock, 100 updates a second)
      --modsdir DIR       -modsdir=DIR: a folder holding the fixes mod (a mods folder is empty by default, and the game
                          then plays as the original)

Each run writes DIR/<name>/ (log.txt, cmds.txt, the user dir, shots) and prints one RESULT line: the mode, the moves
made, the level and score reached, whether the game came to its end screen, and the shot of the high scores.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip()).resolve()
EXE = ROOT / "build/native/tiptop.exe"
RUNDESK = ROOT / "build/native/rundesk.exe"
GAMEDIR = ROOT / "assets/tiptop"
ANSWER = re.compile(r"^(?:INFO: )?(STATE |MOVE |OK |ERROR )")


class GameEnded(Exception):
    pass


class Game:
    def __init__(self, out, seed=1, fast=True, extra=()):
        self.out = Path(out)
        if self.out.exists():
            shutil.rmtree(self.out)
        (self.out / "user").mkdir(parents=True)
        self.cf = self.out / "cmds.txt"
        self.empty = self.out / "empty.txt"
        self.logpath = self.out / "log.txt"
        self.cf.write_text("")
        self.empty.write_text("")
        cmd = [str(RUNDESK), str(EXE), f"-gamedir={GAMEDIR}", f"-userdir={self.out / 'user'}",
               f"-commands={self.cf}", f"-realinput={self.empty}", f"-seed={seed}"]
        if fast:
            cmd.append("-fast")
        cmd += list(extra)
        self.logf = open(self.logpath, "wb")
        self.p = subprocess.Popen(cmd, stdout=self.logf, stderr=subprocess.STDOUT)
        self.pos = 0
        self.pending = b""

    def read_lines(self):
        with open(self.logpath, "rb") as f:
            f.seek(self.pos)
            data = f.read()
        self.pos += len(data)
        data = self.pending + data
        parts = data.split(b"\n")
        self.pending = parts.pop()
        return [x.decode("utf-8", "replace").rstrip("\r") for x in parts]

    def wait_for_text(self, text, timeout):
        end = time.time() + timeout
        while time.time() < end:
            for line in self.read_lines():
                if text in line:
                    return line
            if self.p.poll() is not None:
                raise GameEnded("the game ended before: " + text)
            time.sleep(0.02)
        raise TimeoutError("no '%s' within %d s" % (text, timeout))

    def send(self, lines, timeout=60):
        """Appends the lines to the commands file and returns the log lines up to the last answer."""
        with open(self.cf, "a", newline="\n") as f:
            f.write("".join(l + "\n" for l in lines))
        want = len(lines)
        got = []
        answers = 0
        end = time.time() + timeout
        while answers < want:
            if time.time() > end:
                raise TimeoutError("no answer to %r within %d s" % (lines, timeout))
            fresh = self.read_lines()
            for line in fresh:
                got.append(line)
                if ANSWER.match(line):
                    answers += 1
            if answers < want:
                if self.p.poll() is not None:
                    raise GameEnded("the game ended while waiting for %r" % (lines,))
                time.sleep(0.01)
        for line in got:
            if re.match(r"^(INFO: )?ERROR ", line):
                print("  driver: " + line)
        return got

    def state(self):
        got = self.send(["state"])
        last = [l for l in got if ANSWER.match(l)][-1]
        last = re.sub(r"^INFO: ", "", last)
        if not last.startswith("STATE "):
            raise RuntimeError("state: " + last)
        return json.loads(last[6:])

    def botmove(self):
        got = self.send(["move"])
        last = [l for l in got if ANSWER.match(l)][-1]
        last = re.sub(r"^INFO: ", "", last)
        if not last.startswith("MOVE "):
            raise RuntimeError("move: " + last)
        return json.loads(last[5:])

    def wait_updates(self, n):
        s = self.state()
        target = s["updates"] + n
        while s["updates"] < target:
            s = self.state()
        return s

    def play(self, mv):
        """One MOVE as real events: its ordered input steps, then its updates."""
        for st in mv.get("input", []):
            if "move" in st:
                self.send(["move game %d %d" % tuple(st["move"])])
            elif "click" in st:
                self.send(["click game %d %d%s" % (st["click"][0], st["click"][1],
                                                  " right" if st.get("button") == "right" else "")])
            elif "press" in st:
                self.send(["down game %d %d" % tuple(st["press"])])
            elif "release" in st:
                self.send(["up game %d %d" % tuple(st["release"])])
            elif "key" in st:
                k = st["key"]
                # a typed letter is its text (SDL's key names have no case): type gives the shifted key its capital
                self.send([("type " + k) if len(k) == 1 and k.isalnum() else ("key " + k)])
            elif "wait" in st:
                self.wait_updates(int(st["wait"]))
        self.wait_updates(max(1, int(mv.get("updates", 1))))

    def alt_enter(self):
        self.send(["keydown Left Alt"])
        self.send(["key Return"])
        self.send(["keyup Left Alt"])

    def shot(self, name):
        path = self.out / (name + ".png")
        self.send(["screenshot " + str(path).replace("\\", "/")])
        return path

    def fullshot(self, name):
        """The whole window (screenshotfull): its size says windowed (the game's 640 x 480) or full screen (the screen's)."""
        path = self.out / (name + ".png")
        self.send(["screenshotfull " + str(path).replace("\\", "/")])
        try:
            from PIL import Image
            return list(Image.open(path).size)
        except Exception:
            return None

    def close(self):
        try:
            if self.p.poll() is None:
                self.send(["quit"], timeout=10)
        except Exception:
            pass
        try:
            self.p.wait(timeout=20)
        except subprocess.TimeoutExpired:
            self.p.kill()
        self.logf.close()


def tour(args):
    n = args.name
    out = Path(args.out or ROOT / "build/rt") / ("tour" + n)
    src = ROOT / "games/tiptop/tools/shots" / ("tour" + n + ".txt")
    shotdir = out / "port"
    shotdir.mkdir(parents=True, exist_ok=True)
    made = subprocess.check_output([sys.executable, str(ROOT / "games/tiptop/tools/shots/make.py"), str(src),
                                    out.as_posix(), "port"], text=True)
    # make.py's lines are -script's: turn them into -realinput's (points in game pixels, the focus words, typed keys)
    lines = []
    for line in made.splitlines():
        m = re.match(r"^(\d+) (\w+) ?(.*)$", line)
        if not m:
            continue
        upd, cmd, rest = m.groups()
        ev = str(max(0, int(upd) + args.shift))
        if cmd in ("click", "move"):
            lines.append("%s %s game %s" % (ev, cmd, rest))
        elif cmd == "char":
            lines.append("%s type %s" % (ev, rest))
        elif cmd == "key":
            lines.append("%s key %s" % (ev, rest))
        elif cmd == "focus":
            fe = str(max(0, int(upd) + (args.shift if args.focus_shift is None else args.focus_shift)))
            lines.append("%s focus %s" % (fe, {"lose": "lost", "regain": "gained"}[rest.strip()]))
        else:
            lines.append(line)
    lines.sort(key=lambda l: int(l.split()[0]))  # stable: a shift can put an event before a shot of the same line
    rt = out.parent / ("tour" + n + ".realinput")
    rt.write_text("\n".join(lines) + "\n")
    userdir = out / "user"
    if userdir.exists():
        shutil.rmtree(userdir)
    userdir.mkdir(parents=True)
    log = out / "log.txt"
    cmd = [str(RUNDESK), str(EXE), f"-gamedir={GAMEDIR}", f"-userdir={userdir}", f"-realinput={rt}", "-seed=1", "-fast"]
    if args.modsdir:
        cmd.append("-modsdir=" + str(Path(args.modsdir).resolve()))
    t0 = time.time()
    with open(log, "wb") as lf:
        p = subprocess.run(cmd, stdout=lf, stderr=subprocess.STDOUT, timeout=600)
    count = len(list(shotdir.glob("*.png")))
    print("RESULT tour %s: exit %d, %d real-input lines, %d shots, %.0f s" % (n, p.returncode, len(lines), count, time.time() - t0))


def play(args):
    mode = args.name
    out = Path(args.out or ROOT / "build/rt") / ("play-" + mode + ("-fs" if args.fullscreen_at is not None else ""))
    extra = ["-modsdir=" + str(Path(args.modsdir).resolve())] if args.modsdir else []
    g = Game(out, seed=args.seed, fast=not args.real_time, extra=extra)
    moves = 0
    t0 = time.time()
    result = {"mode": mode}
    try:
        g.send(["set botseed %d" % args.botseed])
        if mode == "timed":
            # the timed mode's button is off the screen: through the title and the menu with the bot's own moves,
            # then the game's own way in
            while g.state()["screen"] != "menu":
                g.play(g.botmove())
            g.send(["goto timed"])
        else:
            g.send(["set mode " + mode])
        nulls = 0
        fullscreen = False
        focus_back_at = None
        while moves < args.steps:
            st = g.state()
            if st.get("over"):
                break
            if args.fullscreen_at is not None and moves == args.fullscreen_at and not fullscreen:
                g.alt_enter()
                fullscreen = True
                g.shot("fullscreen")
                result["window_after_alt_enter"] = g.fullshot("fullscreen_window")
            if args.windowed_at is not None and moves == args.windowed_at and fullscreen:
                g.alt_enter()
                fullscreen = False
                g.shot("windowed_again")
                result["window_after_alt_enter_back"] = g.fullshot("windowed_window")
            if args.focus_at is not None and moves == args.focus_at:
                g.send(["focus lost"])
                g.wait_updates(60)
                result["paused_after_focus_lost"] = g.state().get("paused")
                g.shot("focus_lost")
                g.send(["focus gained"])
                g.wait_updates(60)
                g.shot("focus_gained")
            mv = g.botmove()
            if mv is None:
                nulls += 1
                if nulls >= 2:
                    break
                g.wait_updates(5)
                continue
            nulls = 0
            g.play(mv)
            moves += 1
            if moves % 100 == 0:
                print("  %d moves: %s level %s score %s" % (moves, st["screen"], st.get("level"), st.get("score")), flush=True)
        final = g.state()
        result.update({"moves": moves, "over": bool(final.get("over")), "screen": final["screen"],
                       "level": final.get("level"), "score": final.get("score")})
        # the high score list as the game shows it after the name was typed
        if final.get("over"):
            g.send(["goto highscores"])
            g.wait_updates(30)
            g.shot("highscores")
            result["highscores_shot"] = str(out / "highscores.png")
        g.shot("end")
    except (GameEnded, TimeoutError, RuntimeError) as e:
        result["error"] = str(e)
    finally:
        g.close()
    result["seconds"] = round(time.time() - t0)
    print("RESULT " + json.dumps(result))
    return 0 if result.get("over") else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("what", choices=["tour", "play"])
    ap.add_argument("name")
    ap.add_argument("--out")
    ap.add_argument("--fullscreen-at", type=int)
    ap.add_argument("--windowed-at", type=int)
    ap.add_argument("--focus-at", type=int)
    ap.add_argument("--steps", type=int, default=20000)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--botseed", type=int, default=7)
    ap.add_argument("--real-time", action="store_true")
    ap.add_argument("--modsdir")
    ap.add_argument("--shift", type=int, default=0)
    ap.add_argument("--focus-shift", type=int)
    args = ap.parse_args()
    return tour(args) if args.what == "tour" else play(args)


if __name__ == "__main__":
    sys.exit(main())
