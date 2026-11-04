"""Turns the port's bot script (-autoplay ... -autoplaylog=<file>) into an origrun script for the original (orig.sh).

    python games/bigmoney/tools/origcmp/fromport.py <bot.txt> [--title-click U] [--end U] [--shots N] > orig_script.txt

The original's title link is ready later than the port's (its loading thread takes real time on the virtual clock), so
the bot's first input (the title click) moves to update U (default 1000) and every later input by the same amount. On
origrun's virtual clock an input lands on the update it names, so nothing else moves. The name entry's keys become
origrun's key codes (whose WM_KEYDOWN the game's TranslateMessage turns into the character) and its characters text. The run ends at --end (default:
600 updates after the last input). The port is then given the original's real input updates by toport.py.

--shots N (for a sandbox run, tour.sh) adds a shot every N updates from the first input (g<update>), and around each
level's end and the game's end the bot logged: the stats screen the update before the bot's next input
(level<k>_stats, over) and 100 updates after it (level<k>_next, over_after).
"""
import sys

VK = {"BACK": 8, "TAB": 9, "RETURN": 13, "ESCAPE": 27}


def main():
    a = sys.argv[1:]
    opt = {"--title-click": 1000, "--end": 0, "--shots": 0}
    for k in list(opt):
        if k in a:
            i = a.index(k)
            opt[k] = int(a[i + 1])
            del a[i:i + 2]
    cmds = []
    events = []  # (index of the next command, shot name)
    for ln in open(a[0]):
        p = ln.split()
        if len(p) > 3 and p[0] == "#" and p[1] == "level" and p[3] == "done,":
            events.append((len(cmds), "level%s" % p[2]))
        elif len(p) > 3 and p[0] == "#" and p[1] == "game" and p[3] == "over,":
            events.append((len(cmds), "over"))
        if not p or p[0].startswith("#") or p[1] == "quit":
            continue
        if p[1] == "key" and p[2] in VK:  # origrun posts the key; the game's TranslateMessage types its character
            cmds.append((int(p[0]), "key", [str(VK[p[2]])]))
        elif p[1] == "char":
            cmds.append((int(p[0]), "text", [" " if p[2] == "SPACE" else p[2][0]]))
        elif p[1] in ("move", "click", "rclick", "down", "up"):
            cmds.append((int(p[0]), p[1], p[2:]))
        else:
            sys.exit("fromport: no origrun command for %r" % ln.strip())
    shift = opt["--title-click"] - cmds[0][0]
    end = max(opt["--end"], cmds[-1][0] + shift + 600)
    shots = []
    if opt["--shots"]:
        for k, name in events:
            if k < len(cmds):
                u = cmds[k][0] + shift
                shots += [(u - 1, name + "_stats" if name != "over" else "over"),
                          (u + 100, name + "_next" if name != "over" else "over_after")]
        shots += [(u, "g%06d" % u) for u in range(cmds[0][0] + shift, end, opt["--shots"])]
        shots.sort()
    print("at 10 focus regain")
    i = 0
    while i < len(cmds):
        u, c, v = cmds[i]
        u += shift
        while shots and shots[0][0] <= u:
            print("at %d shot %s" % shots.pop(0))
        if c == "text":  # the characters of one update in one text line
            t = v[0]
            while i + 1 < len(cmds) and cmds[i + 1][0] == cmds[i][0] and cmds[i + 1][1] == "text":
                i += 1
                t += cmds[i][2][0]
            print("at %d text %s" % (u, t))
        else:
            print("at %d %s %s" % (u, c, " ".join(v)))
        i += 1
    for s in shots:
        print("at %d shot %s" % s)
    print("at %d move 1 1" % end)
    print("quit")


if __name__ == "__main__":
    main()
