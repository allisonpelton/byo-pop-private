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
const waits = [];

const root = path.join(__dirname, "..");
const ctx = { window: {}, console, setTimeout, clearTimeout, MessageChannel };
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
// a pak marker: the pak, or the game's folders with it already unpacked (a staged copy, a zip of the unpacked game)
check("root: a pak game with its pak", I.findRoot(spec("bookwormadv"), files("B/main.pak", "B/BookwormAdventures.exe"), false), "B/");
check("root: a pak game unpacked", I.findRoot(spec("bookwormadv"), files("x/readme.txt", "B/BookwormAdventures.exe", "B/Images/a.png"), false), "B/");
check("root: a pak game at the top, unpacked", I.findRoot(spec("bookwormadv"), files("bookwormadventures.exe", "images/a.png", "data/b.txt"), false), "");
check("root: a pak game with neither", I.findRoot(spec("bookwormadv"), files("B/BookwormAdventures.exe", "B/readme.txt"), false), null);
check("root: the unpacked folders beside another exe", I.findRoot(spec("bookwormadv"), files("A/BookwormAdventures.exe", "B/images/a.png"), false), null);
// the error names only what the best folder lacks, in markerText's casing, as a list
check("missing: the one marker a folder lacks", I.missingMarkers(spec("tiptop"), files("a/TipTop.dat", "a/readme.txt")), "TipTop.exe");
check("missing: the folder with the most markers", I.missingMarkers(spec("bookworm"), files("x/wordlist.txt", "y/wordlist.txt", "y/music/bookworm1.mo3")), "Bookworm.exe");
check("missing: all of them, as a list", I.missingMarkers(spec("tiptop"), files("a/other.txt")), "TipTop.dat and TipTop.exe");
check("missing: a marker of any type, named without it", I.missingMarkers(spec("bejeweled"), files("Bejeweled/x.txt")), "images/gemsheet6");
check("missing: a pak in dropped files is a file to find", I.missingMarkers(spec("bookwormadv"), files("B/BookwormAdventures.exe", "B/Images/a.png")), "main.pak");
check("missing: a pak game's exe, any case", I.missingMarkers(spec("bookwormadv"), files("B/MAIN.PAK", "B/other.exe")), "BookwormAdventures.exe");
check("missing in a known root",I.missingIn(spec("tiptop"), ["TipTop.exe"]), "TipTop.dat");
// the files picked
const picked = I.pick(spec("bejeweled"), files("B/Images/gemsheet6.png", "B/images/x.sig", "B/Bejeweled.exe", "B/ReadMe.html", "B/license.txt", "B/Music/m.mo3"), "B/", safeRel)
  .map((f) => f.rel);
// the game's own readme is kept, in lower case, for the desktop's Help menu (web/deskapps.js); other loose files are not
check("pick: folders in lower case, no .sig, the readme, nothing else", picked, ["images/gemsheet6.png", "readme.html", "music/m.mo3"]);
check("pick: top files", I.pick(spec("tiptop"), files("T/TipTop.dat", "T/TipTop.exe", "T/unins000.exe"), "T/", safeRel).map((f) => f.rel),
  ["TipTop.dat", "TipTop.exe"]);
