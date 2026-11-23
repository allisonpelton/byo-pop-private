// Uninstall's check (docs/WEB_PLAYER.md, "The Library"): the Library's menu and a desktop shortcut's menu each taken to an
// uninstall, the result compared with the recycle bin's. It runs in the front page, served by tools/webtest/serve.py on a
// development host, at a width the desktop takes (760 pixels or more):
//   http://127.0.0.1:18780/index.html?silent=1&driver=tools/web/uninstallcheck.js
// The page fetches it from /__repo/ and runs it as the body of an async function; its result is window.byopopDeskCheck.result
// (and the page's title ends "uninstallcheck: PASS" or "FAIL"): {pass, fail, results: [{check, result: "pass"|"FAIL", detail}]}.
// A game the listing adds and whose database this browser does not have is given stand-in files (a few records in its IDBFS
// database, as a copy would leave them: the game folder, a save, a mod, and the screen saver's profile beside it) and its
// record of files loaded, three times over: then the recycle bin forgets them (its panel's Yes), the Library's row menu's
// Uninstall with Yes does, and a shortcut's Uninstall with Yes does. After each, what is left (the database's keys, the record,
// the bin's last name, the card's data-files, the saver's profile, the menus' entries) must be the same. The checks:
//   game        a game to try it on was found
//   greyed      Uninstall is greyed in both menus for a game whose files are not here, and not once they are
//   question    the box: its title, the standard question with the game's name, Yes and No, No focused; No (a click, and
//               Escape) leaves the files
//   bin         the bin's Yes forgot the files: the game folder's records gone, the save and the mod kept
//   library     the Library's Uninstall left the same as the bin
//   shortcut    the shortcut's Uninstall left the same as the bin
//   focus       after each, the focus back on the Library's row or the shortcut
// Everything it changed (its database, the record, the bin, the favourite, the stored desk) is put back at the end. No link is
// clicked and no game is run.
var results = [];
function note(check, ok, detail) { results.push({ check: check, result: ok ? "pass" : "FAIL", detail: detail || "" }); }
function wait(ms) { return new Promise(function (r) { setTimeout(r, ms); }); }
async function until(fn, ms) { var end = Date.now() + (ms || 5000); while (Date.now() < end) { if (fn()) return true; await wait(25); } return !!fn(); }
var STORE = "FILE_DATA", VERSION = 21;
function idb(req) { return new Promise(function (ok, no) { req.onsuccess = function () { ok(req.result); }; req.onerror = function () { no(req.error); }; }); }
function open(name) {
  var r = indexedDB.open(name, VERSION);
  r.onupgradeneeded = function () { var db = r.result; if (!db.objectStoreNames.contains(STORE)) db.createObjectStore(STORE).createIndex("timestamp", "timestamp", { unique: false }); };
  return idb(r);
}
async function keys(name) {
  var names = (await indexedDB.databases()).map(function (d) { return d.name; });
  if (names.indexOf(name) < 0) return null;
  var db = await open(name), out = await idb(db.transaction(STORE, "readonly").objectStore(STORE).getAllKeys());
  db.close();
  return out.map(String).sort();
}
async function dbExists(name) { return (await indexedDB.databases()).some(function (d) { return d.name === name; }); }
function drop(name) { return new Promise(function (ok) { var r = indexedDB.deleteDatabase(name); r.onsuccess = r.onerror = r.onblocked = function () { ok(); }; }); }

