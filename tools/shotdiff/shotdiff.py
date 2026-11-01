#!/usr/bin/env python3
"""shotdiff: compare screenshots of an original game with its port's, and write a review page (docs/SHOTDIFF.md).

    python tools/shotdiff/shotdiff.py --game <game> --orig <folder> --port <folder> [--run <id>]
        [--config <json>]       per-game settings (default games/<game>/shotdiff.json when it exists)
        [--map <file>]          names that differ: JSON {"orig.png": "port.png"} or lines "orig.png = port.png"
        [--tolerance 8]         a pixel differs when a channel differs by more than this
        [--max-shift 8]         search the port's offset up to this many pixels (0 = no search)
        [--align DX,DY]         use this offset (the port's pixel at x+DX,y+DY is the original's at x,y); no search
        [--only <glob>,...]     compare only the pairs whose name matches
        [--out <dir>]           default build/shotreview/<game>/<run>/
        [--dilate 3] [--merge 12] [--max-boxes 12] [--margin 16] [--allow 0] [--fail-on-diff]
        [--window 3] [--drift 2] [--stall-min 5]       for clips
    python tools/shotdiff/shotdiff.py --index-only [--root build/shotreview]     rebuild the all-games page only

Both folders hold PNGs. They are paired by relative path (case-insensitive), or through the map. Each picture is cut to the
game's client area first (the config's "crop", or origrun's window frame when the sizes say so), then the port is aligned
to the original, the differing pixels are grouped into boxes, and a page, summary.json and a crop per box are written under
build/shotreview/<game>/<run>/. Nothing is hidden except what an ignore rectangle of the config covers, and a pair
matches only when no pixel differs by more than the tolerance (--allow raises that to a few pixels, and says so).
A box is visible or faint (docs/SHOTDIFF.md, "Visible and faint"): faint boxes, differences no one can see (antialiased
edges, sub-pixel sampling), stay in summary.json with their measurements and go in a collapsed list at the page's end.

The config (all keys optional):
    {"client": [640, 480],
     "orig": {"crop": [3, 26, 640, 480]},       x, y, width, height of the client area in each side's pictures
     "port": {"offset": [0, 0]},                 or only x, y (the size is "client")
     "ignore": [[x, y, w, h], {"rect": [x, y, w, h], "pairs": "scr_00*", "label": "bubbles"}],   client coordinates
     "tolerance": 8, "maxShift": 8, "align": [0, 0], "names": {"orig.png": "port.png"},
     "clips": {"window": 3, "drift": 2, "stallMin": 5}}

Clips: a folder with a clip.json ({"interval_ms": 100}) on either side and numbered frames in it, in both folders at the
same relative path, is a clip rather than a set of stills (docs/SHOTDIFF.md). Its name ends in a slash.
"""
import argparse
import datetime
import fnmatch
import json
import os
import posixpath
import re
import shutil
import sys

try:
	import numpy as np
	from PIL import Image, ImageDraw, ImageFont
except ImportError as e:
	sys.exit("shotdiff: needs Python with numpy and Pillow (%s)" % e)

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
if __name__ == "__main__":
	sys.modules["shotdiff"] = sys.modules[__name__]  # so clip.py's "import shotdiff" is this module, not a second copy
import report  # noqa: E402
from report import crop_rel, img_rel  # noqa: E402
DEFAULT_REVIEW_ROOT = os.path.join(ROOT, "build", "shotreview")

# origrun's window captures include the window frame: 3 px each side, a 26 px title bar and 3 px at the bottom, so
# 646x509 for a 640x480 game
FRAME_LEFT, FRAME_TOP, FRAME_EXTRA_W, FRAME_EXTRA_H = 3, 26, 6, 29
GRAY = (128, 128, 128)  # what the aligned port shows where the shift left no picture
OUTLINE, HALO = (255, 26, 217), (255, 255, 255)
CROP_SCALE = 2  # a crop shows its pictures enlarged this much, nearest-neighbour
CROP_SMALL = (320, 240)  # a region up to this size is enlarged; a larger one is shown 1:1
CROP_MAX_HALF = (640, 480)  # and one beyond this is scaled down to fit
# Visible and faint (docs/SHOTDIFF.md, "Visible and faint"): a pixel is visibly different when, after a 3x3 box blur of
# both pictures, it still differs by VISIBLE_DELTA or more in some channel; a box is visible when it holds at least
# VISIBLE_MIN such pixels. Measured on the Bookworm Adventures pilot (2026-11-01): antialiased and sub-pixel-sampled
# sprite edges left 0-21 such pixels a box, a changed digit of thin text 158. VISIBLE_SHIFT > 0 would also forgive edges
# moved that many pixels, but a one-pixel allowance also forgave that digit (13 left, against 10-11 for edge noise).
VISIBLE_DELTA, VISIBLE_SHIFT, VISIBLE_MIN = 32, 0, 32
STRONG_DELTA = 32  # a box's measurements count its raw pixels off by this much or more


class Options:
	def __init__(self, **kw):
		self.tolerance = 8
		self.max_shift = 8
		self.align = None
		self.dilate = 3
		self.merge = 12
		self.max_boxes = 12
		self.margin = 16
		self.allow = 0  # differing pixels a pair may have and still count as matching
		self.window = 3  # clips: how many frames either way a port frame may be matched to the original's
		self.drift = 2  # clips: a sequence whose offset changes by this many frames or more drifts
		self.stall_min = 5  # clips: this many identical frames in a row, with the other side moving, is a stall
		for k, v in kw.items():
			if not hasattr(self, k):
				raise TypeError("unknown option " + k)
			setattr(self, k, v)


