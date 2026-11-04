"""Turns an origrun run of the original (orig.sh) into the port's -script, so the port gets the same input on the
same updates and takes its shots where the original's landed.

    python games/bigmoney/tools/origcmp/toport.py <run folder or its origrun log> [--shot-offset N] [--earlier K]
        > port_script.txt

origrun's log gives the update counter each held input was posted at ("at N: counter reached K -> click x y"). On the
virtual clock the original is held at its wait after update K and reads the input before update K+1; the port runs a
script line "K+1 <command>" before update K+1 (SexyAppBase::UpdateFrames counts the update, then runs the script), so
the input reaches both before the same update. Measured: a click at counter 1400 starts the board in update 1401 on
both sides.

Shots (a sandbox run): shots/landed.tsv gives the update each shot shows, L; the port takes it as
"L+<shot offset> screenshot <name>.png" (default 1, the port's frame after update L is the one on screen when the
script runs before update L+1). --earlier K also takes the K updates before it as <name>~1.png ... (for a shot origrun
marks "or-earlier", which may show an older frame). The script ends with a quit 2 updates after the run's last command.
"""
import os
import re
import sys


def main():
    a = sys.argv[1:]
    opt = {"--shot-offset": 1, "--earlier": 0}
    for k in list(opt):
        if k in a:
            i = a.index(k)
            opt[k] = int(a[i + 1])
            del a[i:i + 2]
    path = a[0]
    run = path
    if os.path.isdir(path):
        log = os.path.join(path, "logs", "origrun-sandbox.log")
        path = log if os.path.exists(log) else os.path.join(path, "logs", "origrun.log")
    else:
        run = os.path.dirname(os.path.dirname(path))
    landed = {}
    lt = os.path.join(run, "shots", "landed.tsv")
    if os.path.exists(lt):
        for ln in open(lt):
            p = ln.rstrip("\n").split("\t")
            if len(p) >= 2 and p[1].isdigit():
                landed[p[0]] = (int(p[1]), p[2] if len(p) > 2 else "")
    out = []  # (update, order, line)
    last = 0
    for ln in open(path, encoding="utf-8", errors="replace"):
        m = re.search(r"\] at \d+: counter reached (\d+) -> (.*)$", ln)
        if not m:
            continue
        k = int(m.group(1))
        last = max(last, k)
        cmd = m.group(2).split()
        if cmd[0] in ("click", "rclick", "move", "down", "up", "key", "focus"):
            if cmd[0] == "focus" and cmd[1] == "regain" and k <= 10:
                continue  # the start's focus: the port's -script run has the focus throughout
            out.append((k + 1, 1, " ".join(cmd)))
        elif cmd[0] == "text":
            out.extend((k + 1, 1, "char %s" % ("SPACE" if ch == " " else ch)) for ch in " ".join(cmd[1:]))
        elif cmd[0] == "shot" and cmd[1] in landed:
            u = landed[cmd[1]][0] + opt["--shot-offset"]
            out.append((u, 0, "screenshot %s.png" % cmd[1]))
            for e in range(1, opt["--earlier"] + 1):
                out.append((u - e, 0, "screenshot %s~%d.png" % (cmd[1], e)))
            last = max(last, u)
    out.sort(key=lambda t: (t[0], t[1]))
    for u, _, line in out:
        print("%d %s" % (u, line))
    print("%d quit" % (last + 2))


if __name__ == "__main__":
    main()