var d = ByopopSkin.desk, kept = {}, i;
for (i = 0; i < localStorage.length; i++) kept[localStorage.key(i)] = localStorage.getItem(localStorage.key(i));
var names = (await indexedDB.databases()).map(function (x) { return x.name; });
// a game the listing adds, with a page, whose database is not here (its files never loaded in this browser)
var tile = Array.prototype.filter.call(document.querySelectorAll("a.tile[data-game]"), function (a) {
  var id = a.getAttribute("data-game"), p = a.getAttribute("data-persist");
  return ByopopAddGames.has(id) && p && names.indexOf(p) < 0 && names.indexOf(p + "-saver") < 0 && !a.closest("li").hidden;
})[0];
note("game", !!tile, tile ? tile.getAttribute("data-game") : "every game the listing adds has a database here");
if (tile) {
  var id = tile.getAttribute("data-game"), persist = tile.getAttribute("data-persist"), li = tile.closest("li");
  var title = li.getAttribute("data-title");
  // the stand-in files: the folders and files as IDBFS keeps them, the screen saver's profile beside them
  async function install() {
    var db = await open(persist), tx = db.transaction(STORE, "readwrite"), st = tx.objectStore(STORE), now = new Date();
    var dir = { timestamp: now, mode: 16895 }, file = { timestamp: now, mode: 33206, contents: new Uint8Array([1, 2, 3]) };
    ["", "/game", "/userdata", "/mods"].forEach(function (k) { st.put(dir, persist + k); });
    [["/game/main.dat"], ["/game/sub/level.dat"], ["/userdata/save.dat"], ["/mods/mod.txt"]].forEach(function (k) { st.put(file, persist + k[0]); });
    await new Promise(function (ok, no) { tx.oncomplete = ok; tx.onerror = function () { no(tx.error); }; });
    db.close();
    var s = await idb(indexedDB.open(persist + "-saver", 1)); s.close();
    ByopopGameFiles.mark(id);
    await ByopopTiles.check();
    await until(function () { return li.getAttribute("data-files") === "yes"; });
    ByopopLibrary.refresh();
  }
  // what is left after an uninstall, without the days and times it was done at
  async function state() {
    var rec = ByopopGameFiles.all()[id] || null, bin = [];
    try { bin = JSON.parse(localStorage.getItem("byopop-bin") || "[]"); } catch (e) {}
    var last = bin.length ? bin[bin.length - 1] : null;
    return JSON.stringify({ keys: await keys(persist), record: rec && { loaded: !!rec.loaded, forgotten: !!rec.forgotten, missing: !!rec.missing, present: ByopopGameFiles.present(rec) },
      bin: last && { id: last.id, title: last.title }, files: li.getAttribute("data-files"), saver: await dbExists(persist + "-saver"),
      installed: d.installed(id) });
  }
  function row() { return Array.prototype.filter.call(document.querySelectorAll("#library tr.lib-row"), function (r) { return r.querySelector(".lib-c-name").textContent.indexOf(title) >= 0 && r.querySelector(".lib-c-name").textContent.slice(-title.length) === title; })[0] || null; }
  function menuEntry(label) {
    var m = document.querySelector(".d95-ctx");
    return m ? Array.prototype.filter.call(m.querySelectorAll(".d95-item"), function (b) { return b.textContent === label; })[0] || null : null;
  }
  function closeMenu() { var m = document.querySelector(".d95-ctx"); if (m) m.dispatchEvent(new KeyboardEvent("keydown", { key: "Escape", bubbles: true })); }
  async function libraryMenu() {
    d.show("library");
    ByopopLibrary.refresh();
    await until(function () { return !!row(); });
    var cell = row().querySelector(".lib-c-name"), r = cell.getBoundingClientRect();
    cell.dispatchEvent(new MouseEvent("mousedown", { bubbles: true, cancelable: true, button: 0, clientX: r.left + 30, clientY: r.top + 4 }));
    cell.dispatchEvent(new MouseEvent("contextmenu", { bubbles: true, cancelable: true, clientX: r.left + 30, clientY: r.top + 4 }));
    await until(function () { return !!document.querySelector(".d95-ctx"); });
    return cell;
  }
  function shortcut() { return document.querySelector('.d95-icons .d95-fav[data-key="f:' + id + '"]'); }
  async function shortcutMenu() {
    await until(function () { return !!shortcut(); });
    var b = shortcut();
    b.focus();
    b.dispatchEvent(new KeyboardEvent("keydown", { key: "ContextMenu", bubbles: true, cancelable: true }));
    await until(function () { return !!document.querySelector(".d95-ctx"); });
    return b;
  }
  var UNINSTALL = ByopopStrings.t("skin.uninstall.entry");
  function box() { return document.querySelector("dialog.d95-msgbox[open]"); }
  function boxButton(label) { var b = box(); return b ? Array.prototype.filter.call(b.querySelectorAll(".d95-msgbox-row button"), function (x) { return x.textContent === label; })[0] : null; }
  var question = ByopopStrings.t("skin.uninstall.question", { game: title }), yes = ByopopStrings.t("skin.uninstall.yes"), no = ByopopStrings.t("skin.uninstall.no");
  function boxRight() {
    var b = box();
    if (!b) return "no box";
    var t = b.querySelector(".d95-tb-text").textContent, q = b.querySelector("#d95-msgbox-text").textContent;
    var btns = Array.prototype.map.call(b.querySelectorAll(".d95-msgbox-row button"), function (x) { return x.textContent; });
    if (t !== ByopopStrings.t("skin.uninstall.title")) return "title " + t;
    if (q !== question) return "question " + q;
    if (btns.join("|") !== yes + "|" + no) return "buttons " + btns.join("|");
    if (document.activeElement !== boxButton(no)) return "the focus is not on No";
    if (!b.classList.contains("d95-msgbox-question")) return "no question sign";
    return "";
  }

  try {
    // greyed while the files are not here
    var cell0 = await libraryMenu(), e0 = menuEntry(UNINSTALL), libGrey = !!(e0 && e0.getAttribute("aria-disabled") === "true");
    closeMenu();
    ByopopSettings.setFavourite({ id: id, title: title, page: tile.getAttribute("href") }, true);
    await shortcutMenu();
    var e1 = menuEntry(UNINSTALL), favGrey = !!(e1 && e1.getAttribute("aria-disabled") === "true");
    closeMenu();
    ByopopSettings.setFavourite({ id: id }, false);

    // the bin
    await install();
    var installedState = await state();
    var libOn = await libraryMenu(), e2 = menuEntry(UNINSTALL), libOnOk = !!(e2 && !e2.getAttribute("aria-disabled"));
    closeMenu();
    ByopopAddGames.bin(id);
    await until(function () { var q = document.getElementById("bin-confirm"); return q && !q.hidden; });
    document.getElementById("bin-yes").click();
    await until(function () { return li.getAttribute("data-files") !== "yes" && !document.getElementById("bin-yes").disabled; }, 8000);
    var binState = await state();
    var kept0 = JSON.parse(binState).keys || [];
    note("bin", JSON.parse(installedState).files === "yes" && kept0.indexOf(persist + "/userdata/save.dat") >= 0 && kept0.indexOf(persist + "/mods/mod.txt") >= 0 &&
      !kept0.some(function (k) { return k.indexOf(persist + "/game") === 0; }) && JSON.parse(binState).files !== "yes",
      "installed " + installedState + "; after the bin " + binState);
    document.getElementById("bin-close").click();

    // the Library's menu: No, then Yes
    await install();
    var beforeNo = await state();
    var c = await libraryMenu();
    menuEntry(UNINSTALL).click();
    await until(function () { return !!box(); });
    var qWrong = boxRight();
    boxButton(no).click();
    await wait(100);
    var afterNo = await state();
    var focusNo = document.activeElement === (row() && row().querySelector(".lib-c-name"));
    await libraryMenu();
    menuEntry(UNINSTALL).click();
    await until(function () { return !!box(); });
    boxButton(yes).click();
    await until(function () { return li.getAttribute("data-files") !== "yes"; }, 8000);
    await wait(150);
    var libState = await state();
    var libFocus = document.activeElement === (row() && row().querySelector(".lib-c-name"));
    note("library", libState === binState, "the Library's: " + libState + "; the bin's: " + binState);

    // a shortcut's menu: Escape, then Yes
    await install();
    var beforeEsc = await state();
    ByopopSettings.setFavourite({ id: id, title: title, page: tile.getAttribute("href") }, true);
    await shortcutMenu();
    menuEntry(UNINSTALL).click();
    await until(function () { return !!box(); });
    var qWrong2 = boxRight();
    box().dispatchEvent(new KeyboardEvent("keydown", { key: "Escape", bubbles: true, cancelable: true }));
    await wait(100);
    var afterEsc = await state();
    await shortcutMenu();
    menuEntry(UNINSTALL).click();
    await until(function () { return !!box(); });
    boxButton(yes).click();
    await until(function () { return li.getAttribute("data-files") !== "yes"; }, 8000);
    await wait(150);
    var favState = await state();
    var favFocus = document.activeElement === shortcut();
    await shortcutMenu();
    var e3 = menuEntry(UNINSTALL), greyAfter = !!(e3 && e3.getAttribute("aria-disabled") === "true");
    closeMenu();
    note("shortcut", favState === binState && greyAfter, "the shortcut's: " + favState + "; the bin's: " + binState + (greyAfter ? "" : "; its Uninstall is not greyed afterwards"));

    note("greyed", libGrey && favGrey && libOnOk, "Library before: " + libGrey + ", shortcut before: " + favGrey + ", Library with the files: " + libOnOk);
    note("question", !qWrong && !qWrong2 && afterNo === beforeNo && afterEsc === beforeEsc,
      [qWrong && "Library's box: " + qWrong, qWrong2 && "shortcut's box: " + qWrong2, afterNo !== beforeNo && "No changed " + afterNo,
       afterEsc !== beforeEsc && "Escape changed " + afterEsc].filter(Boolean).join("; "));
    note("focus", focusNo && libFocus && favFocus, "after No: " + focusNo + ", after the Library's Yes: " + libFocus + ", after the shortcut's Yes: " + favFocus);
  } catch (e) {
    note("run", false, String(e && e.stack || e));
  } finally {
    closeMenu();
    var open = box(); if (open) { try { open.close(); open.remove(); } catch (e) {} }
    var bp = document.getElementById("bin-panel"); if (bp && !bp.hidden) document.getElementById("bin-close").click();
    await drop(persist); await drop(persist + "-saver");
    // the stored values as they were (the favourite, the record, the bin, the desk)
    for (i = localStorage.length - 1; i >= 0; i--) { var k = localStorage.key(i); if (!(k in kept)) localStorage.removeItem(k); }
    Object.keys(kept).forEach(function (k) { if (localStorage.getItem(k) !== kept[k]) localStorage.setItem(k, kept[k]); });
    try { window.dispatchEvent(new CustomEvent("byopop-favourites")); window.dispatchEvent(new CustomEvent("byopop-bin")); } catch (e) {}
    await ByopopTiles.check();
  }
}
var fail = results.filter(function (x) { return x.result === "FAIL"; }).length;
return { pass: results.length - fail, fail: fail, results: results };
