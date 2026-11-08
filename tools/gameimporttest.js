// Checks the import rules shared by the game pages and the listing page (web/gameimport.js; docs/WEB_PLAYER.md, "Adding games from
// the listing page") under Node, with the real games' page.js files registered as the build's import-games.js registers them:
//   node tools/gameimporttest.js
// The game root (markers in any case, at any depth), the files picked (the game's folders in lower case, its top files, not the .sig
// files), the markers check after a pick (a pak marker stands for the game's folders), and the sorting of dropped folders into games:
// by markers, a folder of many games looked into, a folder with no game rejected, two games at one place told apart by the folder's
// name or left to the player.
// It prints "gameimporttest: N checks, M failed" and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const root = path.join(__dirname, "..");
const ctx = { window: {}, console };
vm.createContext(ctx);
require("./stringsload").into(ctx);
vm.runInContext(fs.readFileSync(path.join(root, "web", "gameimport.js"), "utf8"), ctx, { filename: "gameimport.js" });
// every finished game (BYOPOP_WEB_FINISHED in cmake/ByopopWeb.cmake), as the listing registers them; the titles the checks
// below need, and the target as the title of the others
const cmake = fs.readFileSync(path.join(root, "cmake", "ByopopWeb.cmake"), "utf8");
const finished = /\nset\(BYOPOP_WEB_FINISHED([^)]*)\)/.exec(cmake)[1].trim().split(/\s+/);
const titles = {
  bejeweled: "Bejeweled", tiptop: "TipTop", mahjongjapan: "Mahjong Escape: Ancient Japan",
  mahjongchina: "Mahjong Escape: Ancient China", bookwormadv: "Bookworm Adventures", bookwormadv2: "Bookworm Adventures Volume 2",
};
const warned = [];
ctx.console = { warn: (...a) => warned.push(a.join(" ")), log: console.log };
for (const target of finished) {
  const js = fs.readFileSync(path.join(root, "games", target, "web", "page.js"), "utf8");
  vm.runInContext(`ByopopImport.add(${JSON.stringify(target)}, ${JSON.stringify(titles[target] || target)}, function () {\n${js}\n  return PAGE;\n});`, ctx);
}
// the games the site recognises but does not play, as other-games.js holds them (cmake/ByopopOthers.cmake)
const others = /\nset\(BYOPOP_OTHER_GAMES([^)]*)\)/.exec(fs.readFileSync(path.join(root, "cmake", "ByopopOthers.cmake"), "utf8"))[1]
  .match(/"[^"]*"/g).map((q) => {
    const [id, title, markers, project, url] = q.slice(1, -1).split("|");
    return { id, title, markers: markers.split(","), project: project || "", url: url || "" };
  });
vm.runInContext(`var ByopopOtherGames = ${JSON.stringify(others)};`, ctx);
const I = vm.runInContext("ByopopImport", ctx);
const spec = (t) => I.games.filter((g) => g.target === t)[0].spec;

let checks = 0, failed = 0;
function check(what, got, want) {
  checks++;
  const g = JSON.stringify(got), w = JSON.stringify(want);
  if (g !== w) { failed++; console.log(`FAIL ${what}: got ${g}, want ${w}`); }
}
const files = (...paths) => paths.map((p) => ({ path: p, read: () => Promise.resolve(new Uint8Array(0)) }));
const safeRel = (p) => { const parts = String(p).split("/").filter((x) => x && x !== "."); return parts.some((x) => x === "..") || !parts.length ? null : parts.join("/"); };

check("every finished game registered", I.games.map((g) => g.target), finished);
check("no page.js failed", warned, []);
// the game root
check("root at the top", I.findRoot(spec("bejeweled"), files("Bejeweled/images/GemSheet6.png", "Bejeweled/x.txt"), false), "Bejeweled/");
check("root deeper", I.findRoot(spec("tiptop"), files("a/b/TipTop.dat", "a/b/TIPTOP.EXE"), false), "a/b/");
check("root needs every marker", I.findRoot(spec("tiptop"), files("a/TipTop.dat"), false), null);
check("markers in one folder", I.findRoot(spec("tiptop"), files("a/TipTop.dat", "b/TipTop.exe"), false), null);
// the error names only what the best folder lacks, in markerText's casing, as a list
check("missing: the one marker a folder lacks", I.missingMarkers(spec("tiptop"), files("a/TipTop.dat", "a/readme.txt")), "TipTop.exe");
check("missing: the folder with the most markers", I.missingMarkers(spec("bookworm"), files("x/wordlist.txt", "y/wordlist.txt", "y/music/bookworm1.mo3")), "Bookworm.exe");
check("missing: all of them, as a list", I.missingMarkers(spec("tiptop"), files("a/other.txt")), "TipTop.dat and TipTop.exe");
check("missing: a marker of any type, named without it", I.missingMarkers(spec("bejeweled"), files("Bejeweled/x.txt")), "images/gemsheet6");
check("missing: a pak in dropped files is a file to find", I.missingMarkers(spec("bookwormadv"), files("B/BookwormAdventures.exe", "B/Images/a.png")), "main.pak");
check("missing: a pak game's exe, any case", I.missingMarkers(spec("bookwormadv"), files("B/MAIN.PAK", "B/other.exe")), "BookwormAdventures.exe");
check("missing in a known root",I.missingIn(spec("tiptop"), ["TipTop.exe"]), "TipTop.dat");
// the files picked
const picked = I.pick(spec("bejeweled"), files("B/Images/gemsheet6.png", "B/images/x.sig", "B/Bejeweled.exe", "B/readme.txt", "B/Music/m.mo3"), "B/", safeRel)
  .map((f) => f.rel);
