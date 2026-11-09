#!/usr/bin/env python3
"""The native half of the page check's focus line (tools/web/pagecheck.js; docs/WEB_PLAYER.md, "The page check").

    python tools/web/pagecheck_native.py [--first <command>]... [--title-move] [--settle 100] [--updates 200]
        <game exe> [game arguments]

It starts a native port as tools/playbymove does (under rundesk, -commands, an empty -realinput file, -hold, -fast). Once
the first screen is up (a state whose screen is not "loading") it sends the --first commands, the game's table's "start";
when one answers ERROR and --title-move is given (the table's titleMove), it plays the bot's move (a title's link), waits
30 updates and sends them again. Then it waits --settle updates, asks "state", loses the focus as the window would ("focus
lose", a real SDL focus event), waits --updates, asks "state", regains it, waits again and asks a third time. It prints the state keys that changed while the focus was away and after it came back, as the line the
game's table takes for its "focus" field:

    focus: {"hidden": [...], "after": [...]}

and writes the three states to <dir>/states.json (build/pagecheck/<game> by default).
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools", "playbymove"))
import playbymove  # noqa: E402  (its Game: the run under rundesk and the live channel)


def changed(a, b):
    keys = sorted(set(a) | set(b))
    return [k for k in keys if json.dumps(a.get(k), sort_keys=True) != json.dumps(b.get(k), sort_keys=True)]


def main():
    ap = argparse.ArgumentParser(description="The native build's answer to losing and regaining the focus mid-play.")
    ap.add_argument("--dir", help="the run's folder (default build/pagecheck/<game>)")
    ap.add_argument("--first", action="append", default=[], help="a command that starts the game (repeatable)")
    ap.add_argument("--title-move", action="store_true", help="when a --first command fails, play the bot's move and retry")
    ap.add_argument("--settle", type=int, default=100, help="updates to wait once started (100)")
    ap.add_argument("--updates", type=int, default=200, help="updates with the focus away, and again after (200)")
    ap.add_argument("--timeout", type=float, default=120, help="real seconds to wait for one answer (120)")
    ap.add_argument("exe")
    ap.add_argument("game_args", nargs=argparse.REMAINDER)
    args = ap.parse_args()
    game = os.path.splitext(os.path.basename(args.exe))[0]
    args.dir = args.dir or os.path.join(ROOT, "build", "pagecheck", game)
    args.realtime = False
    g = playbymove.Game(args)
    try:
        # the first screen, as the page's default "ready": a state whose screen is not "loading"
        for _ in range(2000):
            s = json.loads(g.send("state")[0][len("STATE "):])
            if s.get("screen") and s.get("screen") != "loading":
                break
            g.send("wait 10")
        for tries in range(21):
            failed = None
            for c in args.first:
                try:
                    g.send(c)
                except playbymove.Stop as e:
                    failed = str(e)
                    break
            if not failed:
                break
            if not args.title_move or tries == 20:
                raise playbymove.Stop(failed)
            m = json.loads(g.send("move")[0][len("MOVE "):])
            if m:
                batch = []
                for line in playbymove.move_lines(m):
                    if batch and not batch[-1].startswith("wait ") and not line.startswith("wait "):
                        batch.append("wait 1")
                    batch.append(line)
                g.send(*batch)
            g.send("wait 30")
        g.send("wait %d" % args.settle)
        n = args.updates
        answers = g.send("state", "focus lose", "wait %d" % n, "state", "focus regain", "wait %d" % n, "state")
    except playbymove.Stop as e:
        g.finish(None)
        print("pagecheck_native %s: stopped: %s" % (game, e))
        return 1
    g.finish(None)
    states = [json.loads(a[len("STATE "):]) for a in answers if a.startswith("STATE ")]
    with open(os.path.join(args.dir, "states.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump(states, f, indent=1)
    print("focus: " + json.dumps({"hidden": changed(states[0], states[1]), "after": changed(states[1], states[2])}))
    return 0


if __name__ == "__main__":
    sys.exit(main())
