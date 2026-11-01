"""Clips for shotdiff: a folder of numbered frames on each side, compared frame by frame (docs/SHOTDIFF.md).

A clip folder holds numbered PNGs and a clip.json ({"interval_ms": 100}) on at least one side; the original's runs on the
wall clock and the port's by updates, so the frames do not line up exactly. Each port frame is matched to the first
original frame within a window of frames that agrees with it under the tolerance, nearest the overall best offset first.
A port frame no original frame in the window agrees with is flagged, and boxed as a still would be. Sequences that drift
(the offset changes over the clip) or stall (one side shows the same picture for many frames while the other moves) are
reported as notes."""
import collections
import copy
import os
import re

import numpy as np
from PIL import Image

import report
import shotdiff as sd

GAP = 6  # pixels between the two halves of an APNG frame
MAX_CLIP_CROPS = 12  # flagged frames that get crops: the worst ones
CROPS_PER_FRAME = 3
APNG_PIXEL_BUDGET = 20000000  # the APNG is scaled down until its frames hold at most this many pixels together
APNG_MAX_WIDTH = 1280
KEEP = 24  # frames kept in memory per side
BORDER = (255, 26, 217)


def natural_key(s):
	return [int(t) if t.isdigit() else t.lower() for t in re.split(r"(\d+)", s)]


def norm_clip(n):
	n = n.replace("\\", "/")
	if n.startswith("./"):
		n = n[2:]
	n = n.strip("/").lower()
	return n[:-4] if n.endswith(".png") else n


def walk_dirs(folder):
	"""{lower-case relative dir: (relative dir, full dir, holds a clip.json)} of every folder below one."""
	out = {}
	for dirpath, dirs, files in os.walk(folder):
		dirs.sort()
		if dirpath == folder:
			continue
		rel = os.path.relpath(dirpath, folder).replace("\\", "/")
		out[rel.lower()] = (rel, dirpath, any(f.lower() == "clip.json" for f in files))
	return out


def find_clips(orig_dir, port_dir, mapping, only=None):
	"""-> (pairs [(name, orig dir, port dir)], orig-only names, port-only names, folders to skip for stills (orig, port),
	the mapping without the clip entries). A folder is a clip when it holds a clip.json on either side; the name is its
	relative path with a trailing slash."""
	od, pd = walk_dirs(orig_dir), walk_dirs(port_dir)
	keys = {k for k, v in od.items() if v[2]} | {k for k, v in pd.items() if v[2]}
	keys = {k for k in keys if not any(k.startswith(p + "/") for p in keys)}  # a folder inside a clip is part of it
	clip_map, rest = {}, {}
	for k, v in (mapping or {}).items():
		if norm_clip(k) in keys and norm_clip(k) in od:
			clip_map[norm_clip(k)] = norm_clip(v)
		else:
			rest[k] = v
	pats = [p.strip().lower() for p in (only or []) if p.strip()]
	pairs, orig_only, used = [], [], set()
	for k in sorted(keys):
		if k not in od:
			continue
		rel = od[k][0]
		if pats and not sd.matches_any(k, pats) and not sd.matches_any(k + "/", pats):
			continue
		pk = clip_map.get(k, k)
		if pk in pd:
			pairs.append((rel + "/", od[k][1], pd[pk][1]))
			used.add(pk)
		else:
			orig_only.append(rel + "/")
	port_only = [pd[k][0] + "/" for k in sorted(keys) if k in pd and k not in used and k not in od
				 and (not pats or sd.matches_any(k, pats))]
	return pairs, orig_only, port_only, (set(keys) & set(od), (set(keys) & set(pd)) | used), rest


def frame_files(folder):
	return [os.path.join(folder, f) for f in sorted(os.listdir(folder), key=natural_key) if f.lower().endswith(".png")]


def read_interval(*dirs):
	"""-> (interval in ms, warnings) from the clip.json of either side."""
	vals, warnings = [], []
	for d in dirs:
		p = next((os.path.join(d, f) for f in os.listdir(d) if f.lower() == "clip.json"), None)
		if not p:
			continue
		try:
			j = sd.load_json(p)
			iv = j.get("interval_ms", j.get("intervalMs", j.get("interval")))
			if iv is None and j.get("fps"):
				iv = 1000.0 / float(j["fps"])
			if iv is not None and float(iv) > 0:
				vals.append(float(iv))
			else:
				warnings.append("%s has no interval_ms" % p)
		except (OSError, ValueError, TypeError) as e:
			warnings.append("%s: %s" % (p, e))
	if not vals:
		return 100, warnings + ["no interval in any clip.json: 100 ms assumed"]
	if len(set(vals)) > 1:
		warnings.append("the two clip.json files give different intervals (%s ms); the original's is used" % ", ".join("%g" % v for v in vals))
	return int(round(vals[0])), warnings