check("markers after the pick", I.hasMarkers(spec("bejeweled"), picked), true);
check("markers missing after the pick", I.hasMarkers(spec("bejeweled"), ["music/m.mo3"]), false);
check("a pak marker: any game folder", I.hasMarkers(spec("bookwormadv"), ["images/a.png", "BookwormAdventures.exe"]), true);
check("a pak marker: no game folder", I.hasMarkers(spec("bookwormadv"), ["BookwormAdventures.exe"]), false);
// A pak marker whose game's folders have common names (Venice's: config.xml beside gfx, levels...): its unpacked copy is known
// by the files only its pak holds (PAGE.unpacked), never by a folder's name (the user, 2026-11-16: "Fix it now")
{
  const v = spec("venice"), own = v.unpacked.map((f) => "V/" + f);
  check("venice: the page names its pak's own files", own.length >= 2, true);
  for (const d of v.gameDirs)
    check(`venice: another game's config.xml beside ${d}/ is not Venice`, I.findRoot(v, files("G/config.xml", `G/${d}/a.png`), false), null);
  check("venice: another game's config.xml beside all its folders is not Venice",
    I.findRoot(v, files("G/config.xml", ...v.gameDirs.map((d) => `G/${d}/a.xml`)), false), null);
  check("venice: one of its pak's files is not enough", I.findRoot(v, files("G/config.xml", own[0].replace("V/", "G/"), "G/gfx/a.png"), false), null);
  check("venice: its install, with main.pak", I.findRoot(v, files("V/main.pak", "V/config.xml", "V/Venice.exe", "V/properties/partner.xml"), false), "V/");
  check("venice: unpacked", I.findRoot(v, files("V/config.xml", ...own, "V/gfx/a.png"), false), "V/");
  check("venice: unpacked, in any case", I.findRoot(v, files("V/Copy/CONFIG.XML", ...v.unpacked.map((f) => "V/Copy/" + f.toUpperCase())), false), "V/Copy/");
  // a dropped .zip's files start with its name without ".zip", its own folder inside it or not
  const install = ["main.pak", "config.xml", "Venice.exe"], unpacked = ["config.xml", ...v.unpacked, "gfx/a.png", "audio/a.ogg"];
  const found = (r) => r.found.map((f) => [f.game ? f.game.target : null, f.root]);
  check("venice: a .zip of the install", found(I.sort(files(...install.map((p) => "Venice/" + p)))), [["venice", "Venice/"]]);
  check("venice: a .zip of the install's folder", found(I.sort(files(...install.map((p) => "Venice/Venice Deluxe/" + p)))), [["venice", "Venice/Venice Deluxe/"]]);
  check("venice: a .zip of the unpacked copy", found(I.sort(files(...unpacked.map((p) => "Venice/" + p)))), [["venice", "Venice/"]]);
  check("venice: a .zip of the unpacked copy's folder", found(I.sort(files(...unpacked.map((p) => "Venice/venice/" + p)))), [["venice", "Venice/venice/"]]);
  const other = I.sort(files("Other Game/config.xml", "Other Game/gfx/a.png", "Other Game/levels/1.xml", "Other Game/Other.exe"));
  check("venice: another game's folder is rejected, not Venice", [found(other), other.rejected], [[], ["Other Game"]]);
  check("venice: picked files unpacked", I.hasMarkers(v, ["config.xml", ...v.unpacked, "gfx/a.png"]), true);
  check("venice: picked files with its folders only", I.hasMarkers(v, ["config.xml", "gfx/a.png", "levels/1.xml"]), false);
  // the other pak games' markers hold their own exe, which no other game has, so any of their folders still stands for the pak
  check("venice: the only pak game naming its pak's files", I.games.filter((g) => g.spec.unpacked).map((g) => g.target), ["venice"]);
}
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
// a marker of alternatives ("a|b"): Typer Shark's exe is WinTS.exe in its Steam copy and TyperShark.exe in the Collection's
if (finished.indexOf("typershark") < 0) {
  const js = fs.readFileSync(path.join(root, "games", "typershark", "web", "page.js"), "utf8");
  vm.runInContext(`ByopopImport.add("typershark", "Typer Shark Deluxe", function () {\n${js}\n  return PAGE;\n});`, ctx);
}
const ts = (exe) => files("TS/data/twords.txt", "TS/Images/TitleScreen.jpg", "TS/sounds/a.ogg").concat(exe ? files("TS/" + exe) : []);
check("either: the Steam copy's exe", I.findRoot(spec("typershark"), ts("WinTS.exe"), false), "TS/");
check("either: the Collection's exe", I.findRoot(spec("typershark"), ts("TyperShark.exe"), false), "TS/");
check("either: neither exe", I.findRoot(spec("typershark"), ts(null), false), null);
check("either: neither exe, named", I.missingMarkers(spec("typershark"), ts(null)), "WinTS.exe or TyperShark.exe");
check("either: picked files", [I.hasMarkers(spec("typershark"), ["data/twords.txt", "images/titlescreen.jpg", "typershark.exe"]),
  I.hasMarkers(spec("typershark"), ["data/twords.txt", "images/titlescreen.jpg", "WINTS.EXE"]),
  I.hasMarkers(spec("typershark"), ["data/twords.txt", "images/titlescreen.jpg", "other.exe"])], [true, true, false]);
