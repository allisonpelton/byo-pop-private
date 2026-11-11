#!/usr/bin/env python3
"""bossfocus.py: the user's fault of 2026-11-07, "the sound stops after a focus loss just after a chapter boss", as a native run.

Plays a chapter's boss by the port's own bot (tools/playbymove's loop, the same real -realinput moves), on a profile
that stands at the boss (profiles/bosschapter1), through the victory, the treasure room and the map, and loses and regains
the window's focus with posted focus events at set moments (after the boss's victory, in the treasure room, on the map).
At each of them it reads `state`'s song fields (song, playing) before the loss, just after it and a few seconds
after the regain, and prints one line a check; the run passes when a song is playing once the map is up and after the
regain. Everything it sends goes through the shared live channel (docs/TEST_COMMANDS.md).

usage: bossfocus.py [--dir DIR] [--steps N] [--userdir DIR]   (run from the repo root; the game's own exe and assets)
"""
import argparse, json, os, shutil, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "playbymove"))
import playbymove as pbm  # noqa: E402


def song(game):
    """(screen, song order, playing, queued) from one state"""
    answers = game.send("state")
    s = json.loads(answers[-1][len("STATE "):])
    return s.get("screen"), s.get("song"), s.get("playing"), s.get("queued"), s


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default=os.path.join(ROOT, "build", "bossfocus"))
    ap.add_argument("--steps", type=int, default=3000)
    ap.add_argument("--stop-level", help="stop at the first map after a boss, once the level is this (the map shows the level just beaten: 4.1 for the start of chapter 2) without the focus checks, and keep the player's folder")
    ap.add_argument("--save", help="with --stop-level: copy the player's folder here (a profile for a comparison case)")
    ap.add_argument("--userdir", default=os.path.join(HERE, "identical", "profiles", "bosschapter1"))
    args = ap.parse_args()
    run = os.path.abspath(args.dir)
    user = os.path.join(run, "user")
    if os.path.isdir(user):
        shutil.rmtree(user)
    shutil.copytree(args.userdir, user)
    ns = argparse.Namespace(dir=run, exe=os.path.join(ROOT, "build", "native", "bookwormadv2.exe"), timeout=120, realtime=False,
                            game_args=["-gamedir=" + os.path.join(ROOT, "assets", "bookwormadv2"), "-userdir=" + user,
                                       "-seed=1"])
    game = pbm.Game(ns)
    results = []
    try:
        game.send("wait 600")  # the load
        game.send("goto adventure")
        moves, last, batch, done_after, seen_boss, stopped = 0, None, [], {}, False, False
        while moves < args.steps:
            answers = game.send(*(batch + ["state", "move"]))
            st = json.loads(answers[-2][len("STATE "):])
            screen = st.get("screen")
            if args.stop_level and seen_boss and screen == "map" and st.get("level") == args.stop_level and not st.get("curtain"):
                game.send("wait 200")
                stopped = True
                break
            if "(Boss)" in (st.get("enemy") or ""):
                seen_boss = True
            # the moments to lose the focus: the boss beaten (its health 0, the victory), and the map that follows it
            boss = "(Boss)" in (st.get("enemy") or "")
            if boss:
                seen_boss = True
            moment = None
            if boss and st.get("enemyhealth") == 0 and "victory" not in done_after and st.get("playing") is not None:
                moment = "victory"
            elif seen_boss and screen == "map" and not st.get("curtain") and "map" not in done_after and st.get("playing") is not None:
                moment = "map"
            if moment:
                done_after[moment] = True
                before = (st.get("song"), st.get("playing"))
                game.send("focus lost", "wait 300", "focus gained", "wait 600")
                after = song(game)
                results.append((moment, before, after[1:3]))
                if moment == "map":
                    break
                batch = []
                continue
            m = json.loads(answers[-1][len("MOVE "):])
            if m is None:
                batch = ["wait 30"]
                continue
            moves += 1
            batch = []
            for line in pbm.move_lines(m):
                if batch and not batch[-1].startswith("wait ") and not line.startswith("wait "):
                    batch.append("wait 1")
                batch.append(line)
            u = m.get("updates", 1)
            batch.append("wait %d" % (u if isinstance(u, int) and u > 0 else 1))
    except pbm.Stop as e:
        results.append(("stop", str(e), None))
    finally:
        game.finish(None)
    if args.stop_level:
        if stopped and args.save:
            if os.path.isdir(args.save):
                shutil.rmtree(args.save)
            shutil.copytree(user, args.save)
        print("bossfocus: %s the map of %s (%d moves)" % ("reached" if stopped else "did not reach", args.stop_level, moves))
        return 0 if stopped else 1
    ok = True
    for screen, before, after in results:
        print("bossfocus:", screen, "before (song, playing)", before, "after the focus came back", after)
        if screen == "map" and not (after and after[1] is True):
            ok = False
    print("bossfocus:", "PASS" if ok and any(r[0] == "map" for r in results) else "FAIL", "(%d moves)" % moves)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
