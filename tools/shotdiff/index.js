/* The all-games page: each game's latest run with its counts, the marks made so far, and the games that still have
   unmarked differences first. The counts in the HTML are those of the last shotdiff run; the marks are read again here. */
(function () {
	'use strict';
	var DATA = JSON.parse(document.getElementById('shotdiff-data').textContent);
	var tbody = document.getElementById('rows');

	function counts(run, doc) {
		var n = { issue: 0, unsure: 0, fine: 0 };
		run.names.forEach(function (name) {
			var e = doc && doc.marks && doc.marks[name];
			if (e && n.hasOwnProperty(e.mark)) n[e.mark]++;
		});
		return n;
	}

	function show(run, n) {
		var marked = n.issue + n.unsure + n.fine, open = run.names.length - marked;
		run.open = open;
		var row = document.getElementById('r-' + run.game);
		row.querySelector('.c-marked').textContent = marked + (marked ? ' (' + n.issue + ' issue, ' + n.unsure + ' not sure, ' + n.fine + ' fine)' : '');
		var u = row.querySelector('.c-open');
		u.textContent = String(open);
		u.className = 'n c-open' + (open ? ' big' : '');
		row.className = open ? '' : 'done';
	}

	function sortRows() {
		DATA.runs.slice().sort(function (a, b) { return b.open - a.open || (a.game < b.game ? -1 : 1); })
			.forEach(function (run) { tbody.appendChild(document.getElementById('r-' + run.game)); });
		var total = 0, left = 0;
		DATA.runs.forEach(function (r) { total += r.names.length; left += r.open; });
		document.getElementById('total').textContent = DATA.runs.length + ' games, ' + total + ' differing pairs, ' + left + ' not yet marked.';
	}

	DATA.runs.forEach(function (run) {
		run.open = run.names.length;
		show(run, run.marks);
		ShotMarks.load(run.game, run.run, run.game + '/' + run.run + '/marks.json', function (doc) {
			show(run, counts(run, doc));
			sortRows();
		});
	});
	sortRows();
})();