check("pick: folders in lower case, no .sig, nothing else", picked, ["images/gemsheet6.png", "music/m.mo3"]);
check("pick: top files", I.pick(spec("tiptop"), files("T/TipTop.dat", "T/TipTop.exe", "T/unins000.exe"), "T/", safeRel).map((f) => f.rel),
  ["TipTop.dat", "TipTop.exe"]);
check("markers after the pick", I.hasMarkers(spec("bejeweled"), picked), true);
check("markers missing after the pick", I.hasMarkers(spec("bejeweled"), ["music/m.mo3"]), false);
check("a pak marker: any game folder", I.hasMarkers(spec("bookwormadv"), ["images/a.png", "BookwormAdventures.exe"]), true);
check("a pak marker: no game folder", I.hasMarkers(spec("bookwormadv"), ["BookwormAdventures.exe"]), false);
// sorting
const brief = (r) => ({ found: r.found.map((f) => [f.folder, f.game ? f.game.target : null, f.root, f.choices.map((g) => g.target)]), rejected: r.rejected });
check("one game's folder", brief(I.sort(files("Bejeweled Deluxe/images/gemsheet6.png"))),
  { found: [["Bejeweled Deluxe", "bejeweled", "Bejeweled Deluxe/", ["bejeweled"]]], rejected: [] });
check("a folder of games", brief(I.sort(files("PopCap Games/TipTop Deluxe/TipTop.dat", "PopCap Games/TipTop Deluxe/TipTop.exe",
  "PopCap Games/Mahjong Escape Ancient Japan/resource.dll", "PopCap Games/Mahjong Escape Ancient Japan/MahjongEscapeAJ.exe",
  "PopCap Games/Common/readme.txt"))),
  { found: [["TipTop Deluxe", "tiptop", "PopCap Games/TipTop Deluxe/", ["tiptop"]],
    ["Mahjong Escape Ancient Japan", "mahjongjapan", "PopCap Games/Mahjong Escape Ancient Japan/", ["mahjongjapan"]]], rejected: ["Common"] });
check("no game", brief(I.sort(files("Photos/a.jpg", "Photos/b/c.jpg"))), { found: [], rejected: ["Photos"] });
check("loose files", brief(I.sort(files("TipTop.dat", "TipTop.exe"))), { found: [["import.dropped_files", "tiptop", "", ["tiptop"]]], rejected: [] });
const both = ["resource.dll", "MahjongEscapeAJ.exe", "MahjongEscapeAC.exe"];
check("two games at one place, told by the name", brief(I.sort(files(...both.map((f) => "Mahjong Escape Ancient China/" + f)))).found[0][1], "mahjongchina");
check("two games at one place, left to the player", brief(I.sort(files(...both.map((f) => "Mahjong/" + f)))).found[0].slice(1),
  [null, "Mahjong/", ["mahjongjapan", "mahjongchina"]]);
check("a folder of games with one game in it", brief(I.sort(files("PopCap Games/TipTop Deluxe/TipTop.dat", "PopCap Games/TipTop Deluxe/TipTop.exe",
  "PopCap Games/Common/readme.txt"))), { found: [["TipTop Deluxe", "tiptop", "PopCap Games/TipTop Deluxe/", ["tiptop"]]], rejected: ["Common"] });
check("a game's own folder with the game a level down", brief(I.sort(files("Bejeweled/Game/images/gemsheet6.png"))).found[0].slice(0, 3),
  ["Bejeweled", "bejeweled", "Bejeweled/Game/"]);