# ---------------------------------------------------------------- the config and the pairing

def load_json(path):
	with open(path, encoding="utf-8-sig") as f:
		return json.load(f)


def norm_name(n):
	n = n.replace("\\", "/")
	if n.startswith("./"):
		n = n[2:]
	n = n.lower()
	return n if n.endswith(".png") else n + ".png"


def read_map(path):
	"""A JSON object, or lines "orig = port" (also tab, "->" or spaces between them; # starts a comment)."""
	with open(path, encoding="utf-8-sig") as f:
		text = f.read()
	if text.lstrip().startswith("{"):
		return dict(json.loads(text))
	out = {}
	for line in text.splitlines():
		line = line.split("#", 1)[0].strip()
		if not line:
			continue
		parts = [p for p in re.split(r"\s*(?:=|->)\s*|\s+", line) if p]
		if len(parts) != 2:
			raise ValueError("%s: cannot read the mapping line '%s'" % (path, line))
		out[parts[0]] = parts[1]
	return out


def list_pngs(folder, skip=()):
	"""{normalized relative path: (relative path, full path)} of the PNGs under a folder, except those in the clip
	folders named by skip (lower-case relative paths)."""
	out = {}
	for dirpath, dirs, files in os.walk(folder):
		dirs.sort()
		reldir = os.path.relpath(dirpath, folder).replace("\\", "/").lower()
		if reldir in skip:
			dirs[:] = []
			continue
		for f in sorted(files):
			if f.lower().endswith(".png"):
				full = os.path.join(dirpath, f)
				rel = os.path.relpath(full, folder).replace("\\", "/")
				out[rel.lower()] = (rel, full)
	return out


def matches_any(key, pats):
	return any(fnmatch.fnmatch(key, p) or fnmatch.fnmatch(posixpath.basename(key), p) for p in pats)


def pair_files(orig_dir, port_dir, mapping, only=None, skip_orig=(), skip_port=()):
	"""-> (pairs [(name, orig path, port name, port path)], orig-only names, port-only names, warnings)."""
	orig, port = list_pngs(orig_dir, skip_orig), list_pngs(port_dir, skip_port)
	warnings = []
	maps = {}
	for k, v in (mapping or {}).items():
		nk, nv = norm_name(k), norm_name(v)
		if nk not in orig:
			warnings.append("map: '%s' is not in the original folder" % k)
		elif nv not in port:
			warnings.append("map: '%s' (for '%s') is not in the port folder" % (v, k))
		maps[nk] = nv
	pats = [p.strip().lower() for p in (only or []) if p.strip()]
	pairs, orig_only, used = [], [], set()
	for key in sorted(orig):
		rel, full = orig[key]
		if pats and not matches_any(key, pats):
			continue
		pkey = maps.get(key, key)
		if pkey in port:
			pairs.append((rel, full, port[pkey][0], port[pkey][1]))
			used.add(pkey)
		else:
			orig_only.append(rel)
	port_only = [port[k][0] for k in sorted(port) if k not in used and (not pats or matches_any(k, pats))]
	return pairs, orig_only, port_only, warnings


def rect_of(v, what):
	if not (isinstance(v, (list, tuple)) and len(v) == 4 and all(isinstance(i, int) for i in v)):
		raise ValueError("%s: expected [x, y, width, height], got %r" % (what, v))
	return tuple(v)


def ignore_list(cfg):
	"""The config's ignore entries, normalized: [{"rect": (x,y,w,h), "label": str, "pairs": glob list or None}]."""
	out = []
	for item in cfg.get("ignore", []):
		if isinstance(item, dict):
			out.append({"rect": rect_of(item.get("rect"), "ignore"), "label": item.get("label", ""),
						"pairs": item.get("pairs") or None})
		else:
			out.append({"rect": rect_of(item, "ignore"), "label": "", "pairs": None})
	return out


def ignores_for(entries, name):
	"""The ignore entries that apply to a pair."""
	return [e for e in entries
			if not e["pairs"] or any(fnmatch.fnmatch(name.lower(), p.strip().lower()) for p in e["pairs"].split(","))]


# ---------------------------------------------------------------- pictures

def explicit_box(size, side_cfg, client):
	"""The config's crop or offset for a side as (x, y, w, h), or None."""
	w, h = size
	if "crop" in side_cfg:
		box = rect_of(side_cfg["crop"], "crop")
	elif "offset" in side_cfg:
		x, y = side_cfg["offset"]
		cw, ch = client if client else (w - x, h - y)
		box = (x, y, cw, ch)
	else:
		return None
	x, y, cw, ch = box
	if x < 0 or y < 0 or cw <= 0 or ch <= 0 or x + cw > w or y + ch > h:
		raise ValueError("the crop %s is outside the %dx%d picture" % (list(box), w, h))
	return box


def frame_box(size):
	return (FRAME_LEFT, FRAME_TOP, size[0] - FRAME_EXTRA_W, size[1] - FRAME_EXTRA_H)