check("either: missing in a known root", I.missingIn(spec("typershark"), ["data/twords.txt", "images/titlescreen.jpg"]), "WinTS.exe or TyperShark.exe");
check("either: dropped folders of both copies", brief(I.sort(ts("WinTS.exe").map((f) => ({ path: "Steam/" + f.path, read: f.read }))
  .concat(ts("TyperShark.exe").map((f) => ({ path: "Collection/" + f.path, read: f.read }))).concat(ts(null).map((f) => ({ path: "Bare/" + f.path, read: f.read }))))),
  { found: [["Steam", "typershark", "Steam/TS/", ["typershark"]], ["Collection", "typershark", "Collection/TS/", ["typershark"]]], rejected: ["Bare"] });
// AstroPop's the same: WinAP.exe in its Steam copy, AstroPop.exe in the Collection's
if (finished.indexOf("astropop") < 0) {
  const js = fs.readFileSync(path.join(root, "games", "astropop", "web", "page.js"), "utf8");
  vm.runInContext(`ByopopImport.add("astropop", "AstroPop Deluxe", function () {\n${js}\n  return PAGE;\n});`, ctx);
}
const ap = (exe) => files("AP/data/bonus-1.xcf", "AP/images/_galaxy1.gif", "AP/sounds/a.ogg").concat(exe ? files("AP/" + exe) : []);
check("either: AstroPop's Steam exe", I.findRoot(spec("astropop"), ap("WinAP.exe"), false), "AP/");
check("either: AstroPop's Collection exe", I.findRoot(spec("astropop"), ap("AstroPop.exe"), false), "AP/");
check("either: AstroPop with neither exe", I.findRoot(spec("astropop"), ap(null), false), null);
check("either: AstroPop with neither exe, named", I.missingMarkers(spec("astropop"), ap(null)), "WinAP.exe or AstroPop.exe");

// Files no port opens stay out (NEVER_OPENED): the original's decoded copies of its Ogg sounds in AstroPop and Dynomite, and only
// those; Rocket Mania's port reads its music's, so they stay
{
  const rels = (t, root, ...p) => I.pick(spec(t), files(...p.map((x) => root + x)), root, safeRel).map((f) => f.rel);
  check("never opened: AstroPop's cached sounds", rels("astropop", "AP/", "sounds/a.ogg", "sounds/cached_a.wav", "sounds/CACHED_B.WAV", "sounds/b.wav"),
    ["sounds/a.ogg", "sounds/b.wav"]);
  if (I.games.some((g) => g.target === "dynomite"))
    check("never opened: Dynomite's cached sounds", rels("dynomite", "D/", "sounds/boom.ogg", "sounds/cached_boom.wav"), ["sounds/boom.ogg"]);
  if (I.games.some((g) => g.target === "rocketmania"))
    check("never opened: Rocket Mania keeps its cached music", rels("rocketmania", "R/", "music/cached_tubetricks.wav"), ["music/cached_tubetricks.wav"]);
}

