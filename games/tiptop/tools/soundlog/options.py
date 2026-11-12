"""options.py: the options screen's sliders by real input, for the sound log (run.sh <name> options).

From the title through the bot's own moves to the main menu, then Options, the sound volume slider pressed, dragged and
released (the game plays VolumeAdjust when it is let go), the music slider the same, and Main menu. One game process.
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import realplay  # noqa: E402


def click(x, y, updates=40):
    return {"input": [{"move": [x, y]}, {"click": [x, y]}], "updates": updates}


def drag(x0, x1, y):
    steps = [{"press": [x0, y]}, {"wait": 10}]
    for x in range(x0, x1, 10 if x1 > x0 else -10):
        steps += [{"move": [x, y]}, {"wait": 3}]
    return {"input": steps + [{"release": [x1, y]}], "updates": 30}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--seed", type=int, default=1)
    args = ap.parse_args()
    g = realplay.Game(Path(args.out) / "options", seed=args.seed, fast=True)
    try:
        while g.state()["screen"] != "menu":
            g.play(g.botmove())
        g.play(click(265, 157))                 # Options
        g.play(drag(175, 255, 190))             # the sound volume slider (x 163..273)
        g.play(drag(255, 185, 190))
        g.play(drag(175, 255, 214))             # the music volume slider
        g.play(click(550, 442))                 # Main menu
        print("RESULT options: screen %s" % g.state()["screen"])
    finally:
        g.close()


if __name__ == "__main__":
    main()