def cut_boxes(osize, psize, cfg):
	"""Where the game's client area is in each side's pictures -> (orig box, port box, orig note, port note); a box is
	(x, y, w, h) or None for the whole picture. An explicit crop or offset in the config is used; otherwise origrun's
	window frame is cut away from a picture that is exactly the other side's size (or the config's client size) plus
	the frame."""
	client = tuple(cfg["client"]) if cfg.get("client") else None
	ob = explicit_box(osize, cfg.get("orig", {}), client)
	pb = explicit_box(psize, cfg.get("port", {}), client)
	onote = "cut to %dx%d at %d,%d" % (ob[2], ob[3], ob[0], ob[1]) if ob else None
	pnote = "cut to %dx%d at %d,%d" % (pb[2], pb[3], pb[0], pb[1]) if pb else None
	osz, psz = (ob[2], ob[3]) if ob else osize, (pb[2], pb[3]) if pb else psize
	framed = "cut away origrun's window frame"
	plus = (FRAME_EXTRA_W, FRAME_EXTRA_H)
	if client:
		if not ob and osz == (client[0] + plus[0], client[1] + plus[1]):
			ob, onote = frame_box(osz), framed
		if not pb and psz == (client[0] + plus[0], client[1] + plus[1]):
			pb, pnote = frame_box(psz), framed
	elif osz != psz:
		if not ob and osz == (psz[0] + plus[0], psz[1] + plus[1]):
			ob, onote = frame_box(osz), framed
		elif not pb and psz == (osz[0] + plus[0], osz[1] + plus[1]):
			pb, pnote = frame_box(psz), framed
	return ob, pb, onote, pnote


def crop_array(img, box):
	"""A picture (cut to a box) as an RGB array."""
	if box:
		img = img.crop((box[0], box[1], box[0] + box[2], box[1] + box[3]))
	return np.asarray(img.convert("RGB"))


def client_areas(oi, pi, cfg):
	"""Both pictures cut to the game's client area -> (orig array, port array, orig note, port note)."""
	ob, pb, onote, pnote = cut_boxes(oi.size, pi.size, cfg)
	return crop_array(oi, ob), crop_array(pi, pb), onote, pnote


def luma(a):
	a = a.astype(np.int32)
	return ((a[..., 0] * 77 + a[..., 1] * 151 + a[..., 2] * 28) >> 8).astype(np.int16)


def overlap(w, h, dx, dy):
	"""The original's region [x0,x1) x [y0,y1) whose port pixel (x+dx, y+dy) exists."""
	return max(0, -dx), max(0, -dy), min(w, w - dx), min(h, h - dy)


def ignore_mask(shape, rects):
	m = np.zeros(shape, bool)
	for x, y, w, h in rects:
		m[max(0, y):max(0, y + h), max(0, x):max(0, x + w)] = True
	return m


def search_shift(a, b, ign, tol, max_shift):
	"""The port's offset (dx, dy) in +-max_shift that best matches the original: orig(x,y) ~ port(x+dx, y+dy).
	Judged on the brightness (every second pixel of a huge picture). A shift is taken only when it clearly beats (0,0)."""
	h, w = a.shape[:2]
	ga, gb = luma(a), luma(b)
	step = 1 if w * h <= 1000000 else 2
	has_ign = bool(ign.any())

	def score(dx, dy):
		x0, y0, x1, y1 = overlap(w, h, dx, dy)
		if x1 - x0 < 16 or y1 - y0 < 16:
			return None
		d = np.abs(ga[y0:y1:step, x0:x1:step] - gb[y0 + dy:y1 + dy:step, x0 + dx:x1 + dx:step]) > tol
		n = d.size
		if has_ign:
			valid = ~ign[y0:y1:step, x0:x1:step]
			n = int(valid.sum())
			d &= valid
		return float(d.sum()) / n if n else None

	zero = score(0, 0)
	if zero is None or zero == 0:
		return (0, 0)
	best, best_s = (0, 0), zero
	cands = sorted(((dx, dy) for dx in range(-max_shift, max_shift + 1) for dy in range(-max_shift, max_shift + 1)),
				   key=lambda s: (abs(s[0]) + abs(s[1]), s))
	for dx, dy in cands:
		if (dx, dy) == (0, 0):
			continue
		s = score(dx, dy)
		if s is not None and s < best_s:
			best, best_s = (dx, dy), s
	if best != (0, 0) and zero - best_s >= 0.001 and best_s <= zero * 0.9:
		return best
	return (0, 0)


# ---------------------------------------------------------------- boxes

def dilate(m, r):
	out = m.copy()
	for k in range(1, r + 1):
		out[:, k:] |= m[:, :-k]
		out[:, :-k] |= m[:, k:]
	m2 = out.copy()
	for k in range(1, r + 1):
		out[k:, :] |= m2[:-k, :]
		out[:-k, :] |= m2[k:, :]
	return out


def components(m):
	"""The 8-connected components of a boolean picture as [x0, y0, x1, y1] (end exclusive), by runs of each row."""
	parent, runs = [], []  # runs: (y, x0, x1, id)

	def find(i):
		while parent[i] != i:
			parent[i] = parent[parent[i]]
			i = parent[i]
		return i

	prev = []
	for y in range(m.shape[0]):
		row = m[y]
		if not row.any():
			prev = []
			continue
		d = np.diff(np.concatenate(([0], row.view(np.int8), [0])))
		starts, ends = np.flatnonzero(d == 1).tolist(), np.flatnonzero(d == -1).tolist()
		cur = []
		j = 0
		for s, e in zip(starts, ends):
			rid = len(parent)
			parent.append(rid)
			while j < len(prev) and prev[j][1] < s:  # the previous row's runs that end before this one starts
				j += 1
			k = j
			while k < len(prev) and prev[k][0] <= e:
				parent[find(prev[k][2])] = find(rid)
				k += 1
			cur.append((s, e, rid))
			runs.append((y, s, e, rid))
		prev = cur
	boxes = {}
	for y, s, e, rid in runs:
		r = find(rid)
		b = boxes.get(r)
		if b is None:
			boxes[r] = [s, y, e, y + 1]
		else:
			b[0], b[1], b[2], b[3] = min(b[0], s), min(b[1], y), max(b[2], e), y + 1
	return list(boxes.values())


