"""For each shot origrun marks "or-earlier", keeps the port's frame that matches the original's.

    python games/bigmoney/tools/origcmp/pickshot.py <original's shots folder> <port's shots folder>

An "or-earlier" shot shows the update it landed on or an older one: the original had not drawn that update when the
shot was taken (docs/ORIGRUN.md, "Which frame the shot shows"). toport.py --earlier K has the port take the K updates
before it too, as <name>~1.png ... <name>~K.png. This keeps the one closest to the original's picture (the least mean
difference over the client area, the landed update on a tie) as <name>.png, deletes the others and prints which update
each shot shows, so shotdiff compares like with like. A shot the original took on its update (drawn) is left alone.
"""
import os
import re
import sys

import numpy as np
from PIL import Image

CLIENT = (640, 480)
FRAME_LEFT, FRAME_TOP = 3, 26  # origrun's window captures include the frame (tools/shotdiff/shotdiff.py)


def client(path):
	im = Image.open(path).convert("RGB")
	if im.size != CLIENT:
		im = im.crop((FRAME_LEFT, FRAME_TOP, FRAME_LEFT + CLIENT[0], FRAME_TOP + CLIENT[1]))
	return np.asarray(im).astype(np.int16)


def main():
	orig_dir, port_dir = sys.argv[1], sys.argv[2]
	landed = {}
	lt = os.path.join(orig_dir, "landed.tsv")
	if os.path.exists(lt):
		for ln in open(lt):
			p = ln.rstrip("\n").split("\t")
			if len(p) >= 2 and p[1].isdigit():
				landed[p[0]] = int(p[1])
	earlier = {}
	for f in os.listdir(port_dir):
		m = re.match(r"(.+)~(\d+)\.png$", f)
		if m:
			earlier.setdefault(m.group(1), []).append(int(m.group(2)))
	for name in sorted(earlier):
		orig = os.path.join(orig_dir, name + ".png")
		base = os.path.join(port_dir, name + ".png")
		if not (os.path.exists(orig) and os.path.exists(base)):
			continue
		o = client(orig)
		scores = [(np.abs(client(base) - o).mean(), 0)]
		scores += [(np.abs(client(os.path.join(port_dir, "%s~%d.png" % (name, k))) - o).mean(), k) for k in earlier[name]]
		best, k = min(scores)
		if k:
			os.replace(os.path.join(port_dir, "%s~%d.png" % (name, k)), base)
		for j in earlier[name]:
			if j != k and os.path.exists(os.path.join(port_dir, "%s~%d.png" % (name, j))):
				os.remove(os.path.join(port_dir, "%s~%d.png" % (name, j)))
		at = landed.get(name)
		print("pickshot: %s shows update %s%s (mean difference %.2f; %s)" % (
			name, "?" if at is None else at - k, "" if k == 0 else ", %d before it landed" % k, best,
			", ".join("%d: %.2f" % (at - j if at is not None else -j, s) for s, j in sorted(scores, key=lambda t: t[1]))))


if __name__ == "__main__":
	main()
