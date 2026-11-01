"""The review pages of shotdiff: one run's page and the all-games page. Static HTML with its CSS and script inline, so
it works offline and as a plain file; the pictures are the files beside it."""
import html
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))


def img_rel(pid, side):
	return "img/%s.%s.png" % (pid, side)


def crop_rel(pid, n):
	return "crops/%s.b%d.png" % (pid, n)


def frame_crop_rel(pid, frame, n):
	return "crops/%s.f%04d.b%d.png" % (pid, frame, n)


def play_rel(pid):
	return "img/%s.play.png" % pid  # an APNG; browsers play it as they do any PNG


def asset(name):
	with open(os.path.join(HERE, name), encoding="utf-8") as f:
		return f.read()


def esc(s):
	return html.escape(str(s), quote=True)


def script_json(obj):
	return json.dumps(obj).replace("<", "\\u003c").replace(">", "\\u003e").replace("&", "\\u0026")


def pct_text(p):
	return ("%.3f" % p).rstrip("0").rstrip(".") if p < 10 else "%.1f" % p


def plural(n, one, many=None):
	return "%d %s" % (n, one if n == 1 else (many or one + "s"))


def sized(r):
	return "%d×%d" % (r[2], r[3])


def page(title, body, scripts, extra_css=""):
	return ("<!DOCTYPE html>\n<html lang=\"en\"><head><meta charset=\"utf-8\">\n"
			"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
			"<title>%s</title>\n<style>\n%s%s</style></head>\n<body>\n%s\n%s</body></html>\n") % (
		esc(title), asset("page.css"), extra_css, body, scripts)


# ---------------------------------------------------------------- one run

def ignore_section(s):
	ig = s["ignore"]
	if not ig:
		return ("<section class=\"ignores\"><h2>Ignore rectangles in use</h2><p>None. Every pixel of every pair is compared; "
				"nothing is hidden.</p></section>")
	items = []
	for e in ig:
		where = "" if not e["pairs"] else " in pairs matching <code>%s</code>" % esc(e["pairs"])
		items.append("<li><code>[%d, %d, %d, %d]</code> (x, y, width, height)%s%s</li>" % (
			e["rect"][0], e["rect"][1], e["rect"][2], e["rect"][3], where, " &ndash; " + esc(e["label"]) if e["label"] else ""))
	return ("<section class=\"ignores\"><h2>Ignore rectangles in use</h2><p>These areas of the game's client are not compared; "
			"they are the only places where a difference can go unseen. Dashed blue outlines in the full pictures.</p>"
			"<ul>%s</ul></section>") % "".join(items)


def box_overlay(cls, r, size, label=None):
	return "<span class=\"%s\" style=\"left:%.4f%%;top:%.4f%%;width:%.4f%%;height:%.4f%%\">%s</span>" % (
		cls, 100.0 * r[0] / size[0], 100.0 * r[1] / size[1], 100.0 * r[2] / size[0], 100.0 * r[3] / size[1],
		"<i>%s</i>" % label if label is not None else "")


def faint_section(s):
	"""The faint differences: collapsed, after everything to review, with each box's measurements."""
	rows = []
	for p in s["pairs"]:
		fb = p.get("faint_boxes") or []
		frames = p.get("faint_frames") or []
		if p.get("kind") == "clip" and frames:
			rows.append("<tr><td><code>%s</code></td><td colspan=\"5\">%s agree with the original only faintly: %s</td></tr>" % (
				esc(p["name"]), plural(len(frames), "frame"), esc(", ".join(str(f) for f in frames[:40]) + (", ..." if len(frames) > 40 else ""))))
		for b in fb:
			rows.append("<tr><td><code>%s</code></td><td>%s at (%d, %d)</td><td class=\"n\">%s</td><td class=\"n\">%d</td>"
						"<td class=\"n\">%s</td><td class=\"n\">%d</td></tr>" % (
							esc(p["name"]), sized((0, 0, b["w"], b["h"])), b["x"], b["y"], "{:,}".format(b["pixels"]), b["max_delta"],
							"{:,}".format(b.get("strong_pixels", 0)), b.get("visible_pixels", 0)))
	if not rows:
		return ""
	v = s.get("visible", {})
	return ("<details id=\"faint\"><summary>Faint differences (%s, not for review)</summary><p class=\"muted\">Pixels that differ but that no "
			"one can see: after a %s blur of both pictures they differ by less than %d%s, or too few of "
			"them do (under %d in a box). Antialiased edges and sub-pixel sampling end up here. They are kept so a session can still "
			"look at every difference.</p><table class=\"games\"><thead><tr><th>Pair</th><th>Box</th><th class=\"n\">Differing px</th>"
			"<th class=\"n\">Largest channel difference</th><th class=\"n\">px off by %d or more</th><th class=\"n\">Visible px</th></tr></thead>"
			"<tbody>%s</tbody></table></details>") % (
				plural(sum(len(p.get("faint_boxes") or []) for p in s["pairs"]), "box", "boxes"), esc(v.get("blur", "3x3 box")),
				v.get("delta", 32), " against the port shifted up to %d px" % v["shift"] if v.get("shift") else "", v.get("min_pixels", 32), 32,
				"".join(rows))


