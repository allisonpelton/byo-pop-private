// Each game's phase on the listing, on a development host only (the user, 2026-11-11: "Perhaps add a status line to the games
// listing for what phase a game is in? For the test site only. Then I'd know if I'm really finding something that would get
// overlooked otherwise."). The coordinator keeps docs/game_status.json (each game's steps done, not applying and under way, and a
// few words of note), served on development hosts by tools/webtest/serve.py at /__repo/docs/game_status.json and fetched only
// there (ByopopTerms.devHost): a deployed page never asks for it and shows nothing. The Library has a Phase column (web/library.js):
// the step under way (or "Done"), its tooltip the steps done, the steps left and the note. Values only: the step names are the file's own labels.
var ByopopPhase = (function () {
  "use strict";
  var URL = "/__repo/docs/game_status.json";
  var data = null, on = false;
  function T(k, o) { try { return ByopopStrings.t(k, o); } catch (e) { return k; } }
  try { on = typeof ByopopTerms !== "undefined" && ByopopTerms.devHost(location); } catch (e) { on = false; }

  function labels() {
    var out = {};
    (data && Array.isArray(data.steps) ? data.steps : []).forEach(function (s) { if (s && s.id) out[s.id] = s.label || s.id; });
    return out;
  }
  // {text, tip, rank} for a game, or null: rank orders the games by how far they are (the step under way's place, done last)
  function of(id) {
    var g = data && data.games && data.games[id];
    if (!g) return null;
    var steps = Array.isArray(data.steps) ? data.steps : [], name = labels();
    var done = Array.isArray(g.done) ? g.done : [], na = Array.isArray(g.na) ? g.na : [];
    var left = steps.filter(function (s) { return done.indexOf(s.id) < 0 && na.indexOf(s.id) < 0; }).map(function (s) { return s.id; });
    var now = g.now && name[g.now] ? g.now : (left[0] || null);
    var text = now ? name[now] : T("phase.done");
    var lines = [];
    if (done.length) lines.push(T("phase.tip.done", { steps: done.map(function (s) { return name[s] || s; }).join(", ") }));
    if (left.length) lines.push(T("phase.tip.left", { steps: left.map(function (s) { return name[s] || s; }).join(", ") }));
    if (na.length) lines.push(T("phase.tip.na", { steps: na.map(function (s) { return name[s] || s; }).join(", ") }));
    if (g.note) lines.push(String(g.note));
    var rank = now ? steps.map(function (s) { return s.id; }).indexOf(now) : steps.length;
    return { text: text, tip: lines.join("\n"), rank: rank };
  }
  var ready = !on ? Promise.resolve(null) : fetch(URL, { cache: "no-store" }).then(function (r) { return r.ok ? r.json() : null; })
    .then(function (d) {
      data = d && typeof d === "object" && d.games ? d : null;
      if (data) { try { window.dispatchEvent(new CustomEvent("byopop-phase")); } catch (e) {} }
      return data;
    }).catch(function () { return null; });
  return { on: function () { return on; }, has: function () { return !!data; }, of: of, ready: ready };
})();