// The exe check (exeUnreadable): a made-up image with one section at 0x401000 (raw data at 0x200) holding a text at 0x401010.
// The probe holds in the plain image and in the same image behind PopCap's launcher marker, and fails once the section's
// bytes are scrambled, as a packed and encrypted copy's are, or for a wrong hash.
{
  const img = new Uint8Array(0x400);
  img[0] = 0x4d; img[1] = 0x5a;
  const dv = new DataView(img.buffer);
  dv.setUint32(0x3c, 0x80, true);
  img.set([0x50, 0x45, 0, 0], 0x80);
  dv.setUint16(0x80 + 6, 1, true); dv.setUint16(0x80 + 20, 0xe0, true); dv.setUint32(0x80 + 24 + 28, 0x400000, true);
  const sec = 0x80 + 24 + 0xe0;
  dv.setUint32(sec + 12, 0x1000, true); dv.setUint32(sec + 16, 0x200, true); dv.setUint32(sec + 20, 0x200, true);
  const text = [0x47, 0x65, 0x6d], at = 0x210;
  img.set(text, at); img[at + 3] = 0;
  let h = 2166136261; for (const c of text) { h ^= c; h = Math.imul(h, 16777619) >>> 0; }
  const probe = { va: 0x401010, len: 3, fnv: h };
  check("exe: plain image", I.probeHolds(img, probe), true);
  const marker = Array.from("junk!popcapdrmprotect!pad").map((c) => c.charCodeAt(0));
  const wrapped = new Uint8Array(0x300 + marker.length + img.length);
  wrapped[0] = 0x4d; wrapped[1] = 0x5a; wrapped.set(marker, 0x300); wrapped.set(img, 0x300 + marker.length);
  check("exe: PopCap's launcher with the image appended", I.probeHolds(wrapped, probe), true);
  const packed = img.slice(); for (let i = 0x200; i < 0x400; i++) packed[i] ^= 0x5a;
  check("exe: the section packed", I.probeHolds(packed, probe), false);
  check("exe: a wrong hash", I.probeHolds(img, { va: probe.va, len: 3, fnv: (h ^ 1) >>> 0 }), false);
  ctx.ByopopExeProbes = { alchemy: { files: ["AlchemyGame.exe"], probes: [probe] } };
  const one = (bytes) => [{ rel: "AlchemyGame.exe", read: () => Promise.resolve(bytes) }];
  waits.push(I.exeUnreadable("alchemy", one(img)).then((r) => check("exe: a readable copy passes", r, null)));
  waits.push(I.exeUnreadable("alchemy", one(packed)).then((r) => check("exe: a packed copy is named", r, "AlchemyGame.exe")));
  waits.push(I.exeUnreadable("alchemy", []).then((r) => check("exe: no exe is the markers' to say", r, null)));
  waits.push(I.exeUnreadable("bejeweled", one(packed)).then((r) => check("exe: a game with no probes passes", r, null)));
}

// The real tables (web/exeprobes.js): every game's probes hold in no exe they should not, and Shockwave's BookWorm (Armadillo:
// its game sections packed and encrypted), when a copy sits in build/shockwave/BookWorm.exe (git-ignored), is refused
{
  const tab = new Function(fs.readFileSync(path.join(root, "web", "exeprobes.js"), "utf8") + "; return ByopopExeProbes;")();
  check("exe table: every game has names and probes", Object.keys(tab).every((g) => tab[g].files.length > 0 && tab[g].probes.length > 0), true);
  const sw = path.join(root, "build", "shockwave", "BookWorm.exe");
  if (fs.existsSync(sw)) {
    const b = new Uint8Array(fs.readFileSync(sw));
    check("exe: Shockwave's BookWorm is refused", tab.bookworm.probes.some((q) => I.probeHolds(b, q)), false);
  }
}