// games the site does not play: named, with another project's link or "coming soon"
const otherBrief = (r) => ({ found: r.found.map((f) => f.game ? f.game.target : null), rejected: r.rejected, others: r.others.map((o) => [o.folder, o.game.id]) });
check("others: the table's rows are well formed", others.every((o) => /^[a-z0-9]+$/.test(o.id) && o.title && o.markers.length >= 2 &&
  (o.project ? /^https:\/\/github\.com\/[^/]+\/[^/]+$/.test(o.url) : !o.url)), true);
check("others: one game's folder", otherBrief(I.sort(files("Zuma Deluxe/levels/claw/claw.dat", "Zuma Deluxe/levels/levels.xml", "Zuma Deluxe/Zuma.exe"))),
  { found: [], rejected: [], others: [["Zuma Deluxe", "zuma"]] });
check("others: a folder of them, with nothing else", otherBrief(I.sort(files("Steam/Zuma Deluxe/levels/claw/claw.dat", "Steam/Zuma Deluxe/levels/levels.xml",
  "Steam/Chuzzle Deluxe/data/mindbenders.txt", "Steam/Chuzzle Deluxe/data/gamedata.cfg", "Steam/Common/x.txt"))),
  { found: [], rejected: ["Common"], others: [["Zuma Deluxe", "zuma"], ["Chuzzle Deluxe", "chuzzle"]] });
check("others: beside one of the site's games", otherBrief(I.sort(files("PopCap Games/TipTop Deluxe/TipTop.dat", "PopCap Games/TipTop Deluxe/TipTop.exe",
  "PopCap Games/Plants Vs Zombies/PlantsVsZombies.exe", "PopCap Games/Plants Vs Zombies/main.pak"))),
  { found: ["tiptop"], rejected: [], others: [["Plants Vs Zombies", "plantsvszombies"]] });
check("others: a main.pak game told by its exe", I.otherGame(files("P/PeggleNights.exe", "P/main.pak")).id, "pegglenights");
check("others: a pak game the site plays is not one of them", otherBrief(I.sort(files("B/BookwormAdventures.exe", "B/main.pak"))),
  { found: ["bookwormadv"], rejected: [], others: [] });
check("others: the exe alone is not enough", I.otherGame(files("Z/levels/levels.xml")), null);
check("others: nothing of theirs", I.otherGame(files("Photos/a.jpg")), null);
// what the import says: a tiny document, enough for otherNote
const doc = {
  createElement: (tag) => ({ tag, attrs: {}, kids: [], textContent: "", setAttribute(k, v) { this.attrs[k] = v; }, appendChild(c) { this.kids.push(c); return c; } }),
  createTextNode: (t) => ({ text: t }),
};
const zuma = others.filter((o) => o.id === "zuma")[0], venice = others.filter((o) => o.id === "venice")[0];
const note = I.otherNote(doc, zuma), link = note.kids[1];
check("others: the link to the project", [link.tag, link.attrs.href, link.attrs.target, link.attrs.rel, link.textContent],
  ["a", "https://github.com/alula/CircleShootApp", "_blank", "noopener noreferrer", "CircleShootApp"]);
check("others: the link's name says it opens a tab, and starts with its text", link.attrs["aria-label"].indexOf("CircleShootApp ") === 0, true);
check("others: the game named before the link", note.kids[0].text.indexOf("Zuma Deluxe") >= 0, true);
check("others: coming soon has no link", [I.otherNote(doc, venice).kids.length, I.otherNote(doc, venice).textContent.indexOf("Venice") >= 0], [0, true]);
check("others: a link that is not a GitHub project's page is not made", I.otherNote(doc, { title: "X", project: "P", url: "javascript:alert(1)" }).kids.length, 0);
// the order the files are written in: the markers last, and for a pak marker the game's folders before them
const rels = (list) => list.map((r) => ({ rel: r }));
check("order: markers last", I.ordered(spec("tiptop"), rels(["TipTop.exe", "x/y.txt", "TipTop.dat"])).map((f) => f.rel),
  ["x/y.txt", "TipTop.exe", "TipTop.dat"]);
check("order: a pak marker's game folders before the markers", I.ordered(spec("bookwormadv"),
  rels(["BookwormAdventures.exe", "images/a.png", "readme.txt", "sounds/b.ogg"])).map((f) => f.rel),
  ["readme.txt", "images/a.png", "sounds/b.ogg", "BookwormAdventures.exe"]);
// the folder's name
check("name: the same", I.nameScore("TipTop Deluxe", I.games.filter((g) => g.target === "tiptop")[0]), 3);
check("name: held", I.nameScore("PopCap TipTop Deluxe v1.0", I.games.filter((g) => g.target === "tiptop")[0]), 2);
check("name: neither", I.nameScore("Games", I.games.filter((g) => g.target === "tiptop")[0]), 0);

console.log(`gameimporttest: ${checks} checks, ${failed} failed`);
process.exit(failed ? 1 : 0);
