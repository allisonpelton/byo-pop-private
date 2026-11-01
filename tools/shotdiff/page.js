/* The run page: marks (Issue / Not sure / Fine and a note), the keyboard, the 1:1 zoom, the clips' frame scrubber and
   "Copy marks". */
(function () {
	'use strict';
	var DATA = JSON.parse(document.getElementById('shotdiff-data').textContent);
	var pairs = DATA.pairs; // what is reviewed, in page order: the stills' items, then pairs reviewed whole (clips last)
	var doc = { game: DATA.game, run: DATA.run, updated: '', marks: {} };
	var loaded = false, cur = -1, saveTimer = 0, zoomState = null;
	var clipFrame = {}; // pair index -> the frame its scrubber shows
	var LABEL = { issue: 'ISSUE', unsure: 'NOT SURE', fine: 'FINE' };
	var VIEWS = { orig: 'original', port: 'port', marked: 'port with the differences outlined', pixels: 'differing pixels' };

	function $(id) { return document.getElementById(id); }
	function article(i) { return $('p-' + pairs[i].id); }
	function entry(p) { return doc.marks[p.name]; }
	function markOf(p) { var e = entry(p); return e && e.mark ? e.mark : ''; }
	function isClip(p) { return p.kind === 'clip'; }

	function say(text) { $('savestatus').textContent = text; }

	function persist() {
		doc.updated = new Date().toISOString();
		clearTimeout(saveTimer);
		ShotMarks.save(DATA.game, DATA.run, 'marks.json', doc, function (ok) {
			if (!ShotMarks.served) {
				say('Opened as a file: marks stay in this browser. Use Copy marks to pass them on, or serve the folder with tools/shotdiff/serve.py to save marks.json.');
			} else if (ok) {
				say('Saved to marks.json at ' + new Date().toLocaleTimeString() + '.');
			} else {
				say('Could not reach the server: marks are kept in this browser only. Use Copy marks, or start tools/shotdiff/serve.py.');
			}
		});
	}
	function persistSoon() { clearTimeout(saveTimer); saveTimer = setTimeout(persist, 400); }

	function ensure(p) {
		var e = doc.marks[p.name];
		if (!e) e = doc.marks[p.name] = { mark: '', note: '', pct: p.pct, boxes: p.count, sig: p.sig, at: '' };
		return e;
	}
	function dropIfEmpty(p) {
		var e = entry(p);
		if (e && !e.mark && !e.note) delete doc.marks[p.name];
	}

	function renderPair(i) {
		var p = pairs[i], a = article(i), m = markOf(p), e = entry(p);
		a.classList.remove('m-issue', 'm-fine', 'm-unsure');
		if (m) a.classList.add('m-' + m);
		['issue', 'unsure', 'fine'].forEach(function (k) {
			a.querySelector('.b-' + k).setAttribute('aria-pressed', m === k ? 'true' : 'false');
		});
		var note = a.querySelector('.note');
		if (document.activeElement !== note) note.value = e ? e.note : '';
		a.querySelector('.changed').hidden = !(e && e.mark && e.sig && e.sig !== p.sig);
	}

	function tally() {
		var n = { issue: 0, unsure: 0, fine: 0 };
		pairs.forEach(function (p) { var m = markOf(p); if (m) n[m]++; });
		var marked = n.issue + n.unsure + n.fine;
		$('t-issue').textContent = 'Issue ' + n.issue;
		$('t-unsure').textContent = 'Not sure ' + n.unsure;
		$('t-fine').textContent = 'Fine ' + n.fine;
		$('t-open').textContent = 'Unmarked ' + (pairs.length - marked) + ' of ' + pairs.length;
	}
	function renderAll() { for (var i = 0; i < pairs.length; i++) renderPair(i); tally(); }

	function setMark(i, kind) {
		if (!loaded || i < 0) return;
		var p = pairs[i], e = ensure(p);
		e.mark = e.mark === kind ? '' : kind;
		e.sig = p.sig; e.pct = p.pct; e.boxes = p.count;
		e.at = new Date().toISOString();
		dropIfEmpty(p);
		renderPair(i); tally(); persist();
	}

	function setCurrent(i, scroll) {
		if (i < 0 || i >= pairs.length) return;
		if (cur >= 0) article(cur).classList.remove('current');
		cur = i;
		var a = article(i);
		a.classList.add('current');
		if (scroll) a.scrollIntoView({ block: 'start' });
	}
	function move(d) { setCurrent(Math.max(0, Math.min(pairs.length - 1, (cur < 0 ? (d > 0 ? -1 : pairs.length) : cur) + d)), true); }

	/* ---- boxes drawn over a picture, as percentages so they follow its size */
	function box(cls, r, size, text) {
		var s = document.createElement('span');
		s.className = cls;
		s.style.left = (100 * r[0] / size[0]) + '%'; s.style.top = (100 * r[1] / size[1]) + '%';
		s.style.width = (100 * r[2] / size[0]) + '%'; s.style.height = (100 * r[3] / size[1]) + '%';
		if (text) { var i = document.createElement('i'); i.textContent = text; s.appendChild(i); }
		return s;
	}
	function addOverlays(stack, p, withBoxes, k) {
		Array.prototype.slice.call(stack.querySelectorAll('.box, .ign')).forEach(function (e) { e.remove(); });
		if (p.size_mismatch) return;
		if (withBoxes) {
			var boxes = isClip(p) ? (flaggedAt(p, k) || { boxes: [] }).boxes : p.boxes;
			boxes.forEach(function (b) { stack.appendChild(box('box', [b.x, b.y, b.w, b.h], p.size, String(b.n))); });
		}
		p.ignored.forEach(function (r) { stack.appendChild(box('ign', r, p.size)); });
	}

	/* ---- clips: the frame scrubber */
	function pad4(k) { return ('0000' + k).slice(-4); }
	function flaggedAt(p, k) {
		for (var x = 0; x < p.flagged.length; x++) if (p.flagged[x].frame === k) return p.flagged[x];
		return null;
	}
	function frameSrc(p, side, k) {
		if (side === 'orig') { var j = p.shown[k]; return j === null || j === undefined ? '' : p.dir + '/o' + pad4(j) + '.png'; }
		return p.dir + '/p' + pad4(k) + '.png';
	}
	function showFrame(i, k) {
		var p = pairs[i], a = article(i);
		k = Math.max(0, Math.min(p.frames - 1, k));
		clipFrame[i] = k;
		a.querySelector('.slider').value = String(k);
		var f = flaggedAt(p, k), j = p.shown[k];
		var text = 'Frame ' + k + ' of ' + (p.frames - 1) + ' (port)';
		text += j === null || j === undefined ? ' · no original frame to compare'
			: ' · original frame ' + j + (j !== k ? ' (' + (j - k > 0 ? '+' : '') + (j - k) + ')' : '');
		text += f ? ' · FLAGGED, ' + f.boxes.length + (f.boxes.length === 1 ? ' box' : ' boxes') : ' · matches';
		var label = a.querySelector('.framelabel');
		label.textContent = text;
		label.classList.toggle('flag', !!f);
		['orig', 'marked'].forEach(function (view) {
			var st = a.querySelector('.stack[data-view="' + view + '"]'), img = st.querySelector('img');
			var src = frameSrc(p, view === 'orig' ? 'orig' : 'port', k);
			if (src) img.setAttribute('src', src); else img.removeAttribute('src');
			addOverlays(st, p, view === 'marked', k);
		});
		if (zoomState && zoomState.i === i) openZoom(i, zoomState.view);
	}
	function stepFrame(i, d) { if (isClip(pairs[i])) showFrame(i, (clipFrame[i] || 0) + d); }
	function jumpFlagged(i, d) {
		var p = pairs[i], k = clipFrame[i] || 0, best = null;
		p.flagged.forEach(function (f) {
			if (d > 0 && f.frame > k && (best === null || f.frame < best)) best = f.frame;
			if (d < 0 && f.frame < k && (best === null || f.frame > best)) best = f.frame;
		});
		if (best !== null) showFrame(i, best);
	}

	/* ---- the zoom at 1:1 */
	function openZoom(i, view) {
		var p = pairs[i], stack = $('zstack'), clip = isClip(p), k = clipFrame[i] || 0;
		zoomState = { i: i, view: view };
		stack.textContent = '';
		var img = new Image();
		if (view === 'pixels' && (clip || !p.images.pixels)) view = 'marked';
		var src = clip ? frameSrc(p, view === 'orig' ? 'orig' : 'port', k) : (view === 'orig' ? p.images.orig : view === 'pixels' ? p.images.pixels : p.images.port);
		if (src) img.src = src;
		img.alt = VIEWS[view] + ' of ' + p.name;
		stack.appendChild(img);
		addOverlays(stack, p, view === 'marked', k);
		if (view !== 'marked') Array.prototype.slice.call(stack.querySelectorAll('.ign')).forEach(function (e) { e.remove(); });
		$('zlabel').textContent = p.name + (clip ? ' frame ' + k : '') + ': ' + VIEWS[view] + ', full size' + (clip ? ' (← → step frames)' : '');
		$('zoom').hidden = false;
	}
	function closeZoom() { $('zoom').hidden = true; zoomState = null; }

	/* ---- copy */
	function summaryText() {
		var lines = [], marked = 0;
		['issue', 'unsure', 'fine'].forEach(function (k) {
			pairs.forEach(function (p) {
				if (markOf(p) !== k) return;
				marked++;
				var e = entry(p);
				var what = isClip(p) ? p.count + ' of ' + p.frames + ' frames flagged'
					: p.what || p.pct + '% ' + p.count + (p.count === 1 ? ' box' : ' boxes');
				lines.push(LABEL[k] + ' ' + p.name + ' ' + what + (e.note ? ': ' + e.note : ''));
			});
		});
		pairs.forEach(function (p) {
			var e = entry(p);
			if (e && !e.mark && e.note) lines.push('NOTE ' + p.name + ': ' + e.note);
		});
		var open = pairs.filter(function (p) { return !markOf(p); }).map(function (p) { return p.name; });
		if (open.length) lines.push('UNMARKED ' + open.length + ': ' + open.slice(0, 30).join(', ') + (open.length > 30 ? ', ...' : ''));
		lines.unshift('shotdiff marks ' + DATA.game + '/' + DATA.run + ' (' + marked + ' of ' + pairs.length + ' marked)');
		return lines.join('\n');
	}
	function copyMarks() {
		var text = summaryText(), area = $('copytext');
		area.value = text;
		function fallback() {
			$('copybox').open = true;
			area.focus(); area.select();
			try { say(document.execCommand('copy') ? 'Copied.' : 'Select the text below and copy it.'); } catch (e) { say('Select the text below and copy it.'); }
		}
		if (navigator.clipboard && navigator.clipboard.writeText) {
			navigator.clipboard.writeText(text).then(function () { say('Copied ' + text.split('\n').length + ' lines.'); }, fallback);
		} else { fallback(); }
	}

	/* ---- events */
	document.addEventListener('click', function (ev) {
		var t = ev.target;
		if (!(t instanceof Element)) return;
		if ($('zoom').contains(t)) { closeZoom(); return; }
		var a = t.closest('.pair');
		if (a) setCurrent(Number(a.getAttribute('data-i')), false);
		var b = t.closest('button[data-mark]');
		if (b && a) { setMark(Number(a.getAttribute('data-i')), b.getAttribute('data-mark')); return; }
		var s = t.closest('.shots .stack');
		if (s && a) openZoom(Number(a.getAttribute('data-i')), s.getAttribute('data-view'));
	});
	document.addEventListener('input', function (ev) {
		var t = ev.target;
		if (!(t instanceof Element)) return;
		if (t.classList.contains('slider')) { showFrame(Number(t.closest('.pair').getAttribute('data-i')), Number(t.value)); return; }
		if (!loaded || !t.classList.contains('note')) return;
		var i = Number(t.closest('.pair').getAttribute('data-i')), p = pairs[i], e = ensure(p);
		e.note = t.value;
		dropIfEmpty(p);
		persistSoon();
	});
	document.addEventListener('focusin', function (ev) {
		var a = ev.target instanceof Element && ev.target.closest('.pair');
		if (a) setCurrent(Number(a.getAttribute('data-i')), false);
	});
	document.addEventListener('focusout', function (ev) {
		if (ev.target instanceof Element && ev.target.classList.contains('note')) { clearTimeout(saveTimer); if (loaded) persist(); }
	});
	document.addEventListener('keydown', function (ev) {
		if (ev.ctrlKey || ev.metaKey || ev.altKey) return;
		var t = ev.target;
		if (t instanceof Element && /^(INPUT|TEXTAREA|SELECT)$/.test(t.tagName)) {
			if (ev.key === 'Escape') t.blur();
			return;
		}
		var k = ev.key;
		if (zoomState) {
			if ((k === 'ArrowLeft' || k === 'ArrowRight') && isClip(pairs[zoomState.i])) stepFrame(zoomState.i, k === 'ArrowLeft' ? -1 : 1);
			else if (k === 'Escape' || k === 'z') closeZoom();
			else if (k === '1') openZoom(zoomState.i, 'orig');
			else if (k === '2') openZoom(zoomState.i, 'port');
			else if (k === '3') openZoom(zoomState.i, 'marked');
			else if (k === '4') openZoom(zoomState.i, 'pixels');
			else return;
			ev.preventDefault();
			return;
		}
		var clip = cur >= 0 && isClip(pairs[cur]);
		if (k === 'j') move(1);
		else if (k === 'k') move(-1);
		else if (k === 'i') setMark(cur, 'issue');
		else if (k === 'u') setMark(cur, 'unsure');
		else if (k === 'f') setMark(cur, 'fine');
		else if (k === 'z' && cur >= 0) openZoom(cur, 'marked');
		else if (k === 'n' && cur >= 0) article(cur).querySelector('.note').focus();
		else if (k === 'ArrowLeft' && clip) stepFrame(cur, -1);
		else if (k === 'ArrowRight' && clip) stepFrame(cur, 1);
		else if (k === '[' && clip) jumpFlagged(cur, -1);
		else if (k === ']' && clip) jumpFlagged(cur, 1);
		else return;
		ev.preventDefault();
	});
	$('copy').addEventListener('click', copyMarks);
	/* a link to a whole shot opens the collapsed list it is in */
	function openTarget() {
		var t = location.hash && document.getElementById(location.hash.slice(1));
		var d = t && t.closest('details');
		if (d && !d.open) { d.open = true; t.scrollIntoView({ block: 'start' }); }
	}
	window.addEventListener('hashchange', openTarget);
	openTarget();

	/* ---- start */
	if (!pairs.length) { say(''); loaded = true; return; }
	ShotMarks.load(DATA.game, DATA.run, 'marks.json', function (d, source) {
		if (d) { doc = d; doc.game = DATA.game; doc.run = DATA.run; }
		loaded = true;
		pairs.forEach(function (p, i) { if (isClip(p)) showFrame(i, p.flagged.length ? p.flagged[0].frame : 0); });
		renderAll();
		var first = 0;
		while (first < pairs.length && markOf(pairs[first])) first++;
		if (first >= pairs.length) first = 0;
		setCurrent(first, first > 0);
		if (source === 'local-newer') persist();
		else if (ShotMarks.served) say(source === 'server' ? 'Marks are saved to marks.json beside this page.' : 'No marks yet; they will be saved to marks.json beside this page.');
		else say('Opened as a file: marks stay in this browser. Use Copy marks to pass them on, or serve the folder with tools/shotdiff/serve.py to save marks.json.');
	});
})();
