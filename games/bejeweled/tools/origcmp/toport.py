"""Turns an origrun run of the original into the port's -script, so the port gets the same input on the same updates
and takes its shots on the updates the original's landed on.

    python games/bejeweled/tools/origcmp/toport.py build/origrun/<run> > port_script.txt [--input-offset N]

Input: origrun's log inside the sandbox (logs/origrun-sandbox.log) gives, for each held input command, the update
counter at which it was posted ("at click: input at counter K"); the original reads it in its next message pump,
before update K+1, which is the port's "K <command>" (run when mUpdateCount reaches K, before the next update).
A shot on the update of an input is left out (see below).
--input-offset shifts every input (for calibration). Shots: shots/landed.tsv gives each shot's update; for a shot it
marks "or-earlier" (a frame that may be older) the port also takes the three updates before it, as <name>~1..3.png,
which earlier.py checks against a pair that differs.
"""
import re
import sys


def main():
    args = sys.argv[1:]
    off = 0
    if "--input-offset" in args:
        i = args.index("--input-offset")
        off = int(args[i + 1])
        del args[i:i + 2]
    run = args[0]
    landed = {}
    unsure = set()
    with open(run + "/shots/landed.tsv") as f:
        for ln in f:
            p = ln.rstrip("\n").split("\t")
            if len(p) >= 2 and p[1].isdigit():
                landed[p[0]] = int(p[1])
                if len(p) >= 3 and p[2] != "drawn":
                    unsure.add(p[0])
    out = []  # (update, shots before input on one update, order, line)
    pending = None
    with open(run + "/logs/origrun-sandbox.log", encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = re.search(r"\] at \d+: counter reached (\d+) -> (.*)$", ln)
            if m:
                cmd = m.group(2).strip()
                word = cmd.split()[0]
                if word == "shot":
                    name = cmd.split()[1]
                    if name in landed:
                        out.append((landed[name], 0, len(out), "screenshot %s.png" % name))
                        # an "or-earlier" shot may show an older frame: the port also shoots the updates before it
                        # (name~k.png, k updates earlier), for earlier.py
                        if name in unsure:
                            for k in range(1, 4):
                                out.append((landed[name] - k, 0, len(out), "screenshot %s~%d.png" % (name, k)))
                    pending = None
                else:
                    pending = (int(m.group(1)), cmd)
                continue
            m = re.search(r"\] at (\w+): input at counter (\d+)", ln)
            if m and pending:
                k = int(m.group(2)) + off
                cmd = pending[1]
                word = cmd.split()[0]
                if word in ("click", "rclick", "move", "down", "up", "key", "focus"):
                    out.append((k, 1, len(out), cmd))
                elif word == "text":
                    for ch in cmd[5:]:
                        out.append((k, 1, len(out), "char %s" % ("SPACE" if ch == " " else ch)))
                pending = None
    # A shot on the update of an input is ambiguous: the original answers the shot's PrintWindow in the message pump
    # that reads the input, and shows the input's effect only when that forces a draw. Such shots are left out.
    inputs = set(u for u, k, _, _ in out if k == 1)
    out = [t for t in out if t[1] == 1 or t[0] not in inputs]
    out.sort()
    for u, _, _, line in out:
        print("%d %s" % (u, line))
    last = out[-1][0] if out else 0
    print("%d quit" % (last + 2))


if __name__ == "__main__":
    main()