class Frames:
	"""One side of a clip, cut to the client area, loaded on demand (the latest few stay in memory)."""

	def __init__(self, paths, box):
		self.paths, self.box, self.cache, self.shape = paths, box, {}, None

	def __len__(self):
		return len(self.paths)

	def get(self, i):
		a = self.cache.get(i)
		if a is None:
			with Image.open(self.paths[i]) as im:
				a = sd.crop_array(im, self.box)
			if self.shape is None:
				self.shape = a.shape
			elif a.shape != self.shape:
				raise ValueError("%s is %dx%d but the first frame is %dx%d" % (
					os.path.basename(self.paths[i]), a.shape[1], a.shape[0], self.shape[1], self.shape[0]))
			if len(self.cache) >= KEEP:
				self.cache.pop(next(iter(self.cache)))
			self.cache[i] = a
		return a


def static_runs(static, min_len):
	"""Runs of identical consecutive frames [(first, last)] of at least min_len frames; static[i] is 'frame i equals i-1'."""
	out, i, n = [], 0, len(static)
	while i < n:
		j = i
		while j + 1 < n and static[j + 1]:
			j += 1
		if j - i + 1 >= min_len:
			out.append((i, j))
		i = j + 1
	return out


def apng_scale(n, w, h):
	"""How much to scale an APNG of n frames of two w x h halves so that it stays a sensible size."""
	total_w = 2 * w + GAP
	s = min(1.0, APNG_MAX_WIDTH / total_w, (APNG_PIXEL_BUDGET / (n * total_w * h)) ** 0.5)
	return s if s < 0.9 else 1.0  # a slight reduction would only blur it


def sig_of(p):
	"""What a mark was made against: the flagged frames and the sequence findings."""
	return "%s|d%s|%s" % (";".join("%d:%d" % (f["frame"], f["diff_pixels"]) for f in p["flagged"]), p.get("drift", 0),
						  ",".join("%s%d-%d" % (s["side"][0], s["from"], s["to"]) for s in p.get("stalls", [])))


