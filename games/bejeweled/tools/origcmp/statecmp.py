"""Compares the game's state in one scripted run of the original and of the port (the "plays identical" phase): the
original's score and level from origrun's --watch, the port's from its -trace, which prints the level and score each
time the board settles after a move.

    python games/bejeweled/tools/origcmp/statecmp.py <origrun run dir> <port out.txt>

The original's run watches, in this order (orig.sh passes them on to origrun; the app's mBoard is at 0x53bcf8):
    --watch [0x53bcf8]+0x1ec --watch [0x53bcf8]+0x8c --watch [0x53bcf8]+0x194 --watch [0x53bcf8]+0x1d8
that is the score, the 0-based level, the board's state and the bar's width. For each settle of the port at update U
the original's score and level after update U must be the same (the port's level is the displayed one, 1-based). The
lists of the original's states, and of its levels, are printed in order with their updates, so that the game's course
(level-ups, the game over) can be read beside the port's.
"""
import glob
import re
import sys


def timelines(run):
    lines = []
    for f in glob.glob(run + "/logs/*.log"):
        lines += open(f, encoding="utf-8", errors="replace").read().splitlines()
    tl = {}
    seen = set()
    for ln in lines:
        m = re.search(r"watch (\d): (\S+) -> (\S+) at update (\d+)", ln)
        if m and m.group(0) not in seen:   # the host's log repeats the sandbox's lines
            seen.add(m.group(0))
            tl.setdefault(int(m.group(1)), []).append((int(m.group(4)), m.group(3)))
    for k in tl:
        tl[k].sort()
    return tl


def at(tl, u):
    v = None
    for uu, val in tl:
        if uu > u:
            break
        v = val
    return v


def main():
    run, port = sys.argv[1], sys.argv[2]
    tl = timelines(run)
    settles = [(int(m.group(1)), int(m.group(2)), int(m.group(3)))
               for m in re.finditer(r"TRACE update (\d+): \w+ level (\d+) score (\d+)", open(port).read())]
    bad = 0
    for u, level, score in settles:
        os_, ol = at(tl.get(1, []), u), at(tl.get(2, []), u)
        if os_ != str(score) or ol is None or ol == "-" or int(ol) + 1 != level:
            bad += 1
            if bad <= 10:
                print("update %d: port level %d score %d, original level %s score %s" % (u, level, score, ol, os_))
    print("states:", " ".join("%s@%d" % (v, u) for u, v in tl.get(3, [])))
    print("levels:", " ".join("%s@%d" % (v, u) for u, v in tl.get(2, [])))
    print("final score: original %s, port %s" % (tl.get(1, [(0, None)])[-1][1], settles[-1][2] if settles else None))
    print("statecmp: %d settles, %d differ -> %s" % (len(settles), bad, "same" if settles and not bad else "DIFFERENT"))
    sys.exit(0 if settles and not bad else 1)


if __name__ == "__main__":
    main()