def merge_boxes(boxes, gap):
	boxes = [list(b) for b in boxes]
	changed = True
	while changed:
		changed = False
		out = []
		for b in boxes:
			for o in out:
				if b[0] - o[2] <= gap and o[0] - b[2] <= gap and b[1] - o[3] <= gap and o[1] - b[3] <= gap:
					o[0], o[1], o[2], o[3] = min(o[0], b[0]), min(o[1], b[1]), max(o[2], b[2]), max(o[3], b[3])
					changed = True
					break
			else:
				out.append(b)
		boxes = out
	return boxes


def blur3(a):
	"""A 3x3 box blur of an RGB picture (edges repeated), as int32."""
	h, w = a.shape[:2]
	p = np.pad(a.astype(np.int32), ((1, 1), (1, 1), (0, 0)), mode="edge")
	acc = np.zeros((h, w, a.shape[2]), np.int32)
	for dy in range(3):
		for dx in range(3):
			acc += p[dy:dy + h, dx:dx + w]
	return acc // 9


def visible_delta(a, b):
	"""Per pixel, the largest channel difference between the blurred pictures, at the port position within VISIBLE_SHIFT
	pixels that agrees best: what is left of a difference once antialiasing and one-pixel edge shifts are absorbed."""
	ab, bb = blur3(a), blur3(b)
	h, w = ab.shape[:2]
	r = VISIBLE_SHIFT
	bp = np.pad(bb, ((r, r), (r, r), (0, 0)), mode="edge")
	best = None
	for dy in range(-r, r + 1):
		for dx in range(-r, r + 1):
			d = np.abs(ab - bp[r + dy:r + dy + h, r + dx:r + dx + w]).max(axis=2)
			best = d if best is None else np.minimum(best, d)
	return best.astype(np.int16)


def visible_and_faint(mask, dmax, vis, opts):
	"""Split a pair's differing pixels into visible boxes (around the visibly different pixels, each holding at least
	VISIBLE_MIN of them) and faint boxes (the differing pixels outside those), each with its measurements. A visible box
	is grown over the differing pieces it touches, so it outlines the whole change, not only its visibly different core."""
	vis = vis & mask
	seeds = [b for b in find_boxes(vis, dmax, opts) if b["pixels"] >= VISIBLE_MIN]
	grown = []
	if seeds:
		pieces = components(mask)
		for b in seeds:
			x0, y0, x1, y1 = b["x"], b["y"], b["x"] + b["w"], b["y"] + b["h"]
			for c in pieces:
				if c[0] < b["x"] + b["w"] and b["x"] < c[2] and c[1] < b["y"] + b["h"] and b["y"] < c[3]:
					x0, y0, x1, y1 = min(x0, c[0]), min(y0, c[1]), max(x1, c[2]), max(y1, c[3])
			grown.append([x0, y0, x1, y1])
		grown = merge_boxes(grown, 0)
	vbox = [{"x": x0, "y": y0, "w": x1 - x0, "h": y1 - y0} for x0, y0, x1, y1 in grown]
	covered = np.zeros(mask.shape, bool)
	for b in vbox:
		covered[b["y"]:b["y"] + b["h"], b["x"]:b["x"] + b["w"]] = True
	fbox = find_boxes(mask & ~covered, dmax, opts)
	for b in vbox + fbox:
		sl = (slice(b["y"], b["y"] + b["h"]), slice(b["x"], b["x"] + b["w"]))
		sub = mask[sl]
		b["visible_pixels"] = int(vis[sl].sum())
		b["pixels"] = int(sub.sum())
		b["max_delta"] = int(dmax[sl][sub].max()) if sub.any() else 0
		b["strong_pixels"] = int((dmax[sl][sub] >= STRONG_DELTA).sum())
	for b in fbox:
		b["faint"] = True
	vbox.sort(key=lambda b: (-b["pixels"], b["y"], b["x"]))
	for i, b in enumerate(vbox, 1):
		b["n"] = i
	return vbox, fbox


def visible_boxes_in(m):
	"""How many boxes, grouped as find_boxes groups them, hold at least VISIBLE_MIN of m's pixels."""
	o = Options()
	r = o.dilate
	comps = components(dilate(np.pad(m, r), r))
	boxes = merge_boxes([[c[0], c[1], c[2] - 2 * r, c[3] - 2 * r] for c in comps], o.merge)
	return sum(1 for x0, y0, x1, y1 in boxes if int(m[y0:y1, x0:x1].sum()) >= VISIBLE_MIN)


def has_visible(a, b, shift=(0, 0), ign=None):
	"""Whether the original a and the port b (at the offset) differ visibly anywhere: a clip frame matches without it."""
	dx, dy = shift
	h, w = a.shape[:2]
	x0, y0, x1, y1 = overlap(w, h, dx, dy)
	if x1 <= x0 or y1 <= y0:
		return True
	vis = visible_delta(a[y0:y1, x0:x1], b[y0 + dy:y1 + dy, x0 + dx:x1 + dx]) >= VISIBLE_DELTA
	if ign is not None:
		vis &= ~ign[y0:y1, x0:x1]
	return int(vis.sum()) >= VISIBLE_MIN and visible_boxes_in(vis) > 0


