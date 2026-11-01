"""loads.py <origrun-sandbox.log> [--start]: from origrun's --watch lines (run.sh), the updates each chapter load's
screen stayed up in the original, comma-separated: watch 1, the screen's mTimer (+0xb0), which each update decrements
or, at 0, resets to 50 with a hint draw; the screen's first value, before its first update, is 50.
With --start, the updates the startup load took instead: from StartLoadingThread (0x549480) setting
mLoadingThreadStarted (watch 2, app+0x4f9, the low byte) to LoadingThreadCompleted setting the app's +0x919 (watch 3)."""
import re
import sys

loads, cur, started, done = [], None, None, None
for line in open(sys.argv[1], errors="replace"):
    m = re.search(r"watch (\d): (\S+) -> (\S+)(?: at update (\d+))?", line)
    if not m:
        continue
    w, old, new, at = m.groups()
    if w in "23":
        if new.isdigit() and int(new) & 0xff and at is not None:
            if w == "2" and started is None:
                started = int(at)
            if w == "3" and done is None:
                done = int(at)
        continue
    if new == "-":
        if cur is not None:
            loads.append(cur)
        cur = None
    elif old in ("-", "?"):
        cur = 0 if new == "50" else 1
    elif cur is not None:
        cur += 1
if "--start" in sys.argv[2:]:
    print(done - started if started is not None and done is not None else "")
else:
    print(",".join(str(n) for n in loads))
