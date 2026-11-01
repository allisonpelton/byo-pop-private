"""Makes a long game's input for the "plays identical" phase: the port plays it move by move, each time the first move
its -trace lists for the settled board, as two clicks 60 and 100 updates after the settle, until the game is over, it
reaches a level or the move limit. Each move is one more short run of the port (port.sh, so under rundesk).

    python games/bejeweled/tools/origcmp/greedy.py <out script> [--seed N] [--timed] [--level L] [--moves M]

The script it writes is the original's (origrun's "at U click x y" lines with a shot every 50 updates); the port's
script then comes from the original's run by toport.py as for every other run, and statecmp.py compares the two.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))


def main():
    a = sys.argv[1:]
    out = a[0]
    seed = int(a[a.index("--seed") + 1]) if "--seed" in a else 1
    level = int(a[a.index("--level") + 1]) if "--level" in a else 3
    limit = int(a[a.index("--moves") + 1]) if "--moves" in a else 300
    timed = "--timed" in a
    inputs = [(800, "click 320 398")]
    if timed:   # Timed, then Yes in the question
        inputs += [(900, "click 135 200"), (950, "click 339 169")]
    script = os.path.join(ROOT, "build", "bejcmp", "scripts", "greedy-%d.port.txt" % os.getpid())
    last = 0
    tries = 0   # a refused move (the first board's instructions take only the hinted one) is replaced by the next
    for n in range(limit):
        end = inputs[-1][0] + 1500
        with open(script, "w") as f:
            for u, c in inputs:
                f.write("%d %s\n" % (u, c))
            f.write("%d quit\n" % end)
        subprocess.run(["bash", os.path.join(HERE, "port.sh"), "greedy-%d" % os.getpid(), script, str(seed), "-trace"], check=False)
        text = open(os.path.join(ROOT, "build", "bejcmp", "greedy-%d" % os.getpid(), "out.txt")).read()
        settles = [m for m in re.finditer(r"TRACE update (\d+): \w+ level (\d+) score (\d+)", text)
                   if int(m.group(1)) > inputs[-1][0]]
        if not settles and tries < 8 and last:
            tries += 1
            inputs = inputs[:-2]
            text, m = prev
            mv = re.findall(r"TRACE   move \d+: \S+ click (\d+) (\d+) then (\d+) (\d+)", text[m.end():].split("TRACE update")[0])
            if tries >= len(mv):
                break
            inputs += [(last + 60, "click %s %s" % mv[tries][:2]), (last + 100, "click %s %s" % mv[tries][2:])]
            continue
        if not settles:
            print("no settle after update %d: the game is over (or stuck)" % inputs[-1][0])
            break
        tries = 0
        m = settles[0]
        prev = (text, m)
        u, lv, score = int(m.group(1)), int(m.group(2)), int(m.group(3))
        mv = re.search(r"TRACE   move 0: \S+ click (\d+) (\d+) then (\d+) (\d+)", text[m.end():])
        print("move %d: settle at %d, level %d, score %d" % (n, u, lv, score), flush=True)
        if lv >= level or not mv:
            break
        inputs += [(u + 60, "click %s %s" % mv.group(1, 2)), (u + 100, "click %s %s" % mv.group(3, 4))]
        last = u
    with open(out, "w") as f:
        events = [(u, "click", c) for u, c in inputs]
        end = inputs[-1][0] + 1500
        events += [(u, "shot", "shot s%06d" % u) for u in range(850, end, 50)]
        for u, kind, c in sorted(events, key=lambda e: (e[0], e[1] == "click")):
            f.write("at %d %s\n" % (u, c))
        f.write("quit\n")
    print("greedy: %d inputs to update %d -> %s" % (len(inputs), end, out))


if __name__ == "__main__":
    main()
