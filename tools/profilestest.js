// Checks the shared page code for the site's settings and players (web/settings.js, web/profiles.js; docs/WEB_PLAYER.md,
// "Site settings", "Player profiles") under Node, on a fake localStorage:
//   node tools/profilestest.js
// Per-game settings (a game's own value wins, "game" is a value, null removes it), the site's players (names unique whatever
// the case, the current one, rename, remove), what a game asks (forGame), what it reports (report), the operations waiting for
// it (queue, and left the queue by a report), the match of a site player with a game's names, and blocked storage.
// It prints "profilestest: N checks, M failed" and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");

function load(blocked) {
  const data = {};
  const storage = blocked ? null : {
    getItem: (k) => (Object.prototype.hasOwnProperty.call(data, k) ? data[k] : null),
    setItem: (k, v) => { data[k] = String(v); },
    removeItem: (k) => { delete data[k]; },
  };
  const window = { localStorage: storage, addEventListener() {} };
  if (blocked) Object.defineProperty(window, "localStorage", { get() { throw new Error("blocked"); } });
  const ctx = { window, document: {}, console };
  vm.createContext(ctx);
  for (const f of ["settings.js", "profiles.js"]) {
    vm.runInContext(fs.readFileSync(path.join(__dirname, "..", "web", f), "utf8"), ctx, { filename: f });
  }
  vm.runInContext("this.S = ByopopSettings; this.P = ByopopProfiles; this.G = ByopopGameFiles;", ctx);
  return { S: ctx.S, P: ctx.P, G: ctx.G, data };
}

let checks = 0, failed = 0;
function check(name, ok, detail) {
  checks++;
  if (!ok) { failed++; console.log("FAIL " + name + (detail !== undefined ? ": " + JSON.stringify(detail) : "")); }
}
function same(name, a, b) { check(name, JSON.stringify(a) === JSON.stringify(b), { got: a, want: b }); }

// ---------- settings: site-wide ----------
{
  const { S, data } = load();
  same("default profile mode", S.get("profile"), "game");
  check("profile bad value", S.set("profile", "yes") === false);
  check("profile create", S.set("profile", "create") && S.get("profile") === "create");
  check("profile default removes the field", S.set("profile", "game") && !("byopop-settings" in data));
  same("volume off by default", S.get("startVolume"), "off");
  same("level default", [S.get("startMusic"), S.get("startSound")], [50, 50]);
  check("level 40.4 rounds", S.set("startMusic", "40.4") && S.get("startMusic") === 40);
  check("level 101 refused", S.set("startMusic", 101) === false && S.get("startMusic") === 40);
  check("a level is not 'game'", S.set("startMusic", "game") === false && S.set("startSound", "") === false);
  check("level 0 is a level", S.set("startSound", 0) && S.get("startSound") === 0);
  check("the default level removes the field", S.set("startMusic", 50) && S.set("startSound", 50) && !("byopop-settings" in data), data);
  check("volume switch", S.set("startVolume", "on") && S.get("startVolume") === "on" && S.set("startVolume", "loud") === false);
  same("fullscreen choices", ["game", "on", "off"].map((v) => S.set("fullscreen", v) && S.get("fullscreen")), ["game", "on", "off"]);
  same("cursors choices", ["on", "off", "game"].map((v) => S.set("cursors", v) && S.get("cursors")), ["on", "off", "game"]);
  check("unknown setting", S.set("nonsense", "on") === false && S.get("nonsense") === null);
  // the chat (web/chat.js) is the one switch that is on until it is turned off
  same("chat: shown by default", [S.get("showChat"), S.all().showChat], ["on", "on"]);
  check("chat: off is stored", S.set("showChat", "off") && S.get("showChat") === "off" && JSON.parse(data["byopop-settings"]).showChat === "off");
  check("chat: on is the default, so its field goes", S.set("showChat", "on") && S.get("showChat") === "on" && !("showChat" in JSON.parse(data["byopop-settings"])));
  check("chat: only on and off", S.set("showChat", "game") === false && S.set("showChat", "yes") === false && S.set("showChat", 1) === false && S.get("showChat") === "on");
  check("chat: one game can turn it off, the others keep it",
    S.set("showChat", "off", "pixelus") && S.get("showChat", "pixelus") === "off" && S.get("showChat", "bejeweled") === "on" && S.get("showChat") === "on");
  check("chat: one game can turn it on while the site has it off",
    S.set("showChat", "off") && S.set("showChat", "on", "pixelus") && S.get("showChat", "pixelus") === "on" && S.get("showChat", "bejeweled") === "off");
  check("chat: removing a game's value follows the site", S.set("showChat", null, "pixelus") && S.get("showChat", "pixelus") === "off");
  S.set("showChat", "on");
  check("chat: back on leaves no field and no game entry", !("showChat" in JSON.parse(data["byopop-settings"])) && !("games" in JSON.parse(data["byopop-settings"])), data["byopop-settings"]);
}

