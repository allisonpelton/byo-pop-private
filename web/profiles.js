// The site's players: names the player gives once, which the games use for their own players and high-score names
// (docs/WEB_PLAYER.md, "Player profiles"). The listing page's Players section (web/players.js) edits them; every game page
// and every game asks for them through here.
//
// Three localStorage items, each a JSON object. Every read and write is guarded: with storage blocked nothing is
// kept, a read gives nothing and a write says it did not happen.
//
//   byopop-profiles        {profiles: [{id, name}], current: id}  the site's players, and the one in use. Names are unique
//                          whatever the case: the games compare names without regard to case.
//   byopop-game-profiles   {<game>: {kind, names, current, fits, rules, deletes, max, canRename, canDelete, at, results}}  what each
//                          game last said about its own players (it says so at every start and whenever its list changes):
//                          kind "profiles" for a game with players, "scores" for one that only asks for a name at each high
//                          score (names then holds the last name it remembers); fits, for each site player's name, the name
//                          the game's name box would keep; rules, the game's name rules in words; deletes, what the game's own
//                          delete removes; results, what became of the operations below.
//   byopop-profile-ops     {<game>: [{id, op, ...}]}  operations from the Players section, waiting for the game: it applies
//                          them at its next start, with its own functions, and reports what became of each. op "rename"
//                          {from, to} renames the game's player; op "delete" {name} deletes the player and its progress.
//
// The setting "profile" (web/settings.js) says what a game does with the current site player: "game" nothing, "fill" its name
// boxes start with the name, "create" the game also has the player and selects it at start. A game asks the page for
// that when it needs it (forGame), and reports back (report): the page cannot push to a game whose dialog opens at start.
var ByopopProfiles = (function () {
  "use strict";
  var KEY = "byopop-profiles", GAME_KEY = "byopop-game-profiles", OPS_KEY = "byopop-profile-ops";
  var NAME_MAX = 32, NAMES_MAX = 100, RESULTS_MAX = 40;

  function store() { try { return window.localStorage || null; } catch (e) { return null; } }
  function plain(o) { return !!o && typeof o === "object" && !Array.isArray(o); }
  function readJson(key) {
    try {
      var s = store();
      return s ? JSON.parse(s.getItem(key) || "null") : null;
    } catch (e) { return null; }
  }
  function writeJson(key, value) {
    try {
      var s = store();
      if (!s) return false;
      if (value === null) s.removeItem(key); else s.setItem(key, JSON.stringify(value));
      return true;
    } catch (e) { return false; }
  }
  function lower(t) { return String(t).toLowerCase(); }
  function text(t, max) { return (typeof t === "string") ? t.slice(0, max) : ""; }
  function validGame(id) { return typeof id === "string" && /^[a-z0-9_-]{1,40}$/.test(id); }

  // ---------- the site's players ----------
  // A name as the site keeps it: control characters and runs of spaces become one space, trimmed, at most NAME_MAX characters
  function clean(name) {
    return String(name === undefined || name === null ? "" : name).replace(/[\u0000-\u001f\u007f-\u009f]/g, " ").replace(/\s+/g, " ")
      .trim().slice(0, NAME_MAX).trim();
  }
  function load() {
    var o = readJson(KEY), out = { profiles: [], current: "" };
    if (!plain(o) || !Array.isArray(o.profiles)) return out;
    var ids = {}, names = {};
    o.profiles.forEach(function (p) {
      if (!plain(p) || typeof p.id !== "string" || !p.id || typeof p.name !== "string") return;
      var name = clean(p.name);
      if (!name || ids[p.id] || names[lower(name)]) return;
      ids[p.id] = names[lower(name)] = true;
      out.profiles.push({ id: p.id, name: name });
    });
    if (typeof o.current === "string" && ids[o.current]) out.current = o.current;
    return out;
  }
  function save(o) { return writeJson(KEY, o.profiles.length ? o : null); }
  function newId(o) {
    var id;
    do { id = "p" + Date.now().toString(36) + Math.floor(Math.random() * 1296).toString(36); }
    while (o.profiles.some(function (p) { return p.id === id; }));
    return id;
  }
  function list() { return load().profiles; }
  function current() {
    var o = load();
    for (var i = 0; i < o.profiles.length; i++) if (o.profiles[i].id === o.current) return o.profiles[i];
    return null;
  }
  function find(name) {
    var n = lower(clean(name)), ps = load().profiles;
    for (var i = 0; i < ps.length; i++) if (lower(ps[i].name) === n) return ps[i];
    return null;
  }
  function failure(error) { return { ok: false, error: error }; }
  // Adds a player (the first one is the current one). {ok, profile} or {ok: false, error}
  function add(name) {
    var n = clean(name), o = load();
    if (!n) return failure("Type a name for the player.");
    var same = find(n);
    if (same) return failure("There is already a player called “" + same.name + "”.");
    var p = { id: newId(o), name: n };
    o.profiles.push(p);
    if (!o.current) o.current = p.id;
    return save(o) ? { ok: true, profile: p } : failure("This browser would not keep the player (its storage is blocked or full).");
  }
  function rename(id, name) {
    var n = clean(name), o = load();
    if (!n) return failure("Type a name for the player.");
    var target = null;
    o.profiles.forEach(function (p) { if (p.id === id) target = p; });
    if (!target) return failure("That player is gone.");
    var same = find(n);
    if (same && same.id !== id) return failure("There is already a player called “" + same.name + "”.");
    target.name = n;
    return save(o) ? { ok: true, profile: target } : failure("This browser would not keep the player (its storage is blocked or full).");
  }
  // Removes the site's player (the games' players stay); the current one falls to the first that is left
  function remove(id) {
    var o = load(), before = o.profiles.length;
    o.profiles = o.profiles.filter(function (p) { return p.id !== id; });
    if (o.profiles.length === before) return failure("That player is gone.");
    if (o.current === id) o.current = o.profiles.length ? o.profiles[0].id : "";
    return save(o) ? { ok: true } : failure("This browser would not keep the change.");
  }
  function setCurrent(id) {
    var o = load();
    if (id && !o.profiles.some(function (p) { return p.id === id; })) return failure("That player is gone.");
    o.current = id || "";
    return save(o) ? { ok: true } : failure("This browser would not keep the change.");
  }

  // ---------- what the games report ----------
  function strings(a, max, each) {
    var out = [];
    if (Array.isArray(a)) a.forEach(function (t) { if (typeof t === "string" && out.length < max) out.push(t.slice(0, each)); });
    return out;
  }
  function games() {
    var o = readJson(GAME_KEY), out = {};
    if (!plain(o)) return out;
    Object.keys(o).forEach(function (id) { if (validGame(id) && plain(o[id])) out[id] = o[id]; });
    return out;
  }
  function allOps() {
    var o = readJson(OPS_KEY), out = {};
    if (!plain(o)) return out;
    Object.keys(o).forEach(function (id) {
      if (validGame(id) && Array.isArray(o[id])) out[id] = o[id].filter(function (op) { return plain(op) && typeof op.id === "number"; });
    });
    return out;
  }
  function saveOps(ops) {
    Object.keys(ops).forEach(function (id) { if (!ops[id].length) delete ops[id]; });
    return writeJson(OPS_KEY, Object.keys(ops).length ? ops : null);
  }
  function ops(gameId) { return allOps()[gameId] || []; }

  // A game's report. rec: {kind, names, current, fits, rules, max, canRename, canDelete, results: [{id, status, note}]}. The
  // operations named in results are done with (applied or skipped) and leave the queue; their results are kept with the record,
  // newest last. Returns whether it was written.
  function report(gameId, rec) {
    if (!validGame(gameId) || !plain(rec)) return false;
    var all = games(), old = all[gameId] || {};
    var queued = allOps(), mine = queued[gameId] || [], done = {};
    var results = (Array.isArray(old.results) ? old.results : []).slice();
    if (Array.isArray(rec.results)) {
      rec.results.forEach(function (r) {
        if (!plain(r) || typeof r.id !== "number") return;
        var op = null;
        mine.forEach(function (q) { if (q.id === r.id) op = q; });
        done[r.id] = true;
        var item = { id: r.id, op: op ? op.op : text(r.op, 16), status: r.status === "done" ? "done" : "skipped", note: text(r.note, 200),
          at: new Date().toISOString() };
        if (op) { ["from", "to", "name"].forEach(function (k) { if (typeof op[k] === "string") item[k] = op[k]; }); }
        else ["from", "to", "name"].forEach(function (k) { if (typeof r[k] === "string") item[k] = text(r[k], NAME_MAX * 2); });
        results.push(item);
      });
    }
    var fits = {};
    if (plain(rec.fits)) Object.keys(rec.fits).slice(0, NAMES_MAX).forEach(function (k) { if (typeof rec.fits[k] === "string") fits[text(k, 80)] = text(rec.fits[k], 80); });
    all[gameId] = {
      kind: rec.kind === "scores" ? "scores" : "profiles",
      names: strings(rec.names, NAMES_MAX, 80),
      current: text(rec.current, 80),
      fits: fits,
      rules: text(rec.rules, 240),
      deletes: text(rec.deletes, 240),
      max: (typeof rec.max === "number" && rec.max > 0) ? Math.floor(rec.max) : 0,
      canRename: rec.canRename !== false,
      canDelete: rec.canDelete !== false && rec.kind !== "scores",
      at: new Date().toISOString(),
      results: results.slice(-RESULTS_MAX)
    };
    var ok = writeJson(GAME_KEY, all);
    if (ok && Object.keys(done).length) {
      queued[gameId] = mine.filter(function (q) { return !done[q.id]; });
      saveOps(queued);
    }
    return ok;
  }

  // ---------- what the page asks of a game, and the game of the page ----------
  // The mode of the setting "profile" for this game, the current site player's name (empty with none), the site players' names
  // and the operations waiting for the game. This is what a game asks for at its decision points.
  function forGame(gameId) {
    var mode = "game";
    try { mode = ByopopSettings.get("profile", gameId); } catch (e) {}
    var cur = current();
    return { mode: mode, name: cur ? cur.name : "", profiles: list().map(function (p) { return p.name; }), ops: ops(gameId) };
  }

  // ---------- operations ----------
  var opCounter = 0;
  // Queues an operation for a game: {op: "rename", from, to} or {op: "delete", name}. Returns its number, or 0 when it was not kept.
  function queue(gameId, op) {
    if (!validGame(gameId) || !plain(op)) return 0;
    var all = allOps(), mine = all[gameId] || [];
    var next = 1;
    Object.keys(all).forEach(function (g) { all[g].forEach(function (q) { if (q.id >= next) next = q.id + 1; }); });
    var rec = games();
    Object.keys(rec).forEach(function (g) { (rec[g].results || []).forEach(function (r) { if (r && typeof r.id === "number" && r.id >= next) next = r.id + 1; }); });
    opCounter = Math.max(opCounter + 1, next);
    var item = { id: opCounter, op: op.op === "delete" ? "delete" : "rename", at: new Date().toISOString() };
    if (item.op === "rename") { item.from = text(op.from, 80); item.to = text(op.to, 80); if (!item.from || !item.to) return 0; }
    else { item.name = text(op.name, 80); if (!item.name) return 0; }
    mine.push(item);
    all[gameId] = mine;
    return saveOps(all) ? item.id : 0;
  }
  function cancel(gameId, id) {
    var all = allOps();
    if (!all[gameId]) return false;
    all[gameId] = all[gameId].filter(function (q) { return q.id !== id; });
    return saveOps(all);
  }
  // The game's name for a site player in a game's report: {name, exact} or null. A game has the player when it has a name that
  // is the same whatever the case, or the name its rules make of it (a shorter one); exact: the same, as typed.
  function match(rec, siteName) {
    if (!rec || !Array.isArray(rec.names)) return null;
    var n = lower(siteName), fitted = plain(rec.fits) && typeof rec.fits[siteName] === "string" ? lower(rec.fits[siteName]) : null, hit = null;
    rec.names.forEach(function (g) { if (hit === null && lower(g) === n) hit = g; });
    if (hit === null && fitted) rec.names.forEach(function (g) { if (hit === null && fitted && lower(g) === fitted) hit = g; });
    return hit === null ? null : { name: hit, exact: hit === siteName };
  }

  return { KEY: KEY, GAME_KEY: GAME_KEY, OPS_KEY: OPS_KEY, NAME_MAX: NAME_MAX, clean: clean, list: list, current: current, find: find,
    add: add, rename: rename, remove: remove, setCurrent: setCurrent, games: games, allOps: allOps, ops: ops, report: report,
    forGame: forGame, queue: queue, cancel: cancel, match: match };
})();
