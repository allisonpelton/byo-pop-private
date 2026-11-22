"""gen.py <steps.txt> <orig_script_out> <port_script_out> <port_shot_dir> [--nominal]

One list of steps for a screen-by-screen comparison, made into an origrun script for the original and a -script for
the port (docs/ORIGRUN.md, docs/SHOTDIFF.md). Both run 100 updates a second, so a step at t ms is update t/10 on
both sides: the port's -script counts updates, and the original's script times each command to its own update counter
(origrun's `at`, reading mUpdateCount; run.sh gives the address), clips included (`burst ... update k`).

Steps, one per line ('#' starts a comment):
  title <dur_ms> <interval_ms>   a clip from the start (first line only): the logo and title
  at <ms>                        go on at this time (both games settled on a screen)
  wait <ms>
  port <command>                 a command for the port alone (state), when a list is checked on the port
  orig <command>                 a command for the original alone (origrun's `peek <expr>...`, its state)
  click <x> <y>                  a move, then a click 50 ms later (both games see the hover first)
  move <x> <y>                   the mouse alone (a hover, a tooltip)
  text <chars>                   typed characters
  key <vk> <name>                a key: the original's virtual-key code, the port's key name
  focus lose|regain
  shot <name>                    a still
  clip <name> <dur_ms> <interval_ms>   numbered frames (origrun's record; the port's screenshots)
  loaded <ms>                    a chapter load (ChapterLoadScreen, app+0x83c) under way or about to start: the
                                 original waits until its screen has come and gone and the curtain over it has
                                 opened (app+0x8ec, mTransitionState, back to 0) and marks the counter, and the steps
                                 after it count from there (`at +N`) as if the curtain opened <ms> after this line,
                                 so the load's length on the machine drops out (the curtain steps on its own count
                                 of 5 from before the load); the port's lines say @<k>+N
                                 for the k-th mark, which run.sh fills in from the original's log (anchors.py), the
                                 port's loads lasting as long as the original's (-chapterloadupdates)
  loaded <ms> gone               the same for a load that may already be over at this line (it waits only for the
                                 screen to be gone and the curtain open)
  loaded <ms> load               the same, counting from the load's end instead (the screen gone): for the steps
                                 that shoot the curtain as it opens, the steps after it waiting on the curtain's
                                 count of 5 (up to 4 updates)
  quit
"""
import os
import sys

LOADSCREEN = "[0x8259e4]+0x83c"   # the app's ChapterLoadScreen, 0 while none is up
TRANSITION = "[0x8259e4]+0x8ec"   # the app's mTransitionState, 0 once the curtain is done


def main():
    # --nominal: a port run with no original (tools/coverage/port.sh): each mark taken as the load's end the step names
    nominal = '--nominal' in sys.argv
    steps, orig_out, port_out, shots = [a for a in sys.argv[1:] if a != '--nominal'][:4]
    orig, port = ["waitshot _window 60000"], []
    t = 0       # ms of game time: update t/10
    k, base = 0, 0      # the marks so far; the game time the last one stands for
    kind = ""           # the last mark's: "g" its load may have been over at its line (the port takes the original's
                        # mark), "l" it counts from the load's end, not the curtain's opening

    def rel(ms):
        # after a mark: updates from it, at least 20 (origrun lets the game run free while it waits for the load)
        return max((ms - base) // 10, 20)

    def oat(ms, cmd):
        if k:
            orig.append("at +%d %s" % (rel(ms), cmd))
        else:
            orig.append("at %d %s" % (max(ms // 10, 1), cmd))

    def pat(ms, cmd):
        # one update later than the original's: the port runs a script line "N ..." at the start of update N, before
        # the widgets update (a screenshot shows update N-1, a click comes before update N), while origrun's `at N`
        # holds the original after update N (its shot shows update N, its click is handled before update N+1)
        if k and nominal:
            port.append("%d %s" % (base // 10 + rel(ms) + 1, cmd))
        elif k:
            port.append("@%d%s+%d %s" % (k, kind, rel(ms) + 1, cmd))
        else:
            port.append("%d %s" % (max(ms // 10, 1) + 1, cmd))

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
        elif op == "move":
            oat(t, "move %s %s" % tuple(args)); pat(t, "move %s %s" % tuple(args))
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
        elif op == "orig":
            oat(t, " ".join(args))   # a command for the original alone (an origrun peek, to read its state)
        elif op == "port":
            pat(t, " ".join(args))   # a command for the port alone (state, to check a step list on the port)
        elif op == "loaded":
            if args[1:] != ["gone"]:
                orig.append("waitfor %s != 0 60000" % LOADSCREEN)
            orig.append("waitfor %s == 0 120000" % LOADSCREEN)
            if args[1:] != ["load"]:
                orig.append("waitfor %s == 0 60000" % TRANSITION)
            orig.append("mark")
            k, base, kind = k + 1, t + int(args[0]), {"gone": "g", "load": "l"}.get((args[1:] or [""])[0], "")
        elif op == "quit":
            t += 500
            oat(t, "move 0 0"); orig.append("quit"); pat(t, "quit")
        else:
            sys.exit("unknown step: " + op)
    open(orig_out, "w", newline="\n").write("\n".join(orig) + "\n")
    open(port_out, "w", newline="\n").write("\n".join(port) + "\n")
    print("%d steps, %.1f s" % (len(port), t / 1000.0))


main()