// ---------- settings: per game ----------
{
  const { S, data } = load();
  S.set("fullscreen", "on");
  same("a game without its own value follows the site", S.get("fullscreen", "pixelus"), "on");
  check("game's own value", S.set("fullscreen", "game", "pixelus"));
  same("the game's own default wins over the site's", S.get("fullscreen", "pixelus"), "game");
  same("the site is unchanged", S.get("fullscreen"), "on");
  same("another game follows the site", S.get("fullscreen", "bejeweled"), "on");
  same("override()", S.override("fullscreen", "pixelus"), "game");
  same("override() of a game without one", S.override("fullscreen", "bejeweled"), null);
  S.set("startMusic", 30, "pixelus");
  same("level override", S.get("startMusic", "pixelus"), 30);
  same("the other level follows the site", S.get("startSound", "pixelus"), 50);
  check("level override refuses 'game'", S.set("startMusic", "game", "pixelus") === false && S.get("startMusic", "pixelus") === 30);
  same("overrides()", S.overrides("pixelus").sort(), ["fullscreen", "startMusic"]);
  check("bad game id refused", S.set("fullscreen", "on", "../x") === false && S.set("fullscreen", "on", "") === false);
  check("bad game value refused", S.set("fullscreen", "loud", "pixelus") === false);
  check("a site-wide write keeps the games", (S.set("cursors", "on"), S.get("fullscreen", "pixelus") === "game"));
  check("null removes", S.set("fullscreen", null, "pixelus") && S.override("fullscreen", "pixelus") === null);
  S.set("startMusic", null, "pixelus");
  check("an empty game leaves no field", !("games" in JSON.parse(data["byopop-settings"])), data["byopop-settings"]);
  const all = S.all("pixelus");
  same("all() with a game", [all.fullscreen, all.cursors], ["on", "on"]);
  // fields of the item this file does not know are kept
  data["byopop-settings"] = JSON.stringify({ fullscreen: "on", future: 7, games: { bejeweled: { fullscreen: "off", junk: 1 } } });
  S.set("wakeLock", "on");
  const kept = JSON.parse(data["byopop-settings"]);
  check("unknown fields kept", kept.future === 7 && kept.games.bejeweled.junk === 1 && kept.wakeLock === "on", kept);
  same("a stored game value is read", S.get("fullscreen", "bejeweled"), "off");
  data["byopop-settings"] = JSON.stringify({ fullscreen: "sideways", startMusic: "loud", games: { bejeweled: { fullscreen: "huge", startSound: 700 } } });
  same("invalid stored values are ignored", [S.get("fullscreen"), S.get("startMusic"), S.get("fullscreen", "bejeweled"), S.get("startSound", "bejeweled")], ["game", 50, "game", 50]);
}