def pair_html(i, p):
	size = p["size"]
	boxes = p["boxes"]
	meta = []
	if p.get("size_mismatch"):
		meta.append("not compared")
	else:
		meta.append(plural(len(boxes), "visible box", "visible boxes"))
		if p.get("faint_boxes"):
			meta.append("%s listed at the end" % plural(len(p["faint_boxes"]), "faint box", "faint boxes"))
		meta.append("%s of %s px differ" % ("{:,}".format(p["diff_pixels"]), "{:,}".format(p["compared_pixels"])))
	meta.append("%d×%d" % (size[0], size[1]))
	if p["shift"] != [0, 0]:
		meta.append("<span class=\"tag\">port lined up by (%+d, %+d) px</span>" % tuple(p["shift"]))
	head = "<header class=\"phead\"><h2>%s</h2><span class=\"pct\">%s%% differs</span><span class=\"meta\">%s</span></header>" % (
		esc(p["name"]), pct_text(p["diff_pct"]), " &middot; ".join(meta))
	notes = []
	if p.get("note"):
		notes.append(esc(p["note"]))
	if p.get("orig_note"):
		notes.append("original: " + esc(p["orig_note"]))
	if p.get("port_note"):
		notes.append("port: " + esc(p["port_note"]))
	if p["ignored"]:
		notes.append("ignored here: " + ", ".join("<code>[%d, %d, %d, %d]</code>%s" % (
			e["rect"][0], e["rect"][1], e["rect"][2], e["rect"][3], " " + esc(e["label"]) if e["label"] else "") for e in p["ignored"]))
	notes_html = "<p class=\"meta\">%s</p>" % "; ".join(notes) if notes else ""
	name = esc(p["name"])
	verdict = verdict_html(name)
	crops = []
	for b in boxes:
		cap = "<b>Box %d</b> &middot; %s px at (%d, %d) &middot; %s of %s px differ &middot; largest channel difference %d" % (
			b["n"], sized((0, 0, b["w"], b["h"])), b["x"], b["y"], "{:,}".format(b["pixels"]), "{:,}".format(b["w"] * b["h"]),
			b["max_delta"])
		rel = crop_rel(p["id"], b["n"])
		crops.append("<figure class=\"crop\"><figcaption>%s</figcaption><img src=\"%s\" width=\"%d\" height=\"%d\" loading=\"lazy\" "
					 "alt=\"Box %d of %s: original on the left, port on the right\"></figure>" % (
						 cap, esc(rel), b["crop_size"][0], b["crop_size"][1], b["n"], name))
	crops_html = "<div class=\"crops\">%s</div>" % "".join(crops) if crops else ""
	ims = p["images"]
	marked = ""
	if not p.get("size_mismatch"):
		marked = "".join(box_overlay("box", (b["x"], b["y"], b["w"], b["h"]), size, b["n"]) for b in boxes)
		marked += "".join(box_overlay("ign", e["rect"], size) for e in p["ignored"])
	shots = (
		"<div class=\"shots\">"
		"<figure><figcaption>Original (click for full size, then 1 2 3)</figcaption><div class=\"stack\" data-view=\"orig\"><img src=\"%s\" alt=\"Original, %s\" loading=\"lazy\"></div></figure>"
		"<figure><figcaption>Port</figcaption><div class=\"stack\" data-view=\"port\"><img src=\"%s\" alt=\"Port, %s\" loading=\"lazy\"></div></figure>"
		"<figure><figcaption>Port with the differences outlined</figcaption><div class=\"stack\" data-view=\"marked\"><img src=\"%s\" alt=\"Port with the differences outlined, %s\" loading=\"lazy\">%s</div></figure>"
		"</div>") % (esc(ims["orig"]), name, esc(ims["port"]), name, esc(ims["port"]), name, marked)
	return "<article class=\"pair\" id=\"p-%s\" data-i=\"%d\">%s%s%s%s%s</article>" % (
		esc(p["id"]), i, head, notes_html, verdict, crops_html, shots)