def find_boxes(mask, dmax, opts):
	"""Boxes around the differing pixels: dilate, label the connected pieces, merge the near ones."""
	if not mask.any():
		return []
	r = opts.dilate
	# padded by r so the dilation is never cut at the border; a piece's box then reaches r past its pixels on each side
	comps = components(dilate(np.pad(mask, r), r))
	boxes = [[c[0], c[1], c[2] - 2 * r, c[3] - 2 * r] for c in comps]
	gap = opts.merge
	merged = merge_boxes(boxes, gap)
	while len(merged) > opts.max_boxes:
		gap *= 2
		merged = merge_boxes(boxes, gap)
	out = []
	for x0, y0, x1, y1 in merged:
		sub = mask[y0:y1, x0:x1]
		sel = dmax[y0:y1, x0:x1][sub]
		out.append({"x": x0, "y": y0, "w": x1 - x0, "h": y1 - y0, "pixels": int(sub.sum()), "max_delta": int(sel.max())})
	out.sort(key=lambda b: (-b["pixels"], b["y"], b["x"]))
	for i, b in enumerate(out, 1):
		b["n"] = i
	return out


# ---------------------------------------------------------------- comparing one pair

def align_port(b, dx, dy):
	"""The port picture moved onto the original's coordinates (orig(x,y) ~ port(x+dx, y+dy)); GRAY where it has none."""
	h, w = b.shape[:2]
	x0, y0, x1, y1 = overlap(w, h, dx, dy)
	aligned = np.full_like(b, GRAY)
	if x1 > x0 and y1 > y0:
		aligned[y0:y1, x0:x1] = b[y0 + dy:y1 + dy, x0 + dx:x1 + dx]
	return aligned


def diff_count(a, b, tol, shift=(0, 0), ign=None):
	"""How many pixels of the original a differ from the port b (at the offset) by more than tol."""
	dx, dy = shift
	h, w = a.shape[:2]
	x0, y0, x1, y1 = overlap(w, h, dx, dy)
	if x1 <= x0 or y1 <= y0:
		return 0
	m = np.abs(a[y0:y1, x0:x1].astype(np.int16) - b[y0 + dy:y1 + dy, x0 + dx:x1 + dx].astype(np.int16)).max(axis=2) > tol
	if ign is not None:
		m &= ~ign[y0:y1, x0:x1]
	return int(m.sum())


def compare_arrays(a, b, opts, ignore_rects=()):
	"""Compare two client pictures of the same size. Returns a dict with status, diff_pct, diff_pixels, compared_pixels,
	shift, boxes and 'aligned' (the port moved onto the original's coordinates, GRAY where it has no picture)."""
	h, w = a.shape[:2]
	ign = ignore_mask((h, w), ignore_rects)
	if opts.align is not None:
		dx, dy = opts.align
	elif opts.max_shift > 0:
		dx, dy = search_shift(a, b, ign, opts.tolerance, opts.max_shift)
	else:
		dx, dy = 0, 0
	x0, y0, x1, y1 = overlap(w, h, dx, dy)
	aligned = align_port(b, dx, dy)
	mask = np.zeros((h, w), bool)
	dmax = np.zeros((h, w), np.int16)
	vis = np.zeros((h, w), bool)
	valid = 0
	if x1 > x0 and y1 > y0:
		delta = np.abs(a[y0:y1, x0:x1].astype(np.int16) - aligned[y0:y1, x0:x1].astype(np.int16)).max(axis=2)
		mask[y0:y1, x0:x1] = (delta > opts.tolerance) & ~ign[y0:y1, x0:x1]
		dmax[y0:y1, x0:x1] = delta
		valid = delta.size - int(ign[y0:y1, x0:x1].sum())
	count = int(mask.sum())
	boxes, faint = [], []
	if count > opts.allow:
		vis[y0:y1, x0:x1] = (visible_delta(a[y0:y1, x0:x1], aligned[y0:y1, x0:x1]) >= VISIBLE_DELTA) & ~ign[y0:y1, x0:x1]
		boxes, faint = visible_and_faint(mask, dmax, vis, opts)
	return {
		"status": "differ" if boxes else ("faint" if faint else "match"),
		"diff_pixels": count,
		"visible_pixels": int(sum(b["visible_pixels"] for b in boxes)),
		"compared_pixels": valid,
		"diff_pct": round(100.0 * count / valid, 4) if valid else 0.0,
		"shift": [dx, dy],
		"boxes": boxes,
		"faint_boxes": faint,
		"aligned": aligned,
	}


def compare_pair(orig_path, port_path, cfg, opts, name):
	"""-> (result dict, original client array, port array aligned to it)."""
	a, b, onote, pnote = client_areas(Image.open(orig_path), Image.open(port_path), cfg)
	res = {"name": name, "size": [a.shape[1], a.shape[0]], "orig_note": onote, "port_note": pnote}
	if a.shape != b.shape:
		res.update(status="differ", diff_pct=100.0, diff_pixels=0, compared_pixels=0, shift=[0, 0], boxes=[], size_mismatch=True,
				   ignored=[], port_size=[b.shape[1], b.shape[0]],
				   note="sizes differ: original %dx%d, port %dx%d; not compared (give the config a \"crop\" for each side)" % (
					   a.shape[1], a.shape[0], b.shape[1], b.shape[0]))
		return res, a, b
	ign = ignores_for(ignore_list(cfg), name)
	r = compare_arrays(a, b, opts, [e["rect"] for e in ign])
	aligned = r.pop("aligned")
	res.update(r)
	res["ignored"] = [{"rect": list(e["rect"]), "label": e["label"]} for e in ign]
	return res, a, aligned