// ---- a dropped installer (installerIn, installerMark): small synthetic files, each carrying one mark and nothing else; never a
// real installer or a game's file
{
  // a program: "MZ", the PE header's place at 0x3c, "PE" and one section, then text at 0x200
  const program = (text, opts) => {
    const b = new Uint8Array(4096);
    b[0] = 0x4d; b[1] = 0x5a; b[0x3c] = 0x80;
    b.set([0x50, 0x45, 0, 0], 0x80);
    b[0x86] = 1;                     // one section
    b[0x94] = 0xe0;                  // the optional header's size: the section table at 0x80 + 24 + 0xe0
    const sec = 0x80 + 24 + 0xe0, name = (opts && opts.section) || ".text";
    for (let i = 0; i < name.length; i++) b[sec + i] = name.charCodeAt(i);
    const t = text || "";
    const bytes = opts && opts.wide ? [...t].flatMap((c) => [c.charCodeAt(0), 0]) : [...t].map((c) => c.charCodeAt(0) & 0xff);
    b.set(bytes, 0x200);
    if (opts && opts.at30) b.set([0x49, 0x6e, 0x6e, 0x6f], 0x30);
    if (opts && opts.tail) { const end = new Uint8Array(b.length + 22); end.set(b); end.set([0x50, 0x4b, 5, 6], b.length); return end; }
    return b;
  };
  const msi = () => { const b = new Uint8Array(1024); b.set([0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1]); return b; };
  const mark = (b) => I.installerMark(b.subarray(0, 4 * 1048576), b.subarray(Math.max(0, b.length - 65558)));
  check("mark: an MSI's compound file header", mark(msi()), "msi");
  check("mark: Inno Setup's \"Inno\" at 0x30", mark(program("", { at30: true })), "inno");
  check("mark: Inno Setup's loader table", mark(program("rDlPtS02\x87eVx")), "inno");
  check("mark: Inno Setup in its version text (UTF-16)", mark(program("This installation was built with Inno Setup.", { wide: true })), "inno");
  check("mark: NSIS's first header", mark(program("\xef\xbe\xad\xdeNullsoftInst")), "nsis");
  check("mark: InstallShield in its version text (UTF-16)", mark(program("InstallShield (R) Setup Launcher", { wide: true })), "installshield");
  check("mark: Wise's .WISE section", mark(program("", { section: ".WISE" })), "wise");
  check("mark: Wise Installation", mark(program("Wise Installation Wizard")), "wise");
  check("mark: a 7-Zip archive inside", mark(program("7z\xbc\xaf\x27\x1c")), "sfx");
  check("mark: a RAR archive inside", mark(program("Rar!\x1a\x07\x00")), "sfx");
  check("mark: a cabinet inside (IExpress)", mark(program("MSCF\0\0\0\0")), "sfx");
  check("mark: WinZip's self-extractor", mark(program("WinZip Self-Extractor")), "sfx");
  check("mark: a zip's end record at the end", mark(program("", { tail: true })), "sfx");
  check("mark: a plain program", mark(program("This program cannot be run in DOS mode.")), null);
  check("mark: a mark's text in a file that is no program", mark(new TextEncoder().encode("Inno Setup NullsoftInst InstallShield")), null);
  // installerIn over drops: {path, read, part}
  const file = (p, bytes) => ({ path: p, read: () => Promise.resolve(bytes || new Uint8Array(0)),
    part: (a, b) => Promise.resolve((bytes || new Uint8Array(0)).slice(a, b)) });
  const brief7 = (r) => r && { file: r.file, mark: r.mark, group: r.group, game: r.game && r.game.target, other: r.other && r.other.id, folder: r.folder };
  const drop = (entries) => I.installerIn(entries, I.sort(entries)).then(brief7);
  const t7 = (what, entries, want) => waits.push(drop(entries).then((r) => check("installer: " + what, r, want)));
  t7("setup.exe alone, by its name", [file("setup.exe")],
    { file: "setup.exe", mark: "name", group: "import.dropped_files", game: null, other: null, folder: null });
  t7("a game's installer by its name: the game and its usual folder", [file("AlchemyDeluxeSetup.exe")],
    { file: "AlchemyDeluxeSetup.exe", mark: "name", group: "import.dropped_files", game: "alchemy", other: null,
      folder: { path: "C:\\Program Files (x86)\\PopCap Games\\Alchemy Deluxe", steam: false } });
  t7("a game the site knows as a Steam copy: steamapps\\common", [file("Bejeweled2_Setup.exe")],
    { file: "Bejeweled2_Setup.exe", mark: "name", group: "import.dropped_files", game: "bejeweled2", other: null,
      folder: { path: "steamapps\\common\\Bejeweled 2 Deluxe", steam: true } });
  t7("a game the site does not play", [file("ZumaDeluxeInstaller.exe")],
    { file: "ZumaDeluxeInstaller.exe", mark: "name", group: "import.dropped_files", game: null, other: "zuma", folder: null });
  t7("a program with a mark, with a few files", [file("Stuff/readme.txt"), file("Stuff/game1.exe", program("NullsoftInst")), file("Stuff/data.bin")],
    { file: "game1.exe", mark: "nsis", group: "Stuff", game: null, other: null, folder: null });
  t7("setup.exe in a folder named for the game: the folder's name names it", [file("Alchemy Deluxe/setup.exe"), file("Alchemy Deluxe/readme.txt")],
    { file: "setup.exe", mark: "name", group: "Alchemy Deluxe", game: "alchemy", other: null,
      folder: { path: "C:\\Program Files (x86)\\PopCap Games\\Alchemy Deluxe", steam: false } });
  t7("a short name held in a game's is not that game", [file("big_setup.exe")],
    { file: "big_setup.exe", mark: "name", group: "import.dropped_files", game: null, other: null, folder: null });
  t7("an .msi by its header", [file("Pkg.msi", msi())], { file: "Pkg.msi", mark: "msi", group: "import.dropped_files", game: null, other: null, folder: null });
  const many = [];
  for (let i = 0; i < 40; i++) many.push(file(`Big/data/f${i}.dat`));
  t7("the only program in a dropped .zip", many.concat([file("Big/run.exe", program("Inno Setup", { wide: true }))]),
    { file: "run.exe", mark: "inno", group: "Big", game: null, other: null, folder: null });
  t7("one of many programs with many files is not looked at", many.concat([file("Big/a.exe", program("NullsoftInst")), file("Big/b.exe")]), null);
  t7("a plain program alone is not one", [file("Tool.exe", program("hello"))], null);
  t7("an uninstaller alone is not one", [file("unins000.exe"), file("unins000.dat")], null);
  t7("DirectX's setup alone is not one", [file("dxsetup.exe")], null);
  // a game's folder with a setup and an uninstaller beside the game is the game, as before
  const tt = [file("TipTop Deluxe/TipTop.dat"), file("TipTop Deluxe/TipTop.exe"), file("TipTop Deluxe/setup.exe"), file("TipTop Deluxe/unins000.exe")];
  check("installer: a game's folder with a setup.exe is still the game", brief(I.sort(tt)).found.map((f) => f[1]), ["tiptop"]);
  t7("none looked for where the sort found the game", tt, null);
  // sortDrop: the installer takes its folder's place among the rejected; one for a game the site does not play is among the others
  waits.push(I.sortDrop([file("Stuff/setup.exe"), file("Stuff/a.txt"), file("Photos/b.jpg")]).then((r) =>
    check("sortDrop: the installer in place of its folder", [r.rejected, r.installer && r.installer.file, r.installer && r.installer.text],
      [["Photos"], "setup.exe", "import.installer setup.exe"])));
  waits.push(I.sortDrop([file("ZumaDeluxeSetup.exe")]).then((r) =>
    check("sortDrop: a game the site does not play, named", [r.rejected, r.others.map((o) => [o.folder, o.game.id]), !!r.installer],
      [[], [["ZumaDeluxeSetup.exe", "zuma"]], false])));
}