def pair_data(p):
	"""What the page's script needs of a differing pair."""
	d = {"id": p["id"], "name": p["name"], "kind": p["kind"], "pct": p["diff_pct"], "size": p["size"],
		 "size_mismatch": bool(p.get("size_mismatch")), "ignored": [e["rect"] for e in p["ignored"]]}
	if p["kind"] == "clip":
		d.update(frames=p["frames"], shown=p["shown"], dir=p.get("images", {}).get("dir", ""), count=len(p["flagged"]),
				 sig="%s|d%s|%s" % (";".join("%d:%d" % (f["frame"], f["diff_pixels"]) for f in p["flagged"]), p["drift"],
									",".join("%s%d-%d" % (x["side"][0], x["from"], x["to"]) for x in p["stalls"])),
				 flagged=[{"frame": f["frame"], "boxes": [{k: b[k] for k in ("n", "x", "y", "w", "h", "pixels")} for b in f["boxes"]]}
						  for f in p["flagged"]])
	else:
		d.update(images=p["images"], count=len(p["boxes"]), sig=still_sig(p),
				 boxes=[{k: b[k] for k in ("n", "x", "y", "w", "h", "pixels")} for b in p["boxes"]])
	return d


def clip_html(i, p):
	n = p["frames"]
	flagged = p["flagged"]
	name = esc(p["name"])
	meta = ["%s of %s port frames flagged" % ("{:,}".format(len(flagged)), "{:,}".format(n)),
			"%d original frames" % p["orig_frames"], "%d\u00d7%d" % tuple(p["size"]), "%d ms apart" % p["interval_ms"]]
	if p["shift"] != [0, 0]:
		meta.append("<span class=\"tag\">port lined up by (%+d, %+d) px</span>" % tuple(p["shift"]))
	head = "<header class=\"phead\"><h2>%s <span class=\"tag\">clip</span></h2><span class=\"pct\">%s</span><span class=\"meta\">%s</span></header>" % (
		name, "sequence differs" if not flagged else "%s%% of frames flagged" % pct_text(p["diff_pct"]), " &middot; ".join(meta))
	notes = []
	if p.get("note"):
		notes.append(esc(p["note"]))
	notes.extend(esc(x) for x in p["notes"])
	if p.get("orig_note"):
		notes.append("original: " + esc(p["orig_note"]))
	if p.get("port_note"):
		notes.append("port: " + esc(p["port_note"]))
	notes.extend(esc(x) for x in p.get("warnings", []))
	if p["ignored"]:
		notes.append("ignored here: " + ", ".join("<code>[%d, %d, %d, %d]</code>%s" % (
			e["rect"][0], e["rect"][1], e["rect"][2], e["rect"][3], " " + esc(e["label"]) if e["label"] else "") for e in p["ignored"]))
	notes_html = "<ul class=\"notes\">%s</ul>" % "".join("<li>%s</li>" % x for x in notes) if notes else ""
	crops = []
	worst = sorted((f for f in flagged if any("crop" in b for b in f["boxes"])), key=lambda f: -f["diff_pixels"])[:12]
	for f in sorted(worst, key=lambda f: f["frame"]):
		for b in f["boxes"]:
			if "crop" not in b:
				continue
			cap = "<b>Frame %d</b> (original frame %d) &middot; box %d: %d\u00d7%d px at (%d, %d) &middot; %s of %s px differ &middot; largest channel difference %d" % (
				f["frame"], f["orig_frame"], b["n"], b["w"], b["h"], b["x"], b["y"], "{:,}".format(b["pixels"]),
				"{:,}".format(b["w"] * b["h"]), b["max_delta"])
			crops.append("<figure class=\"crop\"><figcaption>%s</figcaption><img src=\"%s\" width=\"%d\" height=\"%d\" loading=\"lazy\" "
						 "alt=\"Frame %d box %d: original on the left, port on the right\"></figure>" % (
							 cap, esc(frame_crop_rel(p["id"], f["frame"], b["n"])), b["crop_size"][0], b["crop_size"][1], f["frame"], b["n"]))
	if crops:
		crops_html = "<p class=\"muted\">The %s worst flagged frame%s of %d, original on the left, port on the right; step through the others below.</p><div class=\"crops\">%s</div>" % (
			len(worst), "" if len(worst) == 1 else "s", len(flagged), "".join(crops))
	else:
		crops_html = ""
	verdict = verdict_html(name)
	if p.get("size_mismatch"):
		return "<article class=\"pair\" id=\"p-%s\" data-i=\"%d\">%s%s%s</article>" % (esc(p["id"]), i, head, notes_html, verdict)
	play = ("<figure class=\"play\"><figcaption>Original (left) and port (right) in step, %d ms a frame%s. A port frame no original frame "
			"agrees with has a magenta border.</figcaption><img src=\"%s\" alt=\"%s: original and port side by side, playing\"></figure>") % (
		p["interval_ms"], "" if p["apng_scale"] >= 1 else ", scaled to %d%% to keep the file small (the frames below are full size)" % round(100 * p["apng_scale"]),
		esc(p["images"]["play"]), name)
	k0 = flagged[0]["frame"] if flagged else 0
	ticks = "".join("<i style=\"left:%.3f%%\" title=\"frame %d flagged\"></i>" % (100.0 * f["frame"] / max(1, n - 1), f["frame"]) for f in flagged)
	d = p["images"]["dir"]
	j0 = p["shown"][k0]
	scrub = (
		"<div class=\"scrubber\"><div class=\"ticks\" aria-hidden=\"true\">%s</div>"
		"<input class=\"slider\" type=\"range\" min=\"0\" max=\"%d\" value=\"%d\" aria-label=\"Frame of %s\">"
		"<p class=\"framelabel\" aria-live=\"polite\">Frame %d of %d (port)</p>"
		"<div class=\"shots two\">"
		"<figure><figcaption>Original (click for full size, then 1 2 3, \u2190 \u2192 step)</figcaption><div class=\"stack\" data-view=\"orig\"><img src=\"%s\" alt=\"Original frame\" loading=\"lazy\"></div></figure>"
		"<figure><figcaption>Port, flagged differences outlined</figcaption><div class=\"stack\" data-view=\"marked\"><img src=\"%s\" alt=\"Port frame\" loading=\"lazy\"></div></figure>"
		"</div></div>") % (ticks, max(0, n - 1), k0, name, k0, n - 1,
						   esc("%s/o%04d.png" % (d, j0)) if j0 is not None else "", esc("%s/p%04d.png" % (d, k0)))
	return "<article class=\"pair clip\" id=\"p-%s\" data-i=\"%d\">%s%s%s%s%s%s</article>" % (
		esc(p["id"]), i, head, notes_html, verdict, play, crops_html, scrub)


