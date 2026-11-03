"""Turns an origrun run of the original (orig.sh) into the port's -script, so the port gets the same input on the
same updates.

    python games/bigmoney/tools/origcmp/toport.py build/origrun/<run>/logs/origrun.log > port_script.txt

origrun's log gives the update counter each held input was posted at ("at N: counter reached K -> click x y"). On the
virtual clock the original is held at its wait after update K and reads the input before update K+1; the port runs a
script line "K+1 <command>" before update K+1 (SexyAppBase::UpdateFrames counts the update, then runs the script), so
the input reaches both before the same update. Measured: a click at counter 1400 starts the board in update 1401 on
both sides. The script ends with a quit 2 updates after the run's last held command.
"""
import re
import sys


def main():
    out = []
    last = 0
    for ln in open(sys.argv[1], encoding="utf-8", errors="replace"):
        m = re.search(r"\] at \d+: counter reached (\d+) -> (.*)$", ln)
        if not m:
            continue
        k = int(m.group(1))
        last = max(last, k)
        cmd = m.group(2).split()
        if cmd[0] in ("click", "rclick", "move", "down", "up", "key"):
            out.append("%d %s" % (k + 1, " ".join(cmd)))
        elif cmd[0] == "text":
            out.extend("%d char %s" % (k + 1, "SPACE" if ch == " " else ch) for ch in " ".join(cmd[1:]))
        # focus regain: the port's -script run has the focus throughout
    out.append("%d quit" % (last + 2))
    print("\n".join(out))


if __name__ == "__main__":
    main()
