"""gen.py <steps.txt> <orig_script_out> <port_script_out> <port_shot_dir>

One list of steps for a screen-by-screen comparison, made into an origrun script for the original and a -script for
the port (docs/ORIGRUN.md, docs/SHOTDIFF.md). Both run 100 updates a second, so a step at t ms is update t/10 on
both sides: the port's -script counts updates, and the original's script times each command to its own update counter
(origrun's `at`, reading mUpdateCount; run.sh gives the address), clips included (`burst ... update k`).

Steps, one per line ('#' starts a comment):
  title <dur_ms> <interval_ms>   a clip from the start (first line only): the logo and title
  at <ms>                        go on at this time (both games settled on a screen)
  wait <ms>
  click <x> <y>                  a move, then a click 50 ms later (both games see the hover first)
  text <chars>                   typed characters
  key <vk> <name>                a key: the original's virtual-key code, the port's key name
  focus lose|regain
  shot <name>                    a still
  clip <name> <dur_ms> <interval_ms>   numbered frames (origrun's record; the port's screenshots)
  quit
"""
import os
import sys


def main():
    steps, orig_out, port_out, shots = sys.argv[1:5]
    orig, port = ["waitshot _window 60000"], []
    t = 0       # ms of game time: update t/10

    def oat(ms, cmd):
        orig.append("at %d %s" % (max(ms // 10, 1), cmd))

    def pat(ms, cmd):
        port.append("%d %s" % (max(ms // 10, 1), cmd))

    def frames(name, dur, interval):
        nonlocal t
        oat(t, "burst %s %d update %d" % (name, dur // interval, interval // 10))
        os.makedirs(os.path.join(shots, name), exist_ok=True)
        for i in range(dur // interval):
            pat(t + i * interval, "screenshot %s/%s/%04d.png" % (shots, name, i))
        t += dur

    for line in open(steps):
        line = line.split("#", 1)[0].split()
        if not line:
            continue
        op, args = line[0], line[1:]
        if op == "title":
            frames("title", int(args[0]), int(args[1]))
        elif op == "at":
            t = max(t, int(args[0]))
        elif op == "wait":
            t += int(args[0])
        elif op == "click":
            x, y = args
            oat(t, "move %s %s" % (x, y)); pat(t, "move %s %s" % (x, y))
            t += 50
            oat(t, "click %s %s" % (x, y)); pat(t, "click %s %s" % (x, y))
        elif op == "text":
            for c in args[0]:
                oat(t, "text %s" % c); pat(t, "char %s" % c)
                t += 100
        elif op == "key":
            oat(t, "key %s" % args[0]); pat(t, "key %s" % args[1])
        elif op == "focus":
            oat(t, "focus %s" % args[0]); pat(t, "focus %s" % args[0])
        elif op == "shot":
            oat(t, "shot %s" % args[0]); pat(t, "screenshot %s/%s.png" % (shots, args[0]))
        elif op == "clip":
            frames(args[0], int(args[1]), int(args[2]))
        elif op == "quit":
            t += 500
            oat(t, "move 0 0"); orig.append("quit"); pat(t, "quit")
        else:
            sys.exit("unknown step: " + op)
    open(orig_out, "w", newline="\n").write("\n".join(orig) + "\n")
    open(port_out, "w", newline="\n").write("\n".join(port) + "\n")
    print("%d steps, %.1f s" % (len(port), t / 1000.0))


main()
