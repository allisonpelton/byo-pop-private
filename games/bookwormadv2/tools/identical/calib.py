# calib.py <origrun log> <port statelog>: the load lengths of an original run, as port options. The original's loads take
# as long as the machine takes (the first run's 836 updates were 742 in another), so each case measures them and the port
# replays the same input with them (-startloadupdates, -chapterloadupdates): the startup load ends so that the title's
# first draws come at the update the original had them, and each chapter load ends at the update the original's battle
# engine was made. The port's own state log of the first run says where its loads begin.
import re, sys
orig, port = sys.argv[1], sys.argv[2]
first = None; jumps = []; letters = []; last = None; made = set()
for l in open(orig, errors='replace'):
    m = re.search(r'watch 2: \S+ -> 4294967295 at update (\d+)', l)
    if m: made.add(int(m.group(1)))
for l in open(orig, errors='replace'):
    m = re.search(r'watch 1: (\S+) -> (\S+) at update (\d+)', l)
    if not m or not m.group(2).isdigit(): continue
    v, u = int(m.group(2)), int(m.group(3))
    if first is None and m.group(1) == '624' and v < 624:
        first = u
    if last is not None and (v - last) % 624 == 1: letters.append(u)
    # a load ends with a burst of at least 10 draws (a battle's board, a mini-game's tiles); a reseed is no burst
    if last is not None and not (v == 624 and u in made) and (v - last) % 624 >= 10:
        jumps.append(u)
    last = v
loaded = rand32 = None; starts = []
for l in open(port):
    a = l.split()
    if len(a) == 3 and a[1] == 'loaded' and a[2] == '1' and loaded is None: loaded = int(a[0])
    if len(a) == 3 and a[1] == 'title' and a[2] == '0' and rand32 is None: rand32 = int(a[0])
    if len(a) == 3 and a[1] == 'chload' and a[2] == '1': starts.append(int(a[0]))
s = first - (rand32 - loaded) if first is not None and rand32 is not None and first < 3000 else -1
c = []
for b in starts:
    e = next((u for u in jumps if u >= b), None)
    if e is None: c.append('1'); continue
    # the loading screen draws a letter every 50 updates while the loader works; when the screen stays on well after the last one
    # (a mini-game's), the loader ended at that letter and the port's own steps after it make up the rest
    ls = [u for u in letters if b <= u < e]
    c.append(str((ls[-1] if ls and e - ls[-1] > 60 else e) - b))
print("-startloadupdates=%d%s" % (s, " -chapterloadupdates=" + ",".join(c) if c else ""))
