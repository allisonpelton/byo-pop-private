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
  bejeweled: "Bejeweled Deluxe", tiptop: "TipTop Deluxe", mahjongjapan: "Mahjong Escape: Ancient Japan",
  mahjongchina: "Mahjong Escape: Ancient China", bookwormadv: "Bookworm Adventures Deluxe", bookwormadv2: "Bookworm Adventures Volume 2",
};
const warned = [];
ctx.console = { warn: (...a) => warned.push(a.join(" ")), log: console.log };
for (const target of finished) {
  const js = fs.readFileSync(path.join(root, "games", target, "web", "page.js"), "utf8");
  vm.runInContext(`ByopopImport.add(${JSON.stringify(target)}, ${JSON.stringify(titles[target] || target)}, function () {\n${js}\n  return PAGE;\n});`, ctx);
}
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
check("loose files", brief(I.sort(files("TipTop.dat", "TipTop.exe"))), { found: [["(the dropped files)", "tiptop", "", ["tiptop"]]], rejected: [] });
const both = ["resource.dll", "MahjongEscapeAJ.exe", "MahjongEscapeAC.exe"];
check("two games at one place, told by the name", brief(I.sort(files(...both.map((f) => "Mahjong Escape Ancient China/" + f)))).found[0][1], "mahjongchina");
check("two games at one place, left to the player", brief(I.sort(files(...both.map((f) => "Mahjong/" + f)))).found[0].slice(1),
  [null, "Mahjong/", ["mahjongjapan", "mahjongchina"]]);
check("a folder of games with one game in it", brief(I.sort(files("PopCap Games/TipTop Deluxe/TipTop.dat", "PopCap Games/TipTop Deluxe/TipTop.exe",
  "PopCap Games/Common/readme.txt"))), { found: [["TipTop Deluxe", "tiptop", "PopCap Games/TipTop Deluxe/", ["tiptop"]]], rejected: ["Common"] });
check("a game's own folder with the game a level down", brief(I.sort(files("Bejeweled/Game/images/gemsheet6.png"))).found[0].slice(0, 3),
  ["Bejeweled", "bejeweled", "Bejeweled/Game/"]);
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