def compare_clip(name, odir, pdir, cfg, opts, out_dir, pid):
	"""Compare a clip and write its APNG, its frames (full size) and the crops of its worst flagged frames under out_dir.
	Returns the pair record for the summary."""
	ofiles, pfiles = frame_files(odir), frame_files(pdir)
	if not ofiles or not pfiles:
		raise ValueError("no PNG frames in " + (odir if not ofiles else pdir))
	interval, warns = read_interval(odir, pdir)
	with Image.open(ofiles[0]) as oi, Image.open(pfiles[0]) as pi:
		ob, pb, onote, pnote = sd.cut_boxes(oi.size, pi.size, cfg)
	O, P = Frames(ofiles, ob), Frames(pfiles, pb)
	a0, b0 = O.get(0), P.get(0)
	h, w = a0.shape[:2]
	n, m, W = len(P), len(O), opts.window
	res = {"name": name, "kind": "clip", "size": [w, h], "orig_note": onote, "port_note": pnote, "frames": n, "orig_frames": m,
		   "interval_ms": interval, "warnings": warns, "flagged": [], "shown": [], "stalls": [], "notes": [], "drift": 0,
		   "boxes": [], "ignored": []}
	if a0.shape != b0.shape:
		res.update(status="differ", diff_pct=100.0, diff_pixels=0, compared_pixels=0, shift=[0, 0], size_mismatch=True,
				   port_size=[b0.shape[1], b0.shape[0]],
				   note="sizes differ: original %dx%d, port %dx%d; not compared (give the config a \"crop\" for each side)" % (
					   w, h, b0.shape[1], b0.shape[0]))
		return res
	ign_entries = sd.ignores_for(sd.ignore_list(cfg), name.rstrip("/"))
	ign = sd.ignore_mask((h, w), [e["rect"] for e in ign_entries])
	tol, allow = opts.tolerance, opts.allow
	memo = {}

	def count(i, j, shift):
		key = (i, j, shift)
		if key not in memo:
			memo[key] = sd.diff_count(O.get(j), P.get(i), tol, shift, ign)
		return memo[key]

	faintly = set()  # port frames that agree with their original frame only faintly (docs/SHOTDIFF.md)

	def agrees(i, j, shift):
		"""Port frame i shows original frame j: no pixel differs, or none visibly."""
		if count(i, j, shift) <= allow:
			return True
		key = ("v", i, j, shift)
		if key not in memo:
			memo[key] = not sd.has_visible(O.get(j), P.get(i), shift, ign)
		return memo[key]

	# 1. the picture's offset (once, from a few frames) and the overall frame offset
	samples = sorted({int(round(x)) for x in np.linspace(0, n - 1, min(n, 12))})
	offsets = list(range(-W, W + 1))

	def mean_count(d, shift):
		c = [count(i, i + d, shift) for i in samples if 0 <= i + d < m]
		return sum(c) / len(c) if c else float("inf")

	shift = tuple(opts.align) if opts.align is not None else (0, 0)
	if opts.align is None and opts.max_shift > 0:
		d0 = min(offsets, key=lambda d: (mean_count(d, (0, 0)), abs(d)))
		off = sorted((count(i, i + d0, (0, 0)), i) for i in samples if 0 <= i + d0 < m)
		off = [t for t in off if t[0] > allow]  # the sampled frames that differ at no shift: the middle one tells
		if off:
			_, i = off[len(off) // 2]
			shift = sd.search_shift(O.get(i + d0), P.get(i), ign, tol, opts.max_shift)

	def matches_at(d):
		return sum(1 for i in samples if 0 <= i + d < m and agrees(i, i + d, shift))

	g = min(offsets, key=lambda d: (-matches_at(d), mean_count(d, shift), abs(d)))

	# 2. every port frame: the first original frame in the window that agrees with it, nearest the overall offset first
	matched, shown, static_p = [None] * n, [None] * n, [False] * n
	for i in range(n):
		P.get(i)
		if i:
			static_p[i] = sd.diff_count(P.get(i - 1), P.get(i), tol, (0, 0), ign) <= allow
		best = None
		for j in sorted(range(max(0, i - W), min(m, i + W + 1)), key=lambda j: (abs(j - (i + g)), j)):
			c = count(i, j, shift)
			if agrees(i, j, shift):
				matched[i] = shown[i] = j
				if c > allow:
					faintly.add(i)
				break
			if best is None or c < best[1]:
				best = (j, c)
		else:
			if best:
				shown[i] = best[0]
	flagged_idx = [i for i in range(n) if matched[i] is None]
	offs = [matched[i] - i for i in range(n) if matched[i] is not None]
	best_off = g
	if offs:
		cnt = collections.Counter(offs)
		best_off = min(cnt, key=lambda d: (-cnt[d], abs(d)))
	notes = []
	notes.append("port frame i shows what the original showed at frame i%+d (%+d ms)" % (best_off, best_off * interval)
				 if offs else "no port frame matches an original frame within %d frames" % W)

	# 3. drift: how the offset changes over the clip
	drift = 0
	if len(offs) >= 4:
		q = max(1, len(offs) // 4)
		first, last = int(round(float(np.median(offs[:q])))), int(round(float(np.median(offs[-q:]))))
		drift = last - first
		if abs(drift) >= opts.drift:
			notes.append("drifts: the offset goes from %+d to %+d frames over the clip (the port runs %s the original)" % (
				first, last, "ahead of" if drift > 0 else "behind"))
	# 4. stalls: identical frames in a row on one side while the other side moves
	static_o = [False] * m
	for j in range(1, m):
		static_o[j] = sd.diff_count(O.get(j - 1), O.get(j), tol, (0, 0), ign) <= allow
	stalls = []
	for a, b in static_runs(static_p, opts.stall_min):
		moving = sum(1 for j in range(a + g + 1, b + g + 1) if 0 <= j < m and not static_o[j])
		if moving * 2 >= b - a:
			stalls.append({"side": "port", "from": a, "to": b})
	for a, b in static_runs(static_o, opts.stall_min):
		moving = sum(1 for i in range(a - g + 1, b - g + 1) if 0 <= i < n and not static_p[i])
		if moving * 2 >= b - a:
			stalls.append({"side": "original", "from": a, "to": b})
	for s in stalls:
		notes.append("stalls: the %s shows the same picture in frames %d-%d (%d frames, %d ms) while the %s moves" % (
			s["side"], s["from"], s["to"], s["to"] - s["from"] + 1, (s["to"] - s["from"] + 1) * interval,
			"original" if s["side"] == "port" else "port"))
	if flagged_idx:
		runs, start = [], flagged_idx[0]
		for k in range(1, len(flagged_idx) + 1):
			if k == len(flagged_idx) or flagged_idx[k] != flagged_idx[k - 1] + 1:
				runs.append((start, flagged_idx[k - 1]))
				if k < len(flagged_idx):
					start = flagged_idx[k]
		notes.append("%d of %d port frames match no original frame within %d frames: %s" % (
			len(flagged_idx), n, W, ", ".join(str(a) if a == b else "%d-%d" % (a, b) for a, b in runs[:12]) + (", ..." if len(runs) > 12 else "")))
	if offs and m - 1 - max(matched[i] for i in range(n) if matched[i] is not None) > W:
		notes.append("the original goes on for %d frames after the last one the port matches" % (
			m - 1 - max(matched[i] for i in range(n) if matched[i] is not None)))

	issues = bool(flagged_idx) or abs(drift) >= opts.drift or bool(stalls)
	valid = h * w - int(ign.sum())
	res.update(status="differ" if issues else ("faint" if faintly else "match"), faint_frames=sorted(faintly), shift=list(shift), offset=best_off, offset_ms=best_off * interval, drift=drift,
			   stalls=stalls, notes=notes, shown=shown, diff_pct=round(100.0 * len(flagged_idx) / n, 4), compared_pixels=valid * n,
			   ignored=[{"rect": list(e["rect"]), "label": e["label"]} for e in ign_entries], diff_pixels=0)
	if not issues:  # nothing to review: no pictures
		return res

	# 5. files: the port's frames lined up on the original's coordinates, the original frames they are shown with
	cdir = os.path.join(out_dir, "clips", pid)
	os.makedirs(cdir, exist_ok=True)
	dx, dy = shift
	fopts = copy.copy(opts)
	fopts.align = shift
	flagged, pixes = [], {}
	for i in flagged_idx:
		j = shown[i]
		if j is None:
			flagged.append({"frame": i, "orig_frame": None, "diff_pixels": int(h * w), "diff_pct": 100.0, "boxes": []})
			continue
		r = sd.compare_arrays(O.get(j), P.get(i), fopts, [e["rect"] for e in ign_entries])
		flagged.append({"frame": i, "orig_frame": j, "diff_pixels": r["diff_pixels"], "diff_pct": r["diff_pct"], "boxes": r["boxes"]})
		pixes[i] = r["pixmap"]
	worst = sorted((f for f in flagged if f["boxes"]), key=lambda f: -f["diff_pixels"])[:MAX_CLIP_CROPS]
	for f in worst:
		i, j = f["frame"], f["orig_frame"]
		for box in f["boxes"][:CROPS_PER_FRAME]:
			crop, scale = sd.make_crop(O.get(j), sd.align_port(P.get(i), dx, dy), box, opts, pixes[i])
			rel = report.frame_crop_rel(pid, i, box["n"])
			sd.save_png(crop, os.path.join(out_dir, rel))
			box["crop"] = sd.display_path(os.path.join(out_dir, rel))
			box["crop_scale"] = scale
			box["crop_size"] = list(crop.size)
	del pixes
	flag_set = set(flagged_idx)
	s = apng_scale(n, w, h)
	tw, th = max(1, round((2 * w + GAP) * s)), max(1, round(h * s))
	play, written = [], set()
	for i in range(n):
		al = sd.align_port(P.get(i), dx, dy)
		sd.save_png(al, os.path.join(cdir, "p%04d.png" % i))
		j = shown[i]
		left = O.get(j) if j is not None else np.full_like(al, sd.GRAY)
		if j is not None and j not in written:
			sd.save_png(left, os.path.join(cdir, "o%04d.png" % j))
			written.add(j)
		right = np.array(al)
		if i in flag_set:
			right[:4], right[-4:], right[:, :4], right[:, -4:] = BORDER, BORDER, BORDER, BORDER
		frame = Image.fromarray(np.concatenate([left, np.full((h, GAP, 3), 40, np.uint8), right], axis=1))
		play.append(frame.resize((tw, th), Image.LANCZOS) if s < 1 else frame)
	apng = os.path.join(out_dir, report.play_rel(pid))
	play[0].save(apng, format="PNG", save_all=True, append_images=play[1:], duration=max(interval, 20), loop=0)

	res.update(flagged=flagged, diff_pixels=sum(f["diff_pixels"] for f in flagged), apng_scale=round(s, 3),
			   images={"play": report.play_rel(pid), "dir": "clips/" + pid})
	return res
