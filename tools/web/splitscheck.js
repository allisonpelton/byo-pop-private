// The speedrun timer's Reset keeps nothing the player declined (the user, 2026-11-11: "I told the speedrun timer I didn't
// want to keep my best split when I abandoned a run, but it saved it anyway"; web/splits.js, reset). Runs in the front page,
// like deskcheck.js:
//   http://127.0.0.1:18780/index.html?silent=1&driver=tools/web/splitscheck.js
// A test set of splits (its own game name, removed at the end) is stored with a slow personal best and slow golds, so every
// attempt beats them. Each case reads the stored record from IndexedDB before and after:
//   no        a finished run (a new personal best and new golds), Reset, No: the stored splits are as before, the attempt
//             count apart (counted at Start, as LiveSplit counts it)
//   abandon   a run reset after one split (a new gold), No: as before
//   close     the question closed with Escape: as No
//   undo      a finished run whose last split is undone, then Reset, No: as before
//   yes       a finished run, Reset, Yes: the personal best and golds are the run's
var results = [];
function note(check, ok, detail) { results.push({ check: check, result: ok ? "pass" : "FAIL", detail: detail || "" }); }
function wait(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }
var timer = ByopopSplits.timer();
if (!timer) return { pass: 0, fail: 1, results: [{ check: "timer", result: "FAIL", detail: "no timer on this page" }] };

var GAME = "__splitscheck__", KEY = GAME + "\u0000test";
function idb(mode, fn) {
  return new Promise(function (res, rej) {
    var o = indexedDB.open("byopop-splits", 1);
    o.onupgradeneeded = function () { o.result.createObjectStore("runs", { keyPath: "key" }); };
    o.onerror = function () { rej(o.error); };
    o.onsuccess = function () {
      var t = o.result.transaction("runs", mode), r = fn(t.objectStore("runs"));
      t.oncomplete = function () { o.result.close(); res(r && r.result); };
      t.onerror = function () { o.result.close(); rej(t.error); };
    };
  });
}
function stored() { return idb("readonly", function (s) { return s.get(KEY); }); }
function times(r) { return r ? JSON.stringify(r.segments.map(function (s) { return [s.pb, s.best]; })) : "none"; }
var SLOW = 1e9;
var base = { game: GAME, category: "test", offset: 0, attempts: 0, segments: [0, 1, 2].map(function (i) {
  return { name: "S" + (i + 1), pb: { rt: SLOW * (i + 1), gt: null }, best: { rt: SLOW, gt: null } };
}) };
var lastKey = null;
try { lastKey = localStorage.getItem("byopop-splits-last"); } catch (e) {}
async function fresh() {
  await idb("readwrite", function (s) { var r = JSON.parse(JSON.stringify(base)); r.key = KEY; return s.put(r); });
  timer.load(JSON.parse(JSON.stringify(base)));
  return stored();
}
async function answer(how) {
  await wait(30);
  var dl = Array.prototype.filter.call(document.querySelectorAll("dialog.spl-ask"), function (x) { return x.open; }).pop();
  if (!dl) return false;
  var label = ByopopStrings.t(how === "yes" ? "splits.yes" : "splits.no");
  var b = Array.prototype.filter.call(dl.querySelectorAll("button"), function (x) { return x.textContent === label; })[0];
  if (how === "escape") dl.dispatchEvent(new Event("cancel", { cancelable: true }));
  else if (b) b.click();
  else return false;
  await wait(100);
  return true;
}
async function play(splits) { timer.split(); for (var i = 0; i < splits; i++) { await wait(15); timer.split(); } }
async function same(name, prepare, how) {
  var before = await fresh();
  await prepare();
  timer.reset();
  var asked = await answer(how);
  var after = await stored();
  var ok = asked && times(after) === times(before) && times(timer.run()) === times(before);
  note(name, ok, (asked ? "asked" : "never asked") + "; stored " + (times(after) === times(before) ? "as before" : "changed") +
       "; shown " + (times(timer.run()) === times(before) ? "as before" : "changed"));
}

await same("no", function () { return play(3); }, "no");
await same("abandon", function () { return play(1); }, "no");
await same("close", function () { return play(3); }, "escape");
await same("undo", async function () { await play(3); timer.undo(); }, "no");

var before = await fresh();
await play(3);
var att = timer.attempt().times;
timer.reset();
var asked = await answer("yes");
var after = await stored();
var pbOk = after && after.segments.every(function (s, i) { return s.pb.rt === att[i].rt; });
var goldOk = after && after.segments.every(function (s) { return s.best.rt < SLOW; });
note("yes", asked && pbOk && goldOk, (asked ? "asked" : "never asked") + "; personal best " + (pbOk ? "the run's" : "not the run's") +
     "; golds " + (goldOk ? "the run's" : "not the run's"));

await idb("readwrite", function (s) { return s.delete(KEY); });
try { if (lastKey) localStorage.setItem("byopop-splits-last", lastKey); else localStorage.removeItem("byopop-splits-last"); } catch (e) {}
timer.load({ game: "", category: "", offset: 0, attempts: 0, segments: [{ name: "", pb: { rt: null, gt: null }, best: { rt: null, gt: null } }] });
var fail = results.filter(function (x) { return x.result === "FAIL"; }).length;
return { pass: results.length - fail, fail: fail, results: results };
