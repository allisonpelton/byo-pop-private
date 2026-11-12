# hover.py <origrun script> [count]: puts "at N-300 move x y" before each of the first <count> (default: every) lines "at N click x y d",
# the mouse hovering over a menu button 300 updates before its click. The original loses a click that comes with its mouse move
# in one update on its menu buttons (the engine's hot flag is set from the position of an earlier update: Water Bugs'
# GAME_NOTES.md, "Coverage check"), and a real mouse always moves first. A line that is not an "at N click" is kept as it is.
import re, sys
path = sys.argv[1]
limit = int(sys.argv[2]) if len(sys.argv) > 2 else 10 ** 9
out, seen, last = [], 0, 0
for line in open(path, encoding="utf-8").read().splitlines():
    m = re.match(r"at (\d+) (\w+)(.*)", line)
    if m:
        t = int(m.group(1))
        c = re.match(r" (\d+) (\d+)", m.group(3))
        if m.group(2) in ("click", "rclick") and c and seen < limit:
            seen += 1
            out.append("at %d move %s %s" % (max(last, t - 300), c.group(1), c.group(2)))
        last = max(last, t)
    out.append(line)
open(path, "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
