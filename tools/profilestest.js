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
  same("level default", S.get("musicVolume"), "game");
  check("level 40.4 rounds", S.set("musicVolume", "40.4") && S.get("musicVolume") === 40);
  check("level 101 refused", S.set("musicVolume", 101) === false && S.get("musicVolume") === 40);
  check("unknown setting", S.set("nonsense", "on") === false && S.get("nonsense") === null);
}

// ---------- settings: per game ----------
{
  const { S, data } = load();
  S.set("mute", "on");
  same("a game without its own value follows the site", S.get("mute", "pixelus"), "on");
  check("game's own value", S.set("mute", "game", "pixelus"));
  same("the game's own default wins over the site's", S.get("mute", "pixelus"), "game");
  same("the site is unchanged", S.get("mute"), "on");
  same("another game follows the site", S.get("mute", "bejeweled"), "on");
  same("override()", S.override("mute", "pixelus"), "game");
  same("override() of a game without one", S.override("mute", "bejeweled"), null);
  S.set("musicVolume", 30, "pixelus");
  same("level override", S.get("musicVolume", "pixelus"), 30);
  S.set("musicVolume", "game", "pixelus");
  same("level override of 'game'", S.get("musicVolume", "pixelus"), "game");
  same("overrides()", S.overrides("pixelus").sort(), ["musicVolume", "mute"]);
  check("bad game id refused", S.set("mute", "on", "../x") === false && S.set("mute", "on", "") === false);
  check("bad game value refused", S.set("mute", "loud", "pixelus") === false);
  check("a site-wide write keeps the games", (S.set("fullscreen", "on"), S.get("mute", "pixelus") === "game"));
  check("null removes", S.set("mute", null, "pixelus") && S.override("mute", "pixelus") === null);
  S.set("musicVolume", null, "pixelus");
  check("an empty game leaves no field", !("games" in JSON.parse(data["byopop-settings"])), data["byopop-settings"]);
  const all = S.all("pixelus");
  same("all() with a game", [all.mute, all.fullscreen], ["on", "on"]);
  // fields of the item this file does not know are kept
  data["byopop-settings"] = JSON.stringify({ mute: "on", future: 7, games: { bejeweled: { mute: "off", junk: 1 } } });
  S.set("wakeLock", "on");
  const kept = JSON.parse(data["byopop-settings"]);
  check("unknown fields kept", kept.future === 7 && kept.games.bejeweled.junk === 1 && kept.wakeLock === "on", kept);
  same("invalid stored game values are ignored", S.get("mute", "bejeweled"), "off");
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
  same("settings read with blocked storage", S.get("mute"), "game");
  check("settings write says no", S.set("mute", "on") === false);
  check("players: add says no", P.add("Alice").ok === false);
  same("players: empty", P.list(), []);
  check("report says no", P.report("pixelus", { names: [] }) === false);
  same("forGame still answers", P.forGame("pixelus").mode, "game");
  same("game files: empty", G.all(), {});
}

console.log("profilestest: " + checks + " checks, " + failed + " failed");
process.exit(failed ? 1 : 0);