// ---- a folder named as one of the site's games without its files: the game and what it lacks, not a folder of no game (the user,
// 2026-11-23: the import should name what is missing rather than call the game unsupported)
{
  const lack = (r) => ({ found: r.found.map((f) => f.game && f.game.target), rejected: r.rejected, others: r.others.map((o) => o.game.id),
    lacking: (r.lacking || []).map((l) => [l.folder, l.game.target, l.missing]) });
  check("lacking: Venice's folder without its pak", lack(I.sort(files("Venice Deluxe/config.xml", "Venice Deluxe/readme.html"))),
    { found: [], rejected: [], others: [], lacking: [["Venice Deluxe", "venice", "main.pak"]] });
  check("lacking: a Steam folder's name, without any of the files", lack(I.sort(files("Hammer Heads Deluxe/readme.html"))).lacking,
    [["Hammer Heads Deluxe", "hammerheads", I.missingMarkers(spec("hammerheads"), [])]]);
  check("lacking: a name that only holds a game's is still rejected", lack(I.sort(files("Bejeweled Twist Soundtrack/a.mp3"))),
    { found: [], rejected: ["Bejeweled Twist Soundtrack"], others: [], lacking: [] });
  check("lacking: a game the site does not play is no game of the site's", lack(I.sort(files("Peggle Deluxe/readme.html"))).rejected, ["Peggle Deluxe"]);
  waits.push(I.sortDrop(files("Alchemy Deluxe/setup.exe")).then((r) =>
    check("lacking: a named folder holding the game's installer is the installer's row", [lack(r).lacking, !!r.installer], [[], true])));
}

