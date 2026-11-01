"""loads.py <origrun-sandbox.log>: the updates each chapter load's screen stayed up in the original, comma-separated,
from origrun's --watch of the screen's mTimer (run.sh). Each update decrements it or, at 0, draws a hint and sets it
to 50; the screen's first value, before its first update, is 50."""
import re
import sys

loads, cur = [], None
for line in open(sys.argv[1], errors="replace"):
    m = re.search(r"watch 1: (\S+) -> (\S+)", line)
    if not m:
        continue
    old, new = m.groups()
    if new == "-":
        if cur is not None:
            loads.append(cur)
        cur = None
    elif old in ("-", "?"):
        cur = 0 if new == "50" else 1
    elif cur is not None:
        cur += 1
print(",".join(str(n) for n in loads))
