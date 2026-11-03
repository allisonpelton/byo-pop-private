"""cmp.py <port state log> <origrun log>[,<origrun log>...]: compares the values that decide play, field by
field. The port's -statelog lines are "<event time> <name> <value>"; origrun's watch lines are named by the case's
fields file (<origrun log>.fields, one name per watch). Times are taken from the first deal on each side (and in each of
the original's runs, whose clocks start at different times), so both programs' clocks line up. For each
field it prints "same" or the first change that differs (index, time and value on each side)."""
import re
import sys

def port_log(path):
	seq = {}
	for line in open(path):
		f = line.split()
		if len(f) != 3:  # the board at a deal or shuffle (mouse.py's)
			continue
		t, name, v = f
		seq.setdefault(name, []).append((int(t), int(v)))
	return seq

WATCH = re.compile(r"watch (\d+): (\S+) -> (\S+) at update (\d+)")

def orig_log(path):
	seq = {}
	names = open(path + ".fields").read().split()
	for line in open(path):
		m = WATCH.search(line)
		if m:
			k, v, t = int(m.group(1)), int(m.group(3)), int(m.group(4))
			v = v - (1 << 32) if v >= 1 << 31 else v
			seq.setdefault(names[k - 1], []).append((t, v))
	return seq

def rel(seq):
	# times from the first deal: the first time the generator's index (rngindex, which every run of the original
	# watches) leaves 0 (not seeded yet) and 624 (seeded, nothing drawn); only the changes from there on are compared,
	# since before it the original's watches begin at other moments than the port's first read of its events
	t0 = next(t for t, v in seq["rngindex"] if v not in (0, 624))
	return {n: [(t - t0, v) for t, v in s if t >= t0] for n, s in seq.items()}

def first_diff(p, o):
	# a change at the same value less than one of the port's updates (10 ms) away counts as the same: after an input
	# the original's next tick comes 1 ms later, the port's in the same update (IDENTICAL_PLAY.md, the deviations)
	near = 0
	for i in range(max(len(p), len(o))):
		a = p[i] if i < len(p) else None
		b = o[i] if i < len(o) else None
		if a != b:
			if a and b and a[1] == b[1] and abs(a[0] - b[0]) < 10:
				near += 1
				continue
			return (i, a, b), near
	return None, near

port = rel(port_log(sys.argv[1]))
orig = {}
bad = 0
# every run of the original watches rngindex: a run whose generator went another way than the port's played another
# game, and its other fields say nothing (case.sh runs it again)
for k, path in enumerate(sys.argv[2].split(",")):
	seq = rel(orig_log(path))
	d, near = first_diff(port.get("rngindex", []), seq["rngindex"])
	if d is not None:
		print("run %d (%s): rngindex DIFFERS at change %d: port %s, original %s" % ((k, path) + d))
		bad += 1
	orig.update(seq)
for name in orig:
	p, o = port.get(name, []), orig[name]
	diff, near = first_diff(p, o)
	if diff is None:
		print("%-11s same (%d changes%s)" % (name, len(o), ", %d of them within one update" % near if near else ""))
	else:
		bad += 1
		i, a, b = diff
		print("%-11s DIFFERS at change %d: port %s, original %s" % (name, i, a, b))
sys.exit(1 if bad else 0)
