#!/usr/bin/env python3
"""Plays a native port by its own bot: the one driver loop every game shares (docs/TEST_COMMANDS.md, "The bot behind move").

    python tools/playbymove/playbymove.py [options] <game exe> [game arguments...]

It starts the game under build/native/rundesk.exe (directly when it already runs under rundesk) with the live channel,
-commands=<file>, an empty -realinput file (so the moves are real SDL events, docs/REAL_INPUT.md), -hold (the game's
updates wait for the driver's lines, so the bot's move is never stale and the same game plays the same way) and -fast
(not with --realtime). Then it loops as the web page's ?driver=move does (web/testkit.js, playByMove): it asks "state"
and stops when it says "over": true; it asks "move" and plays the MOVE's input steps in order, one update apart; then
waits the move's "updates" (1 by default). A whole move is one batch of lines (the steps with "wait 1" between them, the
move's wait, the next state and move), each answered OK in turn. MOVE null waits 30 updates and asks again; --nulls of
them running stop it, and so do --stuck moves running that are the same on the same state. It also stops on an ERROR
answer, a game that exits, an answer that does not come within --timeout seconds, or --steps moves. Then it takes
--shot, sends quit and waits for the game to end.

The steps of a MOVE's "input" (game pixels), and the -realinput line each becomes:
    {"move":[x,y]}                      move game x y
    {"click":[x,y]}, "button":"right"   click game x y [right]
    {"press":[x,y]}, {"release":[x,y]}  down game x y, up game x y [button]
    {"key":"B"}                         type B (a printable character), key Return (a DOM name: Enter, Escape, ArrowLeft,
                                        " ", Backspace, Tab...), or key <SDL name> as given
    {"wait":n}                          wait n (the channel's own: n updates)
The older lists "clicks" ([x,y] or {"x","y","button"}), "drags" ({"x0","y0","x1","y1","steps"}) and "keys" are played
in that order when a MOVE has no "input".

--first <line> sends a command before the loop (a game's goto, level, seed, set...), in order; an ERROR stops the run.
Everything goes into --dir (build/playbymove/<game> by default): the commands file, the game's log (game.log) and the
driver's own transcript (driver.txt: each line sent and its answer). The last line printed is the summary:
    playbymove <game>: <moves> moves, stop <over|limit|no move|error: ...|exited ...|timeout ...>; STATE {...}
The exit code is 0 when it stopped on "over" or the step limit, 1 otherwise.
"""

import argparse
import atexit
import json
import os
import re
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ANSWER_PREFIX = "INFO: "

DOM_KEYS = {
    "Enter": "Return", "Escape": "Escape", "Esc": "Escape", " ": "Space", "Spacebar": "Space",
    "ArrowLeft": "Left", "ArrowRight": "Right", "ArrowUp": "Up", "ArrowDown": "Down",
    "Backspace": "Backspace", "Tab": "Tab", "Delete": "Delete", "Insert": "Insert", "Home": "Home", "End": "End",
    "PageUp": "PageUp", "PageDown": "PageDown", "Shift": "Left Shift", "Control": "Left Ctrl", "Alt": "Left Alt",
}


class Stop(Exception):
    pass