def verdict_html(name):
	return (
		"<div class=\"verdict\" role=\"group\" aria-label=\"Your verdict for %s\">"
		"<button type=\"button\" class=\"b-issue\" data-mark=\"issue\" aria-pressed=\"false\">Issue <kbd>i</kbd></button>"
		"<button type=\"button\" class=\"b-unsure\" data-mark=\"unsure\" aria-pressed=\"false\">Not sure <kbd>u</kbd></button>"
		"<button type=\"button\" class=\"b-fine\" data-mark=\"fine\" aria-pressed=\"false\">Fine <kbd>f</kbd></button>"
		"<input class=\"note\" type=\"text\" placeholder=\"Note (n)\" aria-label=\"Note for %s\" autocomplete=\"off\">"
		"<span class=\"changed\" hidden>The differences changed since you marked this.</span></div>") % (name, name)


def still_sig(p):
	return ";".join("%d,%d,%d,%d,%d" % (b["x"], b["y"], b["w"], b["h"], b["pixels"]) for b in p["boxes"])


def render(summary, has_index=False):
	"""The run page. has_index: the folder above the game's holds the all-games page, so link to it."""
	s = summary
	c = s["counts"]
	differing = [p for p in s["pairs"] if p["status"] == "differ"]
	matching = [p for p in s["pairs"] if p["status"] == "match"]
	errors = [p for p in s["pairs"] if p["status"] == "error"]
	data = {"game": s["game"], "run": s["run"], "pairs": [pair_data(p) for p in differing]}

	b = []
	if has_index:
		b.append("<p><a href=\"../../index.html\">&larr; All games</a></p>")
	b.append("<h1>%s / %s</h1>" % (esc(s["game"]), esc(s["run"])))
	line = "%s%s: %d matching, %d differing visibly, %d only faintly" % (
		plural(c["pairs"], "pair"), " (%s)" % plural(c["clips"], "clip") if c.get("clips") else "", c["matching"], c["differing"], c.get("faint", 0))
	if c["errors"]:
		line += ", %s could not be compared" % plural(c["errors"], "pair")
	if c["orig_only"] or c["port_only"]:
		line += "; unpaired: %d only in the original, %d only in the port" % (c["orig_only"], c["port_only"])
	b.append("<p class=\"big\">%s</p>" % esc(line))
	settings = ["A pixel differs when a colour channel differs by more than %d." % s["tolerance"]]
	if s.get("visible"):
		v = s["visible"]
		settings.append("Only visible differences are for review: a box needs %d or more pixels that still differ by %d after a %s blur "
						"of both pictures%s; the rest are faint, listed at the end." % (
							v["min_pixels"], v["delta"], v["blur"], ", against the port shifted up to %d px" % v["shift"] if v["shift"] else ""))
	if s.get("align"):
		settings.append("The port is compared at a fixed offset of (%d, %d) px." % tuple(s["align"]))
	elif s["max_shift"] > 0:
		settings.append("The port is lined up with the original by searching shifts up to %d px; a pair that needed one says so." % s["max_shift"])
	else:
		settings.append("No alignment search: the pictures are compared as they are.")
	if s.get("allow"):
		settings.append("A pair counts as matching with up to %d differing pixels (--allow)." % s["allow"])
	if c["shifted"]:
		settings.append("%s needed a shift." % plural(c["shifted"], "pair"))
	settings.append("Run %s." % esc(s["generated"]))
	b.append("<p class=\"settings\">%s Original: <code>%s</code>. Port: <code>%s</code>.</p>" % (
		" ".join(settings), esc(s["orig_dir"]), esc(s["port_dir"])))
	b.append(ignore_section(s))
	b.append("<noscript><p>Marks need JavaScript; the pictures and boxes below do not.</p></noscript>")
	bar = ("<div class=\"bar\"><span class=\"tally\"><span id=\"t-issue\" class=\"t-issue\"></span><span id=\"t-unsure\" class=\"t-unsure\"></span>"
		   "<span id=\"t-fine\" class=\"t-fine\"></span><span id=\"t-open\"></span></span>"
		   "<button type=\"button\" id=\"copy\">Copy marks</button><span id=\"savestatus\" role=\"status\"></span></div>")
	b.append("<p class=\"keys\"><kbd>j</kbd>/<kbd>k</kbd> next/previous pair &middot; <kbd>i</kbd> issue &middot; <kbd>u</kbd> not sure &middot; "
			 "<kbd>f</kbd> fine &middot; <kbd>n</kbd> note &middot; <kbd>z</kbd> full size &middot; clips: <kbd>\u2190</kbd>/<kbd>\u2192</kbd> step a frame, "
			 "<kbd>[</kbd>/<kbd>]</kbd> previous/next flagged frame</p>")
	b.append(bar)
	b.append("<details id=\"copybox\"><summary>Marks as text</summary><textarea id=\"copytext\" readonly></textarea></details>")
	if differing:
		b.append("<h2 style=\"margin-top:16px\">Differing pairs, largest first, clips last (%d)</h2>" % len(differing))
		b.append("<p class=\"muted\">Each box is shown with the original on the left and the port on the right. The full pictures are below.</p>")
		b.extend(clip_html(i, p) if p["kind"] == "clip" else pair_html(i, p) for i, p in enumerate(differing))
	else:
		b.append("<p>No pair differs.</p>")
	b.append(faint_section(s))
	if errors:
		b.append("<details open><summary>Could not be compared (%d)</summary><ul>%s</ul></details>" % (
			len(errors), "".join("<li><code>%s</code>: %s</li>" % (esc(p["name"]), esc(p.get("note", ""))) for p in errors)))
	if matching:
		items = []
		for p in matching:
			extra = []
			if p["diff_pixels"]:
				extra.append("%d px differ (allowed)" % p["diff_pixels"])
			if p["shift"] != [0, 0]:
				extra.append("<span class=\"tag\">lined up by (%+d, %+d) px</span>" % tuple(p["shift"]))
			items.append("<li>%s%s</li>" % (esc(p["name"]), " <small>" + ", ".join(extra) + "</small>" if extra else ""))
		b.append("<details id=\"matching\"><summary>Matching pairs (%d)</summary><ul class=\"names\">%s</ul></details>" % (len(matching), "".join(items)))
	for key, label in (("orig_only", "Only in the original folder"), ("port_only", "Only in the port folder")):
		if s[key]:
			b.append("<details><summary>%s (%d)</summary><ul class=\"names\">%s</ul></details>" % (
				label, len(s[key]), "".join("<li>%s</li>" % esc(n) for n in s[key])))
	if s["warnings"]:
		b.append("<details><summary>Warnings (%d)</summary><ul>%s</ul></details>" % (len(s["warnings"]), "".join("<li>%s</li>" % esc(w) for w in s["warnings"])))
	zoom = ("<div id=\"zoom\" hidden role=\"dialog\" aria-modal=\"true\" aria-label=\"Full-size picture\"><div class=\"zbar\"><span id=\"zlabel\"></span>"
			" &middot; <kbd>1</kbd> original <kbd>2</kbd> port <kbd>3</kbd> outlined &middot; <kbd>Esc</kbd> or a click closes</div>"
			"<div class=\"zscroll\"><div class=\"stack\" id=\"zstack\"></div></div></div>")
	body = "<main>%s</main>\n%s" % ("\n".join(b), zoom)
	scripts = "<script type=\"application/json\" id=\"shotdiff-data\">%s</script>\n<script>\n%s\n%s</script>\n" % (
		script_json(data), asset("common.js"), asset("page.js"))
	return page("shotdiff %s/%s" % (s["game"], s["run"]), body, scripts)