# ---------------------------------------------------------------- crops

def make_crop(a, b, box, opts):
	"""The original and the port side by side around one box, each with a margin, enlarged 2x with nearest-neighbour
	(1:1 when the region is over 320x240, scaled down to fit when over 640x480); the box outlined in both.
	-> (image, scale)"""
	h, w = a.shape[:2]
	m = opts.margin
	x0, y0 = max(0, box["x"] - m), max(0, box["y"] - m)
	x1, y1 = min(w, box["x"] + box["w"] + m), min(h, box["y"] + box["h"] + m)
	cw, ch = x1 - x0, y1 - y0
	scale = float(CROP_SCALE if cw <= CROP_SMALL[0] and ch <= CROP_SMALL[1] else 1)
	if cw > CROP_MAX_HALF[0] or ch > CROP_MAX_HALF[1]:
		scale = min(CROP_MAX_HALF[0] / cw, CROP_MAX_HALF[1] / ch)
	tw, th = max(1, round(cw * scale)), max(1, round(ch * scale))
	label_h, gap = 14, 6
	out = Image.new("RGB", (tw * 2 + gap, th + label_h), (40, 40, 40))
	d = ImageDraw.Draw(out)
	font = ImageFont.load_default()
	for i, (img, label) in enumerate(((a, "original"), (b, "port"))):
		part = Image.fromarray(np.ascontiguousarray(img[y0:y1, x0:x1]))
		if scale != 1:
			part = part.resize((tw, th), Image.NEAREST if scale >= 1 else Image.LANCZOS)
		ox = i * (tw + gap)
		out.paste(part, (ox, label_h))
		bx0, by0 = (box["x"] - x0) * scale - 1, (box["y"] - y0) * scale - 1
		bx1, by1 = (box["x"] + box["w"] - x0) * scale, (box["y"] + box["h"] - y0) * scale
		d.rectangle((ox + bx0 - 1, label_h + by0 - 1, ox + bx1 + 1, label_h + by1 + 1), outline=HALO)
		d.rectangle((ox + bx0, label_h + by0, ox + bx1, label_h + by1), outline=OUTLINE)
		d.text((ox + 2, 0), label, fill=(230, 230, 230), font=font)
	return out, (round(scale, 3) if scale != int(scale) else int(scale))


# ---------------------------------------------------------------- a whole run

def slug(name):
	s = re.sub(r"\.png$", "", name, flags=re.I)
	return re.sub(r"[^A-Za-z0-9._-]+", "_", s).strip("._") or "pair"


def display_path(p):
	"""A path a session can open: relative to the repository when it is inside it."""
	p = os.path.abspath(p)
	try:
		if os.path.commonpath([p, ROOT]) == ROOT:
			return os.path.relpath(p, ROOT).replace("\\", "/")
	except ValueError:
		pass
	return p.replace("\\", "/")


def save_png(arr_or_img, path):
	img = arr_or_img if isinstance(arr_or_img, Image.Image) else Image.fromarray(np.ascontiguousarray(arr_or_img))
	img.save(path, compress_level=6)


def write_json(path, obj):
	tmp = path + ".tmp"
	with open(tmp, "w", encoding="utf-8", newline="\n") as f:
		json.dump(obj, f, indent=1)
		f.write("\n")
	os.replace(tmp, path)


def scan_runs(root):
	"""{game: [(mtime, run, summary)]} for every run under root that has a summary.json, newest first."""
	runs = {}
	if not os.path.isdir(root):
		return runs
	for game in sorted(os.listdir(root)):
		gdir = os.path.join(root, game)
		if not os.path.isdir(gdir):
			continue
		for run in sorted(os.listdir(gdir)):
			sp = os.path.join(gdir, run, "summary.json")
			if os.path.isfile(sp):
				try:
					runs.setdefault(game, []).append((os.path.getmtime(sp), run, load_json(sp)))
				except (OSError, ValueError):
					pass
	for g in runs:
		runs[g].sort(key=lambda t: -t[0])
	return runs


def read_marks(run_dir):
	p = os.path.join(run_dir, "marks.json")
	try:
		d = load_json(p)
		return d if isinstance(d, dict) and isinstance(d.get("marks"), dict) else None
	except (OSError, ValueError):
		return None


def carry_marks(root, game, run, summary):
	"""A new run of a game starts with the marks of the game's latest earlier run for the pairs that still differ."""
	out_dir = os.path.join(root, game, run)
	earlier = [(m, r) for m, r, s in scan_runs(root).get(game, []) if r != run]
	if not earlier:
		return
	prev = read_marks(os.path.join(root, game, earlier[0][1]))
	if not prev:
		return
	cur = read_marks(out_dir) or {"game": game, "run": run, "updated": "", "marks": {}}
	differing = {p["name"] for p in summary["pairs"] if p["status"] == "differ"}
	added = 0
	for name, m in prev["marks"].items():
		if name in differing and name not in cur["marks"] and (m.get("mark") or m.get("note")):
			cur["marks"][name] = dict(m, carried_from=earlier[0][1])
			added += 1
	if added:
		cur["updated"] = max(cur.get("updated", ""), prev.get("updated", ""))
		write_json(os.path.join(out_dir, "marks.json"), cur)