// ---- the ports localsoon opens (addSoon): taken only where web/localsoon.js opens their tiles, a development host without
// ?localsoon=0 (the user, 2026-11-23: Peggle's, Banana Bugs' and Feeding Frenzy's folders were "not supported" on the local site)
{
  const pegglePage = () => fs.readFileSync(path.join(root, "games", "peggle", "web", "page.js"), "utf8");
  const addPeggle = () => vm.runInContext(`ByopopImport.addSoon("peggle", "Peggle", function () {\n${pegglePage()}\n  return PAGE;\n});`, ctx);
  const has = () => I.games.some((g) => g.target === "peggle");
  addPeggle();
  check("addSoon: not taken without localsoon (the deployed site)", has(), false);
  ctx.URLSearchParams = URLSearchParams;
  vm.runInContext(fs.readFileSync(path.join(root, "web", "localsoon.js"), "utf8"), ctx, { filename: "localsoon.js" });
  vm.runInContext("var ByopopTerms = { devHost: function (l) { return l.hostname === '127.0.0.1'; } };", ctx);
  ctx.location = { hostname: "example.org", search: "" };
  addPeggle();
  check("addSoon: not taken on another host", has(), false);
  ctx.location = { hostname: "127.0.0.1", search: "?localsoon=0" };
  addPeggle();
  check("addSoon: not taken with ?localsoon=0", has(), false);
  ctx.location = { hostname: "127.0.0.1", search: "" };
  addPeggle();
  check("addSoon: taken on a development host", has(), true);
  const brief8 = (r) => ({ found: r.found.map((f) => f.game && f.game.target), others: r.others.map((o) => o.game.id) });
  check("addSoon: a Steam Peggle folder is Peggle", brief8(I.sort(files("Peggle Deluxe/Peggle.exe", "Peggle Deluxe/main.pak", "Peggle Deluxe/music/PeggleBeats.mo3"))),
    { found: ["peggle"], others: [] });
  // Peggle Extreme holds Peggle's markers and its own exe: it is Peggle Extreme, a game the site does not play
  check("addSoon: Peggle Extreme's folder is not Peggle", brief8(I.sort(files("Peggle Extreme/PeggleExtreme.exe", "Peggle Extreme/main.pak",
    "Peggle Extreme/music/PeggleBeats.mo3"))), { found: [], others: ["peggleextreme"] });
  I.games.splice(I.games.findIndex((g) => g.target === "peggle"), 1);
  delete ctx.location;
}

Promise.all(waits).then(() => {
console.log(`gameimporttest: ${checks} checks, ${failed} failed`);
process.exit(failed ? 1 : 0);
});
