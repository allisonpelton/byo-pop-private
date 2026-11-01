/* Shared by the run page and the all-games page: where the marks live.
   Served by tools/shotdiff/serve.py they are saved to marks.json beside the page; opened as a plain file they stay in
   localStorage (and "Copy marks" passes them on). A copy is always kept in localStorage as a backup. */
var ShotMarks = (function () {
	'use strict';
	var served = location.protocol === 'http:' || location.protocol === 'https:';

	function key(game, run) { return 'shotdiff:' + game + '/' + run; }
	function valid(d) { return d && typeof d === 'object' && d.marks && typeof d.marks === 'object' ? d : null; }
	function readLocal(game, run) {
		try { var s = localStorage.getItem(key(game, run)); return s ? valid(JSON.parse(s)) : null; } catch (e) { return null; }
	}
	function writeLocal(game, run, doc) {
		try { localStorage.setItem(key(game, run), JSON.stringify(doc)); } catch (e) { /* private window or blocked storage */ }
	}
	function newer(a, b) {
		if (!a) return b;
		if (!b) return a;
		return (b.updated || '') > (a.updated || '') ? b : a;
	}

	/* done(doc or null, source): source is 'server', 'local', 'local-newer' (the browser's copy is newer than the file) or 'none' */
	function load(game, run, url, done) {
		var local = readLocal(game, run);
		if (!served) { done(local, local ? 'local' : 'none'); return; }
		fetch(url, { cache: 'no-store' })
			.then(function (r) { return r.ok ? r.json() : null; })
			.catch(function () { return null; })
			.then(function (remote) {
				remote = valid(remote);
				var pick = newer(remote, local);
				done(pick, !pick ? 'none' : pick === remote ? 'server' : remote ? 'local-newer' : 'local');
			});
	}

	/* done(true) or done(false) */
	function save(game, run, url, doc, done) {
		writeLocal(game, run, doc);
		if (!served) { done(false); return; }
		fetch(url, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(doc), keepalive: true })
			.then(function (r) { done(r.ok); })
			.catch(function () { done(false); });
	}

	return { served: served, load: load, save: save, writeLocal: writeLocal };
})();