def write_index(root):
	"""build/shotreview/index.html: every game with its latest run and that run's counts."""
	entries = []
	for game, runs in scan_runs(root).items():
		_, run, s = runs[0]
		names = [p["name"] for p in s["pairs"] if p["status"] == "differ"]
		marks = (read_marks(os.path.join(root, game, run)) or {}).get("marks", {})
		kinds = {"issue": 0, "unsure": 0, "fine": 0}
		for n in names:
			k = marks.get(n, {}).get("mark")
			if k in kinds:
				kinds[k] += 1
		entries.append({"game": game, "run": run, "generated": s.get("generated", ""), "counts": s["counts"],
						"names": names, "marks": kinds, "earlier": [r for _, r, _ in runs[1:]]})
	entries.sort(key=lambda e: (-(len(e["names"]) - sum(e["marks"].values())), e["game"]))
	os.makedirs(root, exist_ok=True)
	path = os.path.join(root, "index.html")
	with open(path, "w", encoding="utf-8", newline="\n") as f:
		f.write(report.render_index(entries))
	return path


def run_compare(orig_dir, port_dir, out_dir, game, run, cfg=None, mapping=None, opts=None, only=None, root=None):
	"""Compare two folders and write everything under out_dir (root: the review folder whose index and marks to update)."""
	cfg, opts = cfg or {}, opts or Options()
	import clip
	mapping = dict(cfg.get("names", {}), **(mapping or {}))
	clips, clip_orig_only, clip_port_only, (skip_o, skip_p), mapping = clip.find_clips(orig_dir, port_dir, mapping, only)
	pairs, orig_only, port_only, warnings = pair_files(orig_dir, port_dir, mapping, only, skip_o, skip_p)
	orig_only, port_only = orig_only + clip_orig_only, port_only + clip_port_only
	for sub in ("img", "crops", "clips"):
		shutil.rmtree(os.path.join(out_dir, sub), ignore_errors=True)
		os.makedirs(os.path.join(out_dir, sub), exist_ok=True)
	results, used_ids = [], set()
	for name, opath, pname, ppath in pairs:
		pid, k = slug(name), 2
		while pid in used_ids:
			pid, k = "%s_%d" % (slug(name), k), k + 1
		used_ids.add(pid)
		try:
			res, a, b = compare_pair(opath, ppath, cfg, opts, name)
		except Exception as e:  # one unreadable picture must not stop the run
			res = {"name": name, "status": "error", "diff_pct": 0.0, "diff_pixels": 0, "boxes": [], "faint_boxes": [], "shift": [0, 0],
				   "ignored": [], "note": "%s: %s" % (type(e).__name__, e)}
			a = b = None
		res.update(id=pid, orig=display_path(opath), port=display_path(ppath), port_name=pname)
		if res["status"] == "differ" and a is not None:
			save_png(a, os.path.join(out_dir, img_rel(pid, "orig")))
			save_png(b, os.path.join(out_dir, img_rel(pid, "port")))
			res["images"] = {"orig": img_rel(pid, "orig"), "port": img_rel(pid, "port")}
			for box in res["boxes"]:
				crop, scale = make_crop(a, b, box, opts)
				rel = crop_rel(pid, box["n"])
				save_png(crop, os.path.join(out_dir, rel))
				box["crop"] = display_path(os.path.join(out_dir, rel))
				box["crop_scale"] = scale
				box["crop_size"] = list(crop.size)
		results.append(res)
	for name, odir, pdir in clips:
		pid = "clip_" + slug(name.rstrip("/"))
		while pid in used_ids:
			pid += "_2"
		used_ids.add(pid)
		try:
			res = clip.compare_clip(name, odir, pdir, cfg, opts, out_dir, pid)
		except Exception as e:
			res = {"name": name, "kind": "clip", "status": "error", "diff_pct": 0.0, "diff_pixels": 0, "boxes": [], "flagged": [],
				   "shift": [0, 0], "ignored": [], "note": "%s: %s" % (type(e).__name__, e)}
		res.update(id=pid, orig=display_path(odir), port=display_path(pdir), port_name=name)
		results.append(res)
	for r in results:
		r.setdefault("kind", "still")
		r.setdefault("faint_boxes", [])
	results.sort(key=lambda r: (r["status"] != "differ", r["kind"] == "clip", -r["diff_pct"], -r["diff_pixels"], r["name"]))
	counts = {
		"pairs": len(results),
		"clips": len(clips),
		"matching": sum(r["status"] == "match" for r in results),
		"differing": sum(r["status"] == "differ" for r in results),
		"faint": sum(r["status"] == "faint" for r in results),
		"faint_boxes": sum(len(r["faint_boxes"]) for r in results),
		"errors": sum(r["status"] == "error" for r in results),
		"shifted": sum(r["shift"] != [0, 0] for r in results),
		"orig_only": len(orig_only),
		"port_only": len(port_only),
	}
	summary = {
		"game": game, "run": run, "generated": datetime.datetime.now().astimezone().isoformat(timespec="seconds"),
		"orig_dir": display_path(orig_dir), "port_dir": display_path(port_dir), "dir": display_path(out_dir),
		"tolerance": opts.tolerance, "max_shift": opts.max_shift,
		"visible": {"delta": VISIBLE_DELTA, "shift": VISIBLE_SHIFT, "min_pixels": VISIBLE_MIN, "blur": "3x3 box"}, "align": list(opts.align) if opts.align else None,
		"allow": opts.allow,
		"ignore": [{"rect": list(e["rect"]), "label": e["label"], "pairs": e["pairs"]} for e in ignore_list(cfg)],
		"counts": counts, "pairs": results, "orig_only": orig_only, "port_only": port_only, "warnings": warnings,
		"page": display_path(os.path.join(out_dir, "index.html")),
	}
	write_json(os.path.join(out_dir, "summary.json"), summary)
	if root:
		carry_marks(root, game, run, summary)
	with open(os.path.join(out_dir, "index.html"), "w", encoding="utf-8", newline="\n") as f:
		f.write(report.render(summary, bool(root)))
	if root:
		write_index(root)
	return summary