class Game:
    def __init__(self, args):
        self.dir = os.path.abspath(args.dir)
        os.makedirs(self.dir, exist_ok=True)
        self.commands = os.path.join(self.dir, "commands.txt")
        self.realinput = os.path.join(self.dir, "realinput.txt")
        self.logname = os.path.join(self.dir, "game.log")
        for name in (self.commands, self.realinput):
            open(name, "wb").close()
        self.transcript = open(os.path.join(self.dir, "driver.txt"), "w", encoding="utf-8", newline="\n")
        self.timeout = args.timeout
        cmd = [os.path.abspath(args.exe)] + args.game_args + [
            "-commands=" + self.commands, "-realinput=" + self.realinput, "-hold"]
        if not args.realtime:
            cmd.append("-fast")
        if os.environ.get("BYOPOP_TEST_RUN") != "1":
            cmd = [os.path.join(ROOT, "build", "native", "rundesk.exe")] + cmd
        self.log = open(self.logname, "wb")
        self.proc = subprocess.Popen(cmd, stdout=self.log, stderr=subprocess.STDOUT)
        # a driver that ends on an exception outside its try never calls finish(); the game, held by -hold, would
        # keep its slot under rundesk for good
        atexit.register(self.stop)
        self.reader = open(self.logname, "rb")
        self.partial = b""

    def lines(self):
        data = self.reader.read()
        if not data:
            return []
        data = self.partial + data
        cut = data.rfind(b"\n")
        if cut < 0:
            self.partial = data
            return []
        self.partial = data[cut + 1:]
        return [l.rstrip(b"\r").decode("utf-8", "replace") for l in data[:cut].split(b"\n")]

    def send(self, *lines):
        """Appends the lines to the commands file at once and returns their answers, in order: STATE/MOVE for
        state/move, OK/ERROR for the rest (the answers come in the lines' order, as the game runs them)"""
        waiting = []
        for line in lines:
            word = line.split()[0]
            waiting.append((line, word, re.compile(r"^OK " + re.escape(word) + r"( |$)"),
                            re.compile(r"^ERROR " + re.escape(word) + r"( |:)")))
        with open(self.commands, "ab") as f:
            f.write(b"".join(line.encode("utf-8") + b"\n" for line in lines))
        for line in lines:
            self.transcript.write("> " + line + "\n")
        answers = []
        start = time.time()
        while waiting:
            for raw in self.lines():
                if not waiting:
                    break
                line, word, ok, err = waiting[0]
                text = raw[len(ANSWER_PREFIX):] if raw.startswith(ANSWER_PREFIX) else raw
                if ((word == "state" and text.startswith("STATE ")) or (word == "move" and text.startswith("MOVE "))
                        or ok.match(text) or err.match(text)):
                    self.transcript.write("< " + text + "\n")
                    if text.startswith("ERROR "):
                        self.transcript.flush()
                        raise Stop("error: " + text)
                    answers.append(text)
                    waiting.pop(0)
            if not waiting:
                break
            if self.proc.poll() is not None:
                raise Stop("exited (code %d) waiting for '%s'" % (self.proc.returncode, waiting[0][0]))
            if time.time() - start > self.timeout:
                raise Stop("timeout: no answer to '%s' in %d s" % (waiting[0][0], self.timeout))
            time.sleep(0.002)
        self.transcript.flush()
        return answers

    def stop(self):
        """Ends the run and every process under it (rundesk and the game) if it still runs"""
        if self.proc.poll() is None:
            subprocess.call(["taskkill", "/T", "/F", "/PID", str(self.proc.pid)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            try:
                self.proc.wait(10)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()

    def finish(self, shot):
        try:
            if shot and self.proc.poll() is None:
                self.send("screenshot " + os.path.abspath(shot))
        except Stop as e:
            self.transcript.write("! " + str(e) + "\n")
        if self.proc.poll() is None:
            with open(self.commands, "ab") as f:
                f.write(b"quit\n")
            try:
                self.proc.wait(30)
            except subprocess.TimeoutExpired:
                self.stop()
        self.transcript.close()
        self.log.close()
        self.reader.close()


def point(v):
    if isinstance(v, dict):
        return int(round(v["x"])), int(round(v["y"]))
    return int(round(v[0])), int(round(v[1]))


def button(step, v=None):
    b = (v.get("button") if isinstance(v, dict) else None) or step.get("button") or "left"
    return "" if b == "left" else " " + b


def key_line(k):
    if k in DOM_KEYS:
        return "key " + DOM_KEYS[k]
    if len(k) == 1 and 32 < ord(k) < 127:
        return "type " + k
    return "key " + k


def drag_lines(d):
    x0, y0, x1, y1 = (int(round(d[k])) for k in ("x0", "y0", "x1", "y1"))
    n = max(1, int(d.get("steps", 4)))
    out = ["down game %d %d" % (x0, y0)]
    for i in range(1, n + 1):
        out.append("move game %d %d" % (round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n)))
    out.append("up game %d %d" % (x1, y1))
    return out


def step_lines(step):
    for kind, verb in (("move", "move"), ("click", "click"), ("press", "down"), ("release", "up")):
        if kind in step:
            x, y = point(step[kind])
            return ["%s game %d %d%s" % (verb, x, y, "" if kind == "move" else button(step, step[kind]))]
    if "key" in step:
        return [key_line(str(step["key"]))]
    if "wait" in step:
        return ["wait %d" % int(step["wait"])]
    if "drag" in step:
        return drag_lines(step["drag"])
    raise Stop("error: a step the loop does not know: " + json.dumps(step))


def move_lines(m):
    if isinstance(m.get("input"), list):
        steps = m["input"]
    else:
        steps = ([{"click": c} for c in m.get("clicks", [])] + [{"drag": d} for d in m.get("drags", [])] +
                 [{"key": k} for k in m.get("keys", [])])
    out = []
    for s in steps:
        out += step_lines(s)
    return out


def parse_args(argv=None):
    ap = argparse.ArgumentParser(description="Plays a native port by its own bot's move answers.")
    ap.add_argument("--dir", help="the run's folder (default build/playbymove/<game>)")
    ap.add_argument("--steps", type=int, default=2000, help="stop after this many moves (2000)")
    ap.add_argument("--first", action="append", default=[], help="a command sent before the loop (repeatable)")
    ap.add_argument("--nulls", type=int, default=2, help="MOVE null this many times running stops it (2)")
    ap.add_argument("--stuck", type=int, default=50,
                    help="the same move on the same state this many times running stops it (50)")
    ap.add_argument("--timeout", type=float, default=60, help="real seconds to wait for one answer (60)")
    ap.add_argument("--shot", help="a screenshot (PNG) when it stops")
    ap.add_argument("--realtime", action="store_true", help="without -fast")
    ap.add_argument("exe")
    ap.add_argument("game_args", nargs=argparse.REMAINDER)
    args = ap.parse_args(argv)
    args.name = os.path.splitext(os.path.basename(args.exe))[0]
    if not args.dir:
        args.dir = os.path.join(ROOT, "build", "playbymove", args.name)
    return args


def state_is_over(state):
    return state.get("over") is True


def play(args, is_over=state_is_over):
    """The loop on parsed arguments. is_over(state) is the game's end of play, called on each state the game answers
    (a dict); a game whose "over" is also a screen the bot plays on from (a level's summary) passes its own, which may
    keep what it has seen. Returns the exit code."""
    name = args.name
    game = Game(args)
    moves, state, stop = 0, None, None
    try:
        for line in args.first:
            game.send(line)
        # One batch a move: its steps one update apart ("wait 1" between them, so that a game's update runs between a
        # hover and its click, or a press and the drag's moves, as between a hand's), the move's own wait, then the
        # next state and move. The bot reads the state the same update the driver does, so a MOVE always fits it.
        nulls, same, last = 0, 0, None
        batch = []
        while True:
            answers = game.send(*(batch + ["state", "move"]))
            state = answers[-2][len("STATE "):]
            if is_over(json.loads(state)):
                raise Stop("over")
            if moves >= args.steps:
                raise Stop("limit")
            m = json.loads(answers[-1][len("MOVE "):])
            if m is None:
                nulls += 1
                if nulls >= args.nulls:
                    raise Stop("no move")
                batch = ["wait 30"]
                continue
            nulls = 0
            lines = move_lines(m)
            # the same move on the same state, again and again, is a bot that does not see its move take (a move of
            # waits alone is a bot waiting for the game, and is not counted)
            if any(not l.startswith("wait ") for l in lines):
                same = same + 1 if (state, answers[-1]) == last else 0
                last = (state, answers[-1])
                if same >= args.stuck:
                    raise Stop("stuck: the same move %d times on the same state: %s" % (same + 1, answers[-1]))
            moves += 1
            batch = []
            for line in lines:
                if batch and not batch[-1].startswith("wait ") and not line.startswith("wait "):
                    batch.append("wait 1")
                batch.append(line)
            updates = m.get("updates", 1)
            batch.append("wait %d" % (updates if isinstance(updates, int) and updates > 0 else 1))
    except Stop as e:
        stop = str(e)
    except (ValueError, KeyError, TypeError) as e:
        stop = "error: an answer the loop cannot read (%s)" % e
    finally:
        game.finish(args.shot)
    print("playbymove %s: %d moves, stop %s; STATE %s" % (name, moves, stop, state))
    return 0 if stop in ("over", "limit") else 1


def main():
    return play(parse_args())


if __name__ == "__main__":
    sys.exit(main())