// ---------- the old volume settings (mute, musicVolume, sfxVolume) become the starting volume ----------
{
  const old = (o) => { const d = load(); d.data["byopop-settings"] = JSON.stringify(o); return d; };
  const vol = (S, g) => [S.get("startVolume", g), S.get("startMusic", g), S.get("startSound", g)];
  const stored = (d) => JSON.parse(d.data["byopop-settings"] || "{}");
  let t = old({ mute: "on", fullscreen: "on" });
  same("mute becomes the volume at 0 and 0", vol(t.S), ["on", 0, 0]);
  same("the old fields are removed from the item", Object.keys(stored(t)).sort(), ["fullscreen", "startMusic", "startSound", "startVolume"]);
  t = old({ musicVolume: 30, sfxVolume: 80 });
  same("volumes become the volume at those levels", vol(t.S), ["on", 30, 80]);
  same("and nothing old is left", Object.keys(stored(t)).sort(), ["startMusic", "startSound", "startVolume"]);
  t = old({ musicVolume: 30, sfxVolume: "game" });
  same("a level left to the game becomes 50", vol(t.S), ["on", 30, 50]);
  t = old({ sfxVolume: "0" });
  same("a stored 0 is a level", vol(t.S), ["on", 50, 0]);
  t = old({ mute: "on", musicVolume: 60, sfxVolume: 60 });
  same("mute wins over volumes", vol(t.S), ["on", 0, 0]);
  t = old({ mute: "game", musicVolume: "game", sfxVolume: "game", wakeLock: "on" });
  same("settings left to the game leave no volume", [vol(t.S), stored(t)], [["off", 50, 50], { wakeLock: "on" }]);
  t = old({ mute: "yes", musicVolume: 999 });
  same("invalid old values are dropped", [vol(t.S), "byopop-settings" in t.data], [["off", 50, 50], false]);
  t = old({ mute: "on", games: { pixelus: { mute: "game" }, bejeweled: { musicVolume: 20 }, other: { fullscreen: "on", mute: "on" } } });
  same("a game left alone while the site is muted keeps its own off", [vol(t.S, "pixelus"), t.S.override("startVolume", "pixelus")], [["off", 0, 0], "off"]);
  same("a game with a level still follows the site's mute", vol(t.S, "bejeweled"), ["on", 0, 0]);
  same("a game's other settings stay", [t.S.get("fullscreen", "other"), vol(t.S, "other")], ["on", ["on", 0, 0]]);
  check("no old field is left in any game", !/"mute"|musicVolume|sfxVolume/.test(t.data["byopop-settings"]), t.data["byopop-settings"]);
  t = old({ musicVolume: 30, games: { pixelus: { mute: "on" }, bejeweled: { sfxVolume: 10 } } });
  same("a game's own mute is its own volume", vol(t.S, "pixelus"), ["on", 0, 0]);
  same("a game's own level falls back on the site's other", vol(t.S, "bejeweled"), ["on", 30, 10]);
  same("the site's own", vol(t.S), ["on", 30, 50]);
  // read again: nothing more to migrate
  const before = t.data["byopop-settings"];
  t.S.get("startVolume");
  check("migrating twice changes nothing", t.data["byopop-settings"] === before);
  // the migration is read-only when the new fields are there
  t = old({ startVolume: "on", startMusic: 10, startSound: 20 });
  same("new fields are read as they are", vol(t.S), ["on", 10, 20]);
}

// ---------- the site's players ----------
{
  const { P } = load();
  same("no players", P.list(), []);
  check("current is null", P.current() === null);
  const a = P.add("  Alice   Smith ");
  check("add cleans the name", a.ok && a.profile.name === "Alice Smith", a);
  check("the first is current", P.current() && P.current().name === "Alice Smith");
  check("duplicate (other case) refused", P.add("alice smith").ok === false);
  check("empty refused", P.add("   ").ok === false);
  const b = P.add("Bob");
  check("second added", b.ok && P.list().length === 2);
  check("current stays", P.current().id === a.profile.id);
  check("setCurrent", P.setCurrent(b.profile.id).ok && P.current().name === "Bob");
  check("rename", P.rename(b.profile.id, "Robert").ok && P.find("robert").id === b.profile.id);
  check("rename to another's name refused", P.rename(b.profile.id, "ALICE SMITH").ok === false);
  check("rename to its own name in another case", P.rename(b.profile.id, "ROBERT").ok && P.find("Robert").name === "ROBERT");
  check("long name cut", P.add("x".repeat(80)).profile.name.length === P.NAME_MAX);
  check("control characters become spaces", P.add("a\u0007b").profile.name === "a b");
  check("remove the current falls to the first", P.remove(b.profile.id).ok && P.current().name === "Alice Smith");
  check("remove a gone one fails", P.remove(b.profile.id).ok === false);
  check("setCurrent of a gone one fails", P.setCurrent("nope").ok === false);
  check("setCurrent('') clears it", P.setCurrent("").ok && P.current() === null);
}