def summary_line(s):
	c = s["counts"]
	extra = ""
	if c["shifted"]:
		extra += ", %d shifted" % c["shifted"]
	if c["errors"]:
		extra += ", %d errors" % c["errors"]
	if c["orig_only"] or c["port_only"]:
		extra += ", unpaired: %d original-only, %d port-only" % (c["orig_only"], c["port_only"])
	return "shotdiff %s/%s: %d pairs%s, %d matching, %d differing, %d only faintly%s -> %s" % (
		s["game"], s["run"], c["pairs"], " (%d clips)" % c["clips"] if c.get("clips") else "", c["matching"], c["differing"],
		c.get("faint", 0), extra, s["page"])


def main(argv=None):
	ap = argparse.ArgumentParser(description="Compare an original game's screenshots with its port's (docs/SHOTDIFF.md).")
	ap.add_argument("--game")
	ap.add_argument("--orig", help="folder of the original's PNGs")
	ap.add_argument("--port", help="folder of the port's PNGs")
	ap.add_argument("--run", default="latest", help="name of this comparison (default latest)")
	ap.add_argument("--config")
	ap.add_argument("--map")
	ap.add_argument("--tolerance", type=int, default=None)
	ap.add_argument("--max-shift", type=int, default=None)
	ap.add_argument("--align", help="DX,DY: use this offset, do not search")
	ap.add_argument("--only", help="comma-separated globs of pair names")
	ap.add_argument("--out", help="write the run here instead of build/shotreview/<game>/<run> (no all-games page then)")
	ap.add_argument("--root", help="the review folder (default build/shotreview)")
	ap.add_argument("--index-only", action="store_true", help="rebuild the all-games page and stop")
	ap.add_argument("--dilate", type=int, default=3)
	ap.add_argument("--merge", type=int, default=12, help="boxes closer than this many pixels become one")
	ap.add_argument("--max-boxes", type=int, default=12)
	ap.add_argument("--margin", type=int, default=16, help="pixels of picture around a box in its crop")
	ap.add_argument("--allow", type=int, default=0, help="differing pixels a pair may have and still match")
	ap.add_argument("--window", type=int, default=None, help="clips: frames either way a port frame may match (default 3)")
	ap.add_argument("--drift", type=int, default=None, help="clips: frames of offset change that count as drift (default 2)")
	ap.add_argument("--stall-min", type=int, default=None, help="clips: identical frames in a row that count as a stall (default 5)")
	ap.add_argument("--fail-on-diff", action="store_true", help="exit 1 when any pair differs")
	a = ap.parse_args(argv)
	root = os.path.abspath(a.root) if a.root else DEFAULT_REVIEW_ROOT
	if a.index_only:
		print("shotdiff: all-games page -> " + display_path(write_index(root)))
		return 0
	for opt in ("game", "orig", "port"):
		if not getattr(a, opt):
			ap.error("--%s is required" % opt)
	for d in (a.orig, a.port):
		if not os.path.isdir(d):
			sys.exit("shotdiff: %s is not a folder" % d)
	cfg_path = a.config or os.path.join(ROOT, "games", a.game, "shotdiff.json")
	cfg = {}
	if os.path.isfile(cfg_path):
		cfg = load_json(cfg_path)
	elif a.config:
		sys.exit("shotdiff: no config file " + a.config)
	opts = Options(dilate=a.dilate, merge=a.merge, max_boxes=a.max_boxes, margin=a.margin, allow=a.allow)
	if a.tolerance is not None:
		opts.tolerance = a.tolerance
	elif "tolerance" in cfg:
		opts.tolerance = int(cfg["tolerance"])
	if a.max_shift is not None:
		opts.max_shift = a.max_shift
	elif "maxShift" in cfg:
		opts.max_shift = int(cfg["maxShift"])
	clipcfg = cfg.get("clips", {})
	opts.window = a.window if a.window is not None else int(clipcfg.get("window", opts.window))
	opts.drift = a.drift if a.drift is not None else int(clipcfg.get("drift", opts.drift))
	opts.stall_min = a.stall_min if a.stall_min is not None else int(clipcfg.get("stallMin", opts.stall_min))
	if a.align:
		opts.align = tuple(int(v) for v in a.align.split(","))
	elif cfg.get("align") is not None:
		opts.align = tuple(cfg["align"])
	mapping = read_map(a.map) if a.map else None
	out = os.path.abspath(a.out) if a.out else os.path.join(root, a.game, a.run)
	os.makedirs(out, exist_ok=True)
	s = run_compare(a.orig, a.port, out, a.game, a.run, cfg, mapping, opts, a.only.split(",") if a.only else None,
					root=None if a.out else root)
	for w in s["warnings"]:
		print("shotdiff: warning: " + w, file=sys.stderr)
	print(summary_line(s))
	return 1 if a.fail_on_diff and s["counts"]["differing"] + s["counts"]["errors"] else 0


if __name__ == "__main__":
	sys.exit(main())
