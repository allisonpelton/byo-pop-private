#!/usr/bin/env python3
"""Tests of tools/shotdiff on synthetic pictures: statuses, boxes, alignment, ignore rectangles, pairing, the files and
pages a run writes, mark carry-over, the all-games page and the marks server. Prints "shotdiff: N checks, M failed"
(tools/check.sh, suite unit) and exits 1 on a failure. Needs no game files."""
import http.client
import json
import os
import sys
import tempfile
import threading

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import numpy as np  # noqa: E402
from PIL import Image  # noqa: E402

import report  # noqa: E402
import serve  # noqa: E402
import shotdiff as sd  # noqa: E402

W, H = 160, 120
checks = 0
failures = []


def check(name, cond, detail=""):
	global checks
	checks += 1
	if not cond:
		failures.append(name)
		print("FAIL %s %s" % (name, detail))


def base(seed=1):
	"""A picture with plenty of structure: random 4x4 blocks in colours 16..232 (so +-100 never clips)."""
	rng = np.random.default_rng(seed)
	cells = rng.integers(16, 233, (H // 4, W // 4, 3), dtype=np.uint8)
	return np.repeat(np.repeat(cells, 4, axis=0), 4, axis=1).copy()


def change(a, x, y, w, h):
	"""A copy of a with a rectangle recoloured (every channel moves by at least 100)."""
	b = a.copy()
	b[y:y + h, x:x + w] = (b[y:y + h, x:x + w].astype(int) + 100) % 256
	return b


def shifted(a, dx, dy):
	"""The picture as a port would show it displaced by (dx, dy): port(x+dx, y+dy) = orig(x, y)."""
	b = np.full_like(a, 7)
	x0, y0, x1, y1 = sd.overlap(W, H, dx, dy)
	b[y0 + dy:y1 + dy, x0 + dx:x1 + dx] = a[y0:y1, x0:x1]
	return b


def box_tuple(b):
	return (b["x"], b["y"], b["w"], b["h"])


def allb(r):
	"""A result's boxes, visible and faint."""
	return r["boxes"] + r["faint_boxes"]


def cmp(a, b, ign=(), **kw):
	return sd.compare_arrays(a, b, sd.Options(**kw), list(ign))


def test_compare():
	a = base()
	r = cmp(a, a.copy())
	check("identical: match", r["status"] == "match" and r["boxes"] == [] and r["diff_pct"] == 0 and r["shift"] == [0, 0], r)

	b = a.copy()
	b[10:20, 10:20, 0] += 8
	check("tolerance: a difference of exactly 8 matches", cmp(a, b)["status"] == "match")
	b[10:20, 10:20, 0] += 1
	r = cmp(a, b)
	check("tolerance: 9 differs, faintly", r["status"] == "faint" and r["boxes"] == [] and [box_tuple(x) for x in r["faint_boxes"]] == [(10, 10, 10, 10)], r)
	check("tolerance: option raises it", cmp(a, b, tolerance=9)["status"] == "match")
	check("allow: a few pixels may differ", cmp(a, change(a, 5, 5, 2, 2), allow=4)["status"] == "match")

	b = change(a, 30, 40, 20, 10)
	r = cmp(a, b)
	check("changed rectangle: one box exactly there", [box_tuple(x) for x in r["boxes"]] == [(30, 40, 20, 10)], r["boxes"])
	check("changed rectangle: pixel count and percentage", r["diff_pixels"] == 200 and r["boxes"][0]["pixels"] == 200
		  and abs(r["diff_pct"] - 100.0 * 200 / (W * H)) < 1e-3, r)
	check("changed rectangle: largest channel difference", r["boxes"][0]["max_delta"] >= 100)

	r = cmp(a, change(change(a, 10, 10, 8, 8), 100, 80, 8, 8))
	check("two distant rectangles: two boxes", len(r["boxes"]) == 2 and {box_tuple(x) for x in r["boxes"]} == {(10, 10, 8, 8), (100, 80, 8, 8)}, r["boxes"])
	r = cmp(a, change(change(a, 10, 10, 8, 8), 23, 10, 8, 8))  # a gap of 5 px
	check("two near rectangles: merged into one", [box_tuple(x) for x in r["boxes"]] == [(10, 10, 21, 8)], r["boxes"])
	r = cmp(a, change(change(a, 10, 10, 8, 8), 60, 10, 8, 8), merge=60)
	check("--merge: a larger gap merges", len(r["boxes"]) == 1)
	r = cmp(a, change(a, 0, 0, 3, 3))
	check("corner at the origin: exact box", [box_tuple(x) for x in allb(r)] == [(0, 0, 3, 3)], allb(r))
	r = cmp(a, change(a, W - 3, H - 2, 3, 2))
	check("corner at the far edge: exact box", [box_tuple(x) for x in allb(r)] == [(W - 3, H - 2, 3, 2)], allb(r))
	r = cmp(a, change(a, 70, 60, 1, 1))
	check("one pixel: a faint 1x1 box", [box_tuple(x) for x in r["faint_boxes"]] == [(70, 60, 1, 1)] and r["status"] == "faint", allb(r))

	b = a.copy()
	for gx in range(8):
		for gy in range(6):
			b[gy * 20 + 5, gx * 20 + 5] = (b[gy * 20 + 5, gx * 20 + 5].astype(int) + 100) % 256
	r = cmp(a, b, merge=2)
	check("many scattered pixels: boxes capped", 1 <= len(allb(r)) <= 12 and r["diff_pixels"] == 48, len(allb(r)))
	check("many scattered pixels: nothing lost", sum(x["pixels"] for x in allb(r)) >= 48)

	# boxes: 8-connected, and the numbering follows size
	m = np.zeros((20, 20), bool)
	m[3, 3] = m[4, 4] = True
	m[10, 10:14] = True
	bx = sd.find_boxes(m, np.where(m, 50, 0).astype(np.int16), sd.Options(dilate=0, merge=0))
	check("find_boxes: diagonal pixels are one piece, ordered by size",
		  [box_tuple(x) for x in bx] == [(10, 10, 4, 1), (3, 3, 2, 2)] and [x["n"] for x in bx] == [1, 2], bx)


def scene(draw_fn, ss=4):
	"""A smooth picture (a soft gradient), drawn on by draw_fn(ImageDraw, scale) at ss times the size and scaled down:
	antialiased like a game's sprites and text."""
	from PIL import ImageDraw
	g = np.linspace(60, 180, W * ss, dtype=np.uint8)
	big = Image.fromarray(np.stack([np.tile(g, (H * ss, 1))] * 3, axis=2))
	draw_fn(ImageDraw.Draw(big), ss)
	return np.array(big.resize((W, H), Image.BOX))


def glyph(ch, x, y, k=3):
	"""A glyph of the default bitmap font, k times its size (strokes k px wide, like a game's text), black on the scene."""
	from PIL import ImageDraw, ImageFont
	t = Image.new("L", (12, 14), 0)
	ImageDraw.Draw(t).text((1, 1), ch, fill=255, font=ImageFont.load_default())
	t = t.resize((12 * k, 14 * k), Image.NEAREST)
	a = scene(lambda d, ss: None)
	m = np.asarray(t) > 0
	a[y:y + 14 * k, x:x + 12 * k][m] = (20, 20, 20)
	return a


def test_visible():
	def disc(ss):
		return scene(lambda d, k: d.ellipse((40 * k, 40 * k, 80 * k - 1, 80 * k - 1), fill=(240, 220, 40)), ss)
	a, b = disc(4), disc(3)
	r = cmp(a, b, max_shift=0)
	check("antialiased edge (one disc, sampled 4x4 and 3x3): differs, but only faintly", r["status"] == "faint" and r["boxes"] == []
		  and r["faint_boxes"] and r["diff_pixels"] > 0, (r["status"], r["diff_pixels"], [box_tuple(x) for x in r["boxes"]]))
	r = cmp(glyph("8", 40, 40), glyph("3", 40, 40), max_shift=0)
	check("a changed glyph (8 against 3) is visible", r["status"] == "differ" and len(r["boxes"]) == 1
		  and r["boxes"][0]["visible_pixels"] >= sd.VISIBLE_MIN, [(box_tuple(x), x["visible_pixels"]) for x in r["boxes"]])
	a = scene(lambda d, ss: None)
	b = scene(lambda d, ss: d.rectangle((90 * ss, 30 * ss, 104 * ss - 1, 46 * ss - 1), fill=(200, 40, 40)))
	r = cmp(a, b, max_shift=0)
	check("a missing sprite is visible, its box around the sprite", r["status"] == "differ"
		  and [box_tuple(x) for x in r["boxes"]] == [(90, 30, 14, 16)], [box_tuple(x) for x in r["boxes"]])
	r = cmp(a, a.copy())
	check("visible: identical pictures have no boxes of either kind", r["status"] == "match" and allb(r) == [])
	# a rigid one-pixel shift: a sprite of sharp stripes drawn a pixel to the right differs visibly, and is faint, "shifted",
	# because moving the port back a pixel makes it agree; the picture is not lined up as a whole (max_shift 0), as when
	# only one sprite moved
	def stripes(x, y):
		s = scene(lambda d, ss: None)
		for i in range(0, 24, 4):
			s[y:y + 20, x + i:x + i + 2] = (250, 250, 250)
			s[y:y + 20, x + i + 2:x + i + 4] = (10, 10, 10)
		return s
	r = cmp(stripes(60, 40), stripes(61, 40), max_shift=0)
	check("a sharp sprite a pixel to the right: faint, shifted 1 px", r["status"] == "faint" and r["boxes"] == []
		  and [(x.get("shifted"), x["visible_pixels"] >= sd.VISIBLE_MIN) for x in r["faint_boxes"]] == [([1, 0], True)],
		  [(box_tuple(x), x.get("shifted"), x["visible_pixels"]) for x in allb(r)])
	b = scene(lambda d, ss: d.rectangle((90 * ss, 30 * ss, 104 * ss - 1, 46 * ss - 1), fill=(200, 40, 40)))
	for k in (3, 2):
		for ch in ("4", "1"):
			r = cmp(glyph("5", 40, 40, k), glyph(ch, 40, 40, k), max_shift=0)
			check("a changed digit (5 against %s, strokes %d px) stays visible: no one-pixel move makes it agree" % (ch, k),
				  r["status"] == "differ" and not any(x.get("shifted") for x in r["boxes"]),
				  [(box_tuple(x), x.get("shifted")) for x in allb(r)])
	# the differing-pixels picture: bright where a visible box's pixels differ visibly, pale for the rest
	r = cmp(a, b, max_shift=0)
	pm = r["pixmap"]
	check("differing pixels: the sprite in the bright colour, elsewhere gray", tuple(pm[38, 97]) == sd.PIXEL_VISIBLE
		  and pm[5, 5, 0] == pm[5, 5, 1] == pm[5, 5, 2], (pm[38, 97], pm[5, 5]))
	r = cmp(stripes(60, 40), stripes(61, 40), max_shift=0)
	pm = r["pixmap"]
	check("differing pixels: a faint difference in the pale colour only",
		  (pm == sd.PIXEL_FAINT).all(axis=2).any() and not (pm == sd.PIXEL_VISIBLE).all(axis=2).any())


def test_alignment():
	a = base()
	r = cmp(a, shifted(a, 2, 0))
	check("shifted 2 px: found, and it matches", r["shift"] == [2, 0] and r["status"] == "match" and r["boxes"] == [], r["shift"])
	r = cmp(a, shifted(a, -3, 1))
	check("shifted (-3,+1): found", r["shift"] == [-3, 1] and r["status"] == "match", r["shift"])
	r = cmp(a, shifted(a, 2, 0), max_shift=0)
	check("no search: the shift differs", r["shift"] == [0, 0] and r["status"] == "differ")
	r = cmp(a, shifted(a, 2, 0), align=(2, 0))
	check("fixed align: used without search", r["shift"] == [2, 0] and r["status"] == "match")
	r = cmp(a, shifted(a, 10, 0))  # 9 would be found as 8 and a rigid one-pixel shift: faint
	check("shift beyond the range is not found", r["shift"] != [10, 0] and r["status"] == "differ")
	r = cmp(a, change(shifted(a, 2, 0), 32, 40, 20, 10))
	check("shift plus a change: the box is in the original's coordinates",
		  r["shift"] == [2, 0] and [box_tuple(x) for x in r["boxes"]] == [(30, 40, 20, 10)], r["boxes"])
	flat = np.full((H, W, 3), 90, np.uint8)
	r = cmp(flat, change(flat, 40, 40, 30, 30))
	check("a flat picture is not shifted", r["shift"] == [0, 0], r["shift"])
	other = base(2)
	check("two unrelated pictures are not 'aligned' by chance", cmp(a, other)["shift"] == [0, 0])


def test_ignore():
	a = base()
	b = change(a, 30, 40, 20, 10)
	r = cmp(a, b, [(25, 35, 40, 30)])
	check("ignored: a change inside is not a difference", r["status"] == "match" and r["boxes"] == [] and r["diff_pixels"] == 0, r)
	check("ignored: the area leaves the denominator", r["compared_pixels"] == W * H - 40 * 30)
	r = cmp(a, b, [(30, 40, 20, 5)])
	check("ignored: a change half inside leaves the rest", [box_tuple(x) for x in r["boxes"]] == [(30, 45, 20, 5)], r["boxes"])
	r = cmp(a, change(a, 100, 10, 8, 8), [(30, 40, 20, 5)])
	check("ignored: elsewhere still differs", r["status"] == "differ")


def write_png(path, arr):
	os.makedirs(os.path.dirname(path), exist_ok=True)
	Image.fromarray(arr).save(path)


def framed(a, fill=(243, 243, 243)):
	"""The picture inside origrun's window frame (3 left, 26 top, 3 right, 3 bottom)."""
	f = np.zeros((H + 29, W + 6, 3), np.uint8)
	f[:26] = fill
	f[26:26 + H, 3:3 + W] = a
	return f


RECT = "rect.png (30, 40) 20x10"  # the review item of rect.png's one box


def test_files(tmp):
	a = base()
	orig, port = os.path.join(tmp, "orig"), os.path.join(tmp, "port")
	write_png(os.path.join(orig, "same.png"), a)
	write_png(os.path.join(port, "same.png"), a)
	write_png(os.path.join(orig, "rect.png"), a)
	write_png(os.path.join(port, "rect.png"), change(a, 30, 40, 20, 10))
	write_png(os.path.join(orig, "move.png"), a)
	write_png(os.path.join(port, "move.png"), shifted(a, 2, 0))
	write_png(os.path.join(orig, "bubbles.png"), a)
	write_png(os.path.join(port, "bubbles.png"), change(a, 100, 70, 20, 20))
	write_png(os.path.join(orig, "win.png"), framed(a))  # origrun's frame is cut away on its own
	write_png(os.path.join(port, "win.png"), a)
	write_png(os.path.join(orig, "sub/deep.png"), a)
	write_png(os.path.join(port, "sub/deep.png"), a)
	write_png(os.path.join(orig, "x&y's.png"), a)
	write_png(os.path.join(port, "x&y's.png"), change(a, 4, 4, 6, 6))
	write_png(os.path.join(orig, "a_name.png"), a)
	write_png(os.path.join(port, "other_name.png"), a)
	write_png(os.path.join(orig, "big.png"), np.zeros((100, 100, 3), np.uint8))
	write_png(os.path.join(port, "big.png"), np.zeros((100, 120, 3), np.uint8))
	write_png(os.path.join(orig, "only_orig.png"), a)
	write_png(os.path.join(port, "only_port.png"), a)
	os.makedirs(port, exist_ok=True)
	with open(os.path.join(orig, "broken.png"), "wb") as f:
		f.write(b"not a png")
	with open(os.path.join(port, "broken.png"), "wb") as f:
		f.write(b"not a png")
	cmap = os.path.join(tmp, "map.txt")
	with open(cmap, "w") as f:
		f.write("# names that differ\na_name = other_name\n")
	check("map: lines and JSON read alike", sd.read_map(cmap) == {"a_name": "other_name"})
	cfg = {"client": [W, H], "ignore": [{"rect": [95, 65, 30, 30], "pairs": "bubbles*", "label": "test bubbles"}]}
	root = os.path.join(tmp, "review")
	out = os.path.join(root, "demo", "run1")
	os.makedirs(out)
	s = sd.run_compare(orig, port, out, "demo", "run1", cfg, sd.read_map(cmap), sd.Options(), root=root)
	st = {p["name"]: p for p in s["pairs"]}
	check("run: statuses", {n: p["status"] for n, p in st.items()} == {
		"same.png": "match", "rect.png": "differ", "move.png": "match", "bubbles.png": "match", "win.png": "match",
		"sub/deep.png": "match", "x&y's.png": "differ", "a_name.png": "match", "big.png": "differ", "broken.png": "error"},
		{n: p["status"] for n, p in st.items()})
	c = s["counts"]
	check("run: counts", (c["pairs"], c["matching"], c["differing"], c["errors"], c["shifted"], c["orig_only"], c["port_only"]) ==
		  (10, 6, 3, 1, 1, 1, 1), c)
	check("run: shifted pair reports its shift", st["move.png"]["shift"] == [2, 0])
	check("run: the frame was cut", "frame" in (st["win.png"]["orig_note"] or ""), st["win.png"])
	check("run: sorted by size", [p["name"] for p in s["pairs"]][:3] == ["big.png", "rect.png", "x&y's.png"],
		  [p["name"] for p in s["pairs"]])
	check("run: size mismatch is a difference, not compared", st["big.png"].get("size_mismatch") and st["big.png"]["boxes"] == [])
	check("run: only-in lists", s["orig_only"] == ["only_orig.png"] and s["port_only"] == ["only_port.png"])
	check("run: the ignore list is in the summary", s["ignore"] == [{"rect": [95, 65, 30, 30], "label": "test bubbles", "pairs": "bubbles*"}])
	check("run: the ignored pair says what it ignored", st["bubbles.png"]["ignored"] == [{"rect": [95, 65, 30, 30], "label": "test bubbles"}])
	rb = st["rect.png"]["boxes"][0]
	check("run: box in the summary", box_tuple(rb) == (30, 40, 20, 10) and rb["pixels"] == 200, rb)
	crop_path = os.path.join(out, "crops", "rect.b1.png")
	check("run: crop file exists", os.path.isfile(crop_path))
	if os.path.isfile(crop_path):
		w, h = Image.open(crop_path).size
		# region 52x42 (box + 16 px margin, cut at x 14..66, y 24..66), enlarged 2x, three parts (original, port, differing
		# pixels) with 6 px gaps, a 14 px label strip
		check("run: crop is the three parts enlarged 2x", (w, h) == (52 * 2 * 3 + 12, 42 * 2 + 14) and rb["crop_scale"] == 2, (w, h, rb.get("crop_scale")))
	check("run: the crop path is openable from the repository or absolute", rb["crop"].endswith("crops/rect.b1.png"))
	check("run: pictures of differing pairs only", os.path.isfile(os.path.join(out, "img", "rect.orig.png"))
		  and not os.path.exists(os.path.join(out, "img", "same.orig.png")))
	with open(os.path.join(out, "summary.json"), encoding="utf-8") as f:
		check("run: summary.json is the summary", json.load(f)["counts"] == c)
	line = sd.summary_line(s)
	check("run: the stdout line", "demo/run1: 10 pairs, 6 matching, 3 differing" in line and line.endswith("index.html"), line)
	with open(os.path.join(out, "index.html"), encoding="utf-8") as f:
		page = f.read()
	check("page: names are escaped", "x&amp;y&#x27;s.png" in page and "x&y's.png" not in page)
	check("page: markup in a name cannot get through", "<script>alert" not in report.pair_html(
		0, dict(st["rect.png"], name="<script>alert(1)</script>", id="x")) and report.esc("<b>") == "&lt;b&gt;")
	art = page[page.index('id="p-item1"'):]
	check("page: an item's crop comes before the full pictures", 0 < art.index('class="crops"') < art.index('class="shots"'))
	check("page: the whole shots follow the items, linked to them", page.index('id="p-item1"') < page.index('id="s-rect"')
		  and 'href="#p-item1"' in page)
	check("page: the whole shots are collapsed, out of the review queue", '<details id="shots">' in page
		  and page.index('<details id="shots">') < page.index('id="s-rect"'))
	check("page: the differing-pixels picture", 'img/rect.pixels.png' in page and os.path.isfile(os.path.join(out, "img", "rect.pixels.png")))
	check("page: Issue, Not sure and Fine", all(t in page for t in ("data-mark=\"issue\"", "data-mark=\"unsure\"", "data-mark=\"fine\"", "Not sure")))
	check("page: a box states its size and differing pixels", "20×10 px at (30, 40)" in page and "200 of 200 px differ" in page)
	check("page: the ignore rectangles are listed", "Ignore rectangles in use" in page and "[95, 65, 30, 30]" in page and "test bubbles" in page)
	check("page: matching pairs in one collapsed list", "Matching pairs (6)" in page and "<details id=\"matching\">" in page)
	check("page: no CDN or remote URL", "http://" not in page.replace("http://127", "") and "https://" not in page and "//cdn" not in page)
	check("page: dark mode", "prefers-color-scheme: dark" in page)

	# the all-games page and the marks
	idx = os.path.join(root, "index.html")
	check("index: written with the run", os.path.isfile(idx) and "demo/run1/index.html" in open(idx, encoding="utf-8").read())
	marks_path = os.path.join(out, "marks.json")
	marks = {"game": "demo", "run": "run1", "updated": "2026-01-01T00:00:00Z", "marks": {
		RECT: {"mark": "issue", "note": "wrong", "sig": "x"}, "gone.png": {"mark": "fine", "note": ""}}}
	with open(marks_path, "w") as f:
		json.dump(marks, f)
	s2 = sd.run_compare(orig, port, out, "demo", "run1", cfg, sd.read_map(cmap), sd.Options(), root=root)
	with open(marks_path) as f:
		check("re-run of the same run keeps marks.json as it was", json.load(f) == marks)
	check("re-run: same result", s2["counts"] == c)
	out2 = os.path.join(root, "demo", "run2")
	os.makedirs(out2)
	write_png(os.path.join(port, "same.png"), change(a, 50, 50, 4, 4))  # a pair that began to differ
	s3 = sd.run_compare(orig, port, out2, "demo", "run2", cfg, sd.read_map(cmap), sd.Options(), root=root)
	m2 = json.load(open(os.path.join(out2, "marks.json")))
	check("new run: marks of pairs that still differ are carried over", m2["marks"].get(RECT, {}).get("mark") == "issue"
		  and m2["marks"][RECT].get("carried_from") == "run1", m2)
	check("new run: marks of pairs that no longer differ are not", "gone.png" not in m2["marks"])
	check("new run: the pair that began to differ has none", "same.png" not in m2["marks"])
	idx_text = open(idx, encoding="utf-8").read()
	check("index: the latest run is linked, the earlier one listed", "demo/run2/index.html" in idx_text and "earlier:" in idx_text
		  and "run1" in idx_text)
	# a second game with nothing marked sorts above one that is fully marked
	other = os.path.join(root, "zzz", "r")
	os.makedirs(other)
	sd.run_compare(orig, port, other, "zzz", "r", {"client": [W, H]}, None, sd.Options(), root=root)
	idx_text = open(idx, encoding="utf-8").read()
	check("index: unmarked differences first", idx_text.index("zzz/r/index.html") < idx_text.index("demo/run2/index.html"))
	check("index: both games listed with their counts", 'id="r-demo"' in idx_text and 'id="r-zzz"' in idx_text)

	# the page of a run with no difference, and --only
	same = os.path.join(root, "demo", "clean")
	os.makedirs(same)
	s4 = sd.run_compare(orig, port, same, "demo", "clean", cfg, None, sd.Options(), only=["same.png", "sub/*"], root=None)
	check("--only: only the matching names", sorted(p["name"] for p in s4["pairs"]) == ["same.png", "sub/deep.png"], s4["counts"])
	return root


def test_items(tmp):
	"""The same difference in several shots is one review item, dimmed or over other content too; a difference elsewhere is not."""
	a, other = base(), base(2)
	orig, port = os.path.join(tmp, "iorig"), os.path.join(tmp, "iport")
	for n in ("menu_a", "menu_b", "menu_c"):
		write_png(os.path.join(orig, n + ".png"), a)
	write_png(os.path.join(port, "menu_a.png"), change(a, 30, 40, 20, 10))
	write_png(os.path.join(port, "menu_b.png"), change(a, 30, 40, 20, 10))
	write_png(os.path.join(port, "menu_c.png"), change(change(a, 30, 40, 20, 10), 100, 70, 20, 20))
	write_png(os.path.join(orig, "other.png"), other)  # other content, the same difference, dimmed (a dialog over it)
	write_png(os.path.join(port, "other.png"), change(other, 30, 40, 20, 10))
	write_png(os.path.join(orig, "dim.png"), (other // 2).astype(np.uint8))
	write_png(os.path.join(port, "dim.png"), (change(other, 30, 40, 20, 10) // 2).astype(np.uint8))
	write_png(os.path.join(orig, "else.png"), a)
	write_png(os.path.join(port, "else.png"), change(a, 120, 90, 15, 15))
	root = os.path.join(tmp, "ireview")
	out = os.path.join(root, "demo", "items")
	os.makedirs(out)
	s = sd.run_compare(orig, port, out, "demo", "items", {"client": [W, H]}, None, sd.Options(), root=root)
	its = s["items"]
	check("items: one difference in five shots (over other content, dimmed) is one item, first",
		  [(it["count"], it["w"], it["h"]) for it in its] == [(5, 20, 10), (1, 20, 20), (1, 15, 15)]
		  and sorted([its[0]["pair"]] + [o["pair"] for o in its[0]["seen"]]) ==
		  ["dim.png", "menu_a.png", "menu_b.png", "menu_c.png", "other.png"], [(it["name"], it["count"]) for it in its])
	check("items: counted", s["counts"]["items"] == 3 and s["counts"]["items_repeated"] == 1 and s["counts"]["visible_boxes"] == 7, s["counts"])
	st = {p["name"]: p for p in s["pairs"]}
	check("items: each box knows its item", sorted(b["item"] for b in st["menu_c.png"]["boxes"]) == [1, 2]
		  and st["dim.png"]["boxes"][0]["item"] == 1 and st["else.png"]["boxes"][0]["item"] == 3)
	page = open(os.path.join(out, "index.html"), encoding="utf-8").read()
	check("items: the page says how often, and links the other shots", "seen in 5 shots" in page and 'href="#s-menu_b"' in page
		  and "A mark here marks them all" in page)
	check("items: what is marked is the item", sd.review_names(s) == [it["name"] for it in its])
	# the same difference behind a sprite that moves from shot to shot (drawn the same on both sides): still one item
	orig2, port2 = os.path.join(tmp, "iorig2"), os.path.join(tmp, "iport2")
	for k, sx in enumerate((32, 44, 56)):
		o = base()
		o[50:62, sx:sx + 10] = (240, 240, 20)
		pt = change(o, 30, 40, 40, 40)
		pt[50:62, sx:sx + 10] = (240, 240, 20)
		write_png(os.path.join(orig2, "fight_%d.png" % k), o)
		write_png(os.path.join(port2, "fight_%d.png" % k), pt)
	out2 = os.path.join(root, "demo", "items2")
	os.makedirs(out2)
	s = sd.run_compare(orig2, port2, out2, "demo", "items2", {"client": [W, H]}, None, sd.Options())
	check("items: a difference behind a moving sprite is one item", [it["count"] for it in s["items"]] == [3],
		  [(it["name"], it["count"]) for it in s["items"]])


def clip_frames(n, bg):
	"""A square that moves 5 px a frame over a textured background."""
	out = []
	for k in range(n):
		f = bg.copy()
		x = 8 + 5 * k
		f[50:66, x:x + 16] = (250, 250, 20)
		out.append(f)
	return out


def write_clip(folder, frames, interval=50, with_json=True):
	os.makedirs(folder, exist_ok=True)
	for k, f in enumerate(frames):
		Image.fromarray(f).save(os.path.join(folder, "%04d.png" % k))
	if with_json:
		with open(os.path.join(folder, "clip.json"), "w") as fh:
			json.dump({"interval_ms": interval}, fh)


def test_clips(tmp):
	import clip
	orig_dir, port_dir = os.path.join(tmp, "corig"), os.path.join(tmp, "cport")
	bg = base(5)
	o = clip_frames(24, bg)
	glitch = list(o)
	glitch[10] = bg.copy()
	glitch[10][10:30, 100:140] = (0, 255, 255)
	write_clip(os.path.join(orig_dir, "walk"), o)
	write_clip(os.path.join(port_dir, "walk"), o, with_json=False)  # the interval may be on either side
	write_clip(os.path.join(orig_dir, "skip"), o)
	write_clip(os.path.join(port_dir, "skip"), o[:8] + o[10:])
	write_clip(os.path.join(orig_dir, "glitch"), o)
	write_clip(os.path.join(port_dir, "glitch"), glitch)
	write_clip(os.path.join(orig_dir, "stall"), o)
	write_clip(os.path.join(port_dir, "stall"), o[:5] + [o[5]] * 9 + o[14:])
	write_clip(os.path.join(orig_dir, "lag"), o)
	write_clip(os.path.join(port_dir, "lag"), o[2:])
	write_clip(os.path.join(orig_dir, "moved"), o)
	write_clip(os.path.join(port_dir, "moved"), [shifted(f, 2, 0) for f in o])
	write_clip(os.path.join(orig_dir, "lonely"), o[:5])
	write_clip(os.path.join(orig_dir, "named_a"), o[:6])
	write_clip(os.path.join(port_dir, "named_b"), o[:6])
	write_png(os.path.join(orig_dir, "still.png"), bg)  # a still beside the clips
	write_png(os.path.join(port_dir, "still.png"), bg)
	cfg = {"client": [W, H]}
	out = os.path.join(tmp, "creview", "clipdemo", "r1")
	os.makedirs(out)
	s = sd.run_compare(orig_dir, port_dir, out, "clipdemo", "r1", cfg, {"named_a": "named_b"}, sd.Options(), root=os.path.join(tmp, "creview"))
	st = {p["name"]: p for p in s["pairs"]}
	check("clips: found beside the stills", set(st) == {"still.png", "walk/", "skip/", "glitch/", "stall/", "lag/", "moved/", "named_a/"}, sorted(st))
	check("clips: counts", s["counts"]["clips"] == 7 and s["counts"]["pairs"] == 8 and s["orig_only"] == ["lonely/"], s["counts"])
	w = st["walk/"]
	check("clip identical: matches at offset 0, interval from either side", w["status"] == "match" and w["offset"] == 0 and w["flagged"] == []
		  and w["interval_ms"] == 50 and w["drift"] == 0 and w["stalls"] == [], w)
	sk = st["skip/"]
	check("clip skipping frames: all matched, but it drifts", sk["status"] == "differ" and sk["flagged"] == [] and sk["drift"] == 2
		  and any(n.startswith("drifts") for n in sk["notes"]), sk["notes"])
	check("clip skipping frames: the frames after match the original's later ones", sk["shown"][7] == 7 and sk["shown"][8] == 10)
	g = st["glitch/"]
	check("clip glitch: only that frame is flagged, with boxes", g["status"] == "differ" and [f["frame"] for f in g["flagged"]] == [10]
		  and g["flagged"][0]["boxes"] and g["drift"] == 0 and g["stalls"] == [], g["flagged"])
	check("clip glitch: percentage of frames", abs(g["diff_pct"] - 100.0 / 24) < 0.01, g["diff_pct"])
	b = g["flagged"][0]["boxes"][0]
	check("clip glitch: crop of the flagged frame", os.path.isfile(os.path.join(out, "crops", "clip_glitch.f0010.b1.png")) and "crop" in b)
	sl = st["stall/"]
	check("clip stall: the port stalls and its frames past the window are flagged", [x["side"] for x in sl["stalls"]] == ["port"]
		  and (sl["stalls"][0]["from"], sl["stalls"][0]["to"]) == (5, 13) and [f["frame"] for f in sl["flagged"]] == [9, 10, 11, 12, 13]
		  and sl["status"] == "differ", (sl["stalls"], [f["frame"] for f in sl["flagged"]]))
	lg = st["lag/"]
	check("clip lagging by a constant: the offset is found, nothing drifts", lg["offset"] == 2 and lg["status"] == "match" and lg["drift"] == 0
		  and lg["offset_ms"] == 100 and lg["frames"] == 22, (lg["offset"], lg["status"], lg["drift"]))
	mv = st["moved/"]
	check("clip shifted 2 px: found once for the clip", mv["shift"] == [2, 0] and mv["status"] == "match", mv["shift"])
	check("clip: mapped folder names", st["named_a/"]["status"] == "match" and st["named_a/"]["frames"] == 6)
	check("clip: the numbered frames are not stills", "0000.png" not in st and not any(k.endswith("0000.png") for k in st))
	apng = os.path.join(out, "img", "clip_glitch.play.png")
	im = Image.open(apng)
	check("clip: an APNG of both sides in step", getattr(im, "is_animated", False) and im.n_frames >= 2 and im.size == (2 * W + 6, H), (im.size, getattr(im, "n_frames", 0)))
	check("clip: frames at full size", Image.open(os.path.join(out, "clips", "clip_glitch", "p0010.png")).size == (W, H)
		  and Image.open(os.path.join(out, "clips", "clip_glitch", "o0010.png")).size == (W, H))
	check("clip: APNG scale", clip.apng_scale(5, 160, 120) == 1.0 and clip.apng_scale(300, 640, 480) < 0.7 and clip.apng_scale(1, 2000, 100) < 1)
	with open(os.path.join(out, "index.html"), encoding="utf-8") as f:
		page = f.read()
	art = page[page.index('id="p-clip_glitch"'):]
	check("clip page: APNG, crops, scrubber with the flagged frame marked", art.index('class="play"') < art.index('class="crops"') < art.index('class="scrubber"')
		  and 'class="slider"' in art and 'title="frame 10 flagged"' in art and 'max="23"' in art)
	check("clip page: marks like a still", art.count('data-mark=') >= 3 and "Not sure" in art)
	check("clip page: drift and stall are written out", "drifts" in page and "stalls: the port shows the same picture in frames 5-13" in page)
	d = report.pair_data(g)
	check("clip page: the script's data", d["kind"] == "clip" and d["count"] == 1 and d["frames"] == 24 and d["flagged"][0]["frame"] == 10
		  and d["sig"].startswith("10:"), d)
	check("clip: summary_line counts clips", "8 pairs (7 clips)" in sd.summary_line(s), sd.summary_line(s))
	# a clip folder that is not a clip.json folder on the other side's name only; and a bad frame
	write_clip(os.path.join(orig_dir, "odd"), o[:4])
	write_clip(os.path.join(port_dir, "odd"), o[:3] + [bg[:100]])
	out2 = os.path.join(tmp, "creview", "clipdemo", "r2")
	os.makedirs(out2)
	s2 = sd.run_compare(orig_dir, port_dir, out2, "clipdemo", "r2", cfg, None, sd.Options(), only=["odd"], root=os.path.join(tmp, "creview"))
	check("clip: a frame of another size makes it an error, not a crash", [p["status"] for p in s2["pairs"]] == ["error"], s2["pairs"])


def request(port, method, path, body=None, headers=None):
	c = http.client.HTTPConnection("127.0.0.1", port, timeout=10)
	c.request(method, path, body=body, headers=headers or {})
	r = c.getresponse()
	data = r.read()
	c.close()
	return r.status, data


def test_server(root):
	serve.Handler.log_message = lambda self, fmt, *args: None
	srv = serve.make_server(root, 0)
	port = srv.server_address[1]
	t = threading.Thread(target=srv.serve_forever, daemon=True)
	t.start()
	try:
		doc = json.dumps({"game": "demo", "run": "run2", "updated": "t", "marks": {"rect.png": {"mark": "fine", "note": "ok"}}})
		js = {"Content-Type": "application/json"}
		code, _ = request(port, "POST", "/demo/run2/marks.json", doc, js)
		saved = json.load(open(os.path.join(root, "demo", "run2", "marks.json")))
		check("server: saves marks.json beside the page", code == 200 and saved["marks"]["rect.png"]["note"] == "ok", code)
		code, data = request(port, "GET", "/demo/run2/marks.json")
		check("server: serves it back", code == 200 and json.loads(data)["marks"]["rect.png"]["mark"] == "fine")
		code, data = request(port, "GET", "/")
		check("server: the root is the all-games page", code == 200 and b"All games" not in data and b"shotdiff all games" in data)
		code, data = request(port, "GET", "/demo/run2/index.html")
		check("server: serves a run page", code == 200 and b"Not sure" in data)
		c = http.client.HTTPConnection("127.0.0.1", port, timeout=10)
		c.request("GET", "/demo/run2/marks.json")
		r = c.getresponse()
		r.read()
		check("server: no caching", r.getheader("Cache-Control") == "no-store")
		c.close()
		for name, path, body, hdr, want in (
				("another file", "/demo/run2/summary.json", doc, js, 404),
				("a path outside", "/../marks.json", doc, js, 404),
				("encoded traversal", "/demo/%2e%2e/%2e%2e/marks.json", doc, js, 404),
				("a run that does not exist", "/demo/nope/marks.json", doc, js, 404),
				("not JSON", "/demo/run2/marks.json", "xx", js, 400),
				("no marks object", "/demo/run2/marks.json", "{\"a\": 1}", js, 400),
				("another site's origin", "/demo/run2/marks.json", doc, dict(js, Origin="http://evil.example"), 403),
				("another Host", "/demo/run2/marks.json", doc, dict(js, Host="evil.example"), 403)):
			code, _ = request(port, "POST", path, body, hdr)
			check("server: refuses " + name, code == want, code)
		code, _ = request(port, "POST", "/demo/run2/marks.json", doc, dict(js, Origin="http://127.0.0.1:%d" % port))
		check("server: accepts its own origin", code == 200)
		c = http.client.HTTPConnection("127.0.0.1", port, timeout=10)
		c.putrequest("POST", "/demo/run2/marks.json")
		c.putheader("Content-Length", str(5 << 20))
		c.endheaders()
		check("server: refuses a huge body", c.getresponse().status == 413)
		c.close()
		saved = json.load(open(os.path.join(root, "demo", "run2", "marks.json")))
		check("server: refusals left the file alone", saved["marks"]["rect.png"]["note"] == "ok")
	finally:
		srv.shutdown()
		srv.server_close()


def test_misc():
	check("slug", report.crop_rel("a", 2) == "crops/a.b2.png" and sd.slug("sub/x y.png") == "sub_x_y")
	check("pct text", report.pct_text(0.0434) == "0.043" and report.pct_text(12.34) == "12.3" and report.pct_text(0) == "0")


def main():
	test_compare()
	test_visible()
	test_alignment()
	test_ignore()
	test_misc()
	with tempfile.TemporaryDirectory(prefix="shotdiff-selftest-") as tmp:
		root = test_files(tmp)
		test_items(tmp)
		test_clips(tmp)
		test_server(root)
	print("shotdiff: %d checks, %d failed" % (checks, len(failures)))
	return 1 if failures else 0


if __name__ == "__main__":
	sys.exit(main())