// ---------- forGame, report, operations ----------
{
  const { S, P, data } = load();
  P.add("Alice");
  P.add("Bob");
  let g = P.forGame("pixelus");
  same("forGame, default mode", [g.mode, g.name, g.profiles], ["game", "Alice", ["Alice", "Bob"]]);
  S.set("profile", "fill");
  S.set("profile", "create", "pixelus");
  same("forGame, a game's own mode", [P.forGame("pixelus").mode, P.forGame("bejeweled").mode], ["create", "fill"]);

  const id1 = P.queue("pixelus", { op: "rename", from: "Alice", to: "Alicia" });
  const id2 = P.queue("pixelus", { op: "delete", name: "Bob" });
  const id3 = P.queue("bookworm", { op: "rename", from: "Al", to: "Alex" });
  check("queue gives numbers", id1 > 0 && id2 > id1 && id3 > id2, [id1, id2, id3]);
  check("queue refuses an empty operation", P.queue("pixelus", { op: "rename", from: "", to: "x" }) === 0);
  same("forGame carries the operations", P.forGame("pixelus").ops.map((o) => o.id), [id1, id2]);

  check("report", P.report("pixelus", {
    kind: "profiles", names: ["Alice", "Bob", "Carol"], current: "Alice", fits: { Alice: "Alice", Bob: "Bob" }, rules: "Letters",
    max: 0, results: [{ id: id1, status: "done", note: "" }, { id: id2, status: "skipped", note: "no" }],
  }));
  same("the operations left the queue", P.forGame("pixelus").ops, []);
  same("another game's operations stay", P.ops("bookworm").length, 1);
  const rec = P.games().pixelus;
  same("results kept with the operation's fields", rec.results.map((r) => [r.id, r.op, r.status, r.from, r.to, r.name]),
    [[id1, "rename", "done", "Alice", "Alicia", undefined], [id2, "delete", "skipped", undefined, undefined, "Bob"]]);
  check("record fields", rec.kind === "profiles" && rec.names.length === 3 && rec.canRename === true && rec.canDelete === true);
  P.report("pixelus", { kind: "scores", names: ["x"], canDelete: true });
  check("scores cannot delete", P.games().pixelus.canDelete === false);
  check("results survive a later report", P.games().pixelus.results.length === 2);
  check("a bad game id is refused", P.report("../evil", { names: [] }) === false);
  check("an op that was cancelled is gone", (P.cancel("bookworm", id3), P.ops("bookworm").length === 0));
  check("clearResults forgets the results and keeps the names", P.clearResults() && P.games().pixelus.results.length === 0 && P.games().pixelus.names.length === 1);

  const r = { names: ["alice", "Bobby", "Zed"], fits: { Alice: "Alice", Bob: "Bobby", Zed: "" } };
  same("match, other case", P.match(r, "Alice"), { name: "alice", exact: false });
  same("match, by what the game's rules make of it", P.match(r, "Bob"), { name: "Bobby", exact: false });
  same("match, exact", P.match({ names: ["Zed"], fits: {} }, "Zed"), { name: "Zed", exact: true });
  check("no match", P.match(r, "Carol") === null);
  check("storage holds only our keys", Object.keys(data).every((k) => /^byopop-/.test(k)), Object.keys(data));
}

// ---------- blocked storage ----------
{
  const { S, P, G } = load(true);
  same("settings read with blocked storage", [S.get("fullscreen"), S.get("startVolume"), S.get("startMusic")], ["game", "off", 50]);
  check("settings write says no", S.set("fullscreen", "on") === false);
  check("players: add says no", P.add("Alice").ok === false);
  same("players: empty", P.list(), []);
  check("report says no", P.report("pixelus", { names: [] }) === false);
  same("forGame still answers", P.forGame("pixelus").mode, "game");
  same("game files: empty", G.all(), {});
}

console.log("profilestest: " + checks + " checks, " + failed + " failed");
process.exit(failed ? 1 : 0);