# ---------------------------------------------------------------- all games

def render_index(entries):
	"""entries: [{game, run, generated, counts, names (differing pairs), marks {issue, unsure, fine}, earlier [runs]}]."""
	rows = []
	for e in entries:
		c = e["counts"]
		marked = sum(e["marks"].values())
		openn = len(e["names"]) - marked
		earlier = ""
		if e["earlier"]:
			earlier = "<br><small>earlier: %s</small>" % ", ".join("<a href=\"%s/%s/index.html\">%s</a>" % (esc(e["game"]), esc(r), esc(r)) for r in e["earlier"])
		m = e["marks"]
		rows.append(
			"<tr id=\"r-%s\"%s><td><a href=\"%s/%s/index.html\">%s</a><br><small>run %s, %s</small>%s</td><td class=\"n\">%d</td><td class=\"n\">%d</td>"
			"<td class=\"n\">%d%s</td><td class=\"c-marked\">%s</td><td class=\"n c-open%s\">%d</td></tr>" % (
				esc(e["game"]), "" if openn else " class=\"done\"", esc(e["game"]), esc(e["run"]), esc(e["game"]), esc(e["run"]),
				esc(e["generated"]), earlier, c["pairs"], c["matching"], c["differing"],
				" <small>(+%d unpairable)</small>" % c["errors"] if c["errors"] else "",
				"%d (%d issue, %d not sure, %d fine)" % (marked, m["issue"], m["unsure"], m["fine"]) if marked else "0",
				" big" if openn else "", openn))
	if rows:
		table = ("<table class=\"games\"><thead><tr><th>Game</th><th class=\"n\">Pairs</th><th class=\"n\">Matching</th><th class=\"n\">Differing</th>"
				 "<th>Already marked</th><th class=\"n\">Not yet marked</th></tr></thead><tbody id=\"rows\">%s</tbody></table>") % "".join(rows)
	else:
		table = "<p>No runs yet. Run <code>python tools/shotdiff/shotdiff.py --game &lt;game&gt; --orig &lt;folder&gt; --port &lt;folder&gt;</code>.</p>"
	data = {"runs": [{"game": e["game"], "run": e["run"], "names": e["names"], "marks": e["marks"]} for e in entries]}
	body = ("<main><h1>Screenshots: original against port</h1>"
			"<p id=\"total\" class=\"big\"></p>"
			"<p class=\"muted\">The latest run of each game. Games with differences you have not marked yet come first. The numbers of "
			"marks are read again when this page opens; the others are from the last run.</p>%s</main>") % table
	scripts = "<script type=\"application/json\" id=\"shotdiff-data\">%s</script>\n<script>\n%s\n%s</script>\n" % (
		script_json(data), asset("common.js"), asset("index.js"))
	return page("shotdiff all games", body, scripts)
