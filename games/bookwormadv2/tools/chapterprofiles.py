#!/usr/bin/env python3
"""chapterprofiles.py: plays the adventure by the port's own bot and saves the player's folder at each chapter's map.

The comparison cases (identical/case.sh, PROFILE=<dir>) need a player standing at each chapter, with the position the game
itself saves. This plays from a profile (default: identical/profiles/bosschapter1, standing at the first chapter's boss) by the
bot of tools/playbymove (real -realinput moves; the `state` and `move` commands, docs/TEST_COMMANDS.md) and, whenever the map
comes up after a battle with the level it shows changed, copies the player's folder to <out>/<book>.<level>. It stops after
--steps moves or when the run cannot go on.

usage: chapterprofiles.py [--out DIR] [--steps N] [--userdir DIR]   (from the repo root)
"""
import argparse, json, os, shutil, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "playbymove"))
import playbymove as pbm  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "build", "chapters"))
    ap.add_argument("--steps", type=int, default=20000)
    ap.add_argument("--userdir", default=os.path.join(HERE, "identical", "profiles", "bosschapter1"))
    args = ap.parse_args()
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    run = os.path.join(out, "run")
    user = os.path.join(run, "user")
    if os.path.isdir(user):
        shutil.rmtree(user)
    shutil.copytree(args.userdir, user)
    ns = argparse.Namespace(dir=run, exe=os.path.join(ROOT, "build", "native", "bookwormadv2.exe"), timeout=300, realtime=False,
                            game_args=["-gamedir=" + os.path.join(ROOT, "assets", "bookwormadv2"), "-userdir=" + user, "-seed=1"])
    game = pbm.Game(ns)
    saved, moves, batch, last_log = set(), 0, [], 0
    try:
        game.send("wait 600")
        game.send("goto adventure")
        while moves < args.steps:
            answers = game.send(*(batch + ["state", "move"]))
            st = json.loads(answers[-2][len("STATE "):])
            if st.get("screen") == "map" and not st.get("curtain") and st.get("level") not in saved:
                game.send("wait 200")
                name = "after-" + str(st.get("level"))
                dest = os.path.join(out, name)
                if os.path.isdir(dest):
                    shutil.rmtree(dest)
                shutil.copytree(user, dest)
                saved.add(st.get("level"))
                print("saved", name, "after", moves, "moves", flush=True)
            if moves - last_log >= 500:
                last_log = moves
                print("moves", moves, "screen", st.get("screen"), "level", st.get("level"), flush=True)
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
        print("stop:", e)
    finally:
        game.finish(None)
    return 0


if __name__ == "__main__":
    sys.exit(main())
