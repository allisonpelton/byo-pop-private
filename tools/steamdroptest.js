// Checks and times the reading and sorting of a whole Steam library dropped on the listing (web/gameimport.js's fromDrop and
// sort; docs/WEB_PLAYER.md, "Adding games from the listing page"), under Node with every finished game's page.js registered as
// tools/gameimporttest.js registers them. The user asked whether a drop of their Steam library's steamapps folder, 101 games in
// 146,409 files and 24,158 folders, slows the site down (2026-11-16).
//   node tools/steamdroptest.js             a synthetic library of that size, made in memory, read through a stand-in for the
//                                           browser's entry API (FileSystemDirectoryEntry: readEntries in batches of 100, each
//                                           answered later, as the browser answers)
//   node tools/steamdroptest.js --disk DIR  the same library written as empty files under DIR (in build/), read from the disk
//                                           through the same stand-in, and deleted afterwards
//   node tools/steamdroptest.js --small     a tenth of the size, for unit-web
//   ... --import FILE                       FILE in place of web/gameimport.js (EXPECTED below was made from the old one)
// The library holds real games' marker names (the games' own page.js markers, a copy of each game's folders in the layout of its
// copy, and the names other games share with them: config.xml, resource.dll, main.pak, images/title.jpg and the like), and no
// game file. It checks that every game is found where it is, that the folders without a game are rejected as they were before
// the sort was made quicker (the list below, made by the sort as it was), that the sort in steps gives the sort's result, that
// the count the walk reports ends at the library's folders and files, and that Cancel stops the walk. It prints the times.
// It prints "steamdroptest: N checks, M failed" and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const root = path.join(__dirname, "..");
const args = process.argv.slice(2);
const small = args.includes("--small");
const diskAt = args.indexOf("--disk") >= 0 ? args[args.indexOf("--disk") + 1] : null;
const importFile = args.indexOf("--import") >= 0 ? args[args.indexOf("--import") + 1] : path.join(root, "web", "gameimport.js");

// ---------- the site's import, with every finished game (as tools/gameimporttest.js loads them) ----------
// (sortSteps yields through a MessageChannel: this one times each run between yields, the time the sort holds the page)
const slices = [];
class TimedChannel {
  constructor() {
    const c = new MessageChannel();
    this.port2 = c.port2;
    this.port1 = { close: () => c.port1.close(),
      set onmessage(fn) { c.port1.onmessage = (ev) => { const t = performance.now(); fn(ev); slices.push(performance.now() - t); }; } };
  }
}
const ctx = { window: {}, console, setTimeout, clearTimeout, MessageChannel: TimedChannel, performance };
vm.createContext(ctx);
require("./stringsload").into(ctx);
vm.runInContext(fs.readFileSync(importFile, "utf8"), ctx, { filename: "gameimport.js" });
const cmake = fs.readFileSync(path.join(root, "cmake", "ByopopWeb.cmake"), "utf8");
const finished = /\nset\(BYOPOP_WEB_FINISHED([^)]*)\)/.exec(cmake)[1].trim().split(/\s+/);
for (const target of finished) {
  const js = fs.readFileSync(path.join(root, "games", target, "web", "page.js"), "utf8");
  vm.runInContext(`ByopopImport.add(${JSON.stringify(target)}, ${JSON.stringify(target)}, function () {\n${js}\n  return PAGE;\n});`, ctx);
}
const others = /\nset\(BYOPOP_OTHER_GAMES([^)]*)\)/.exec(fs.readFileSync(path.join(root, "cmake", "ByopopOthers.cmake"), "utf8"))[1]
  .match(/"[^"]*"/g).map((q) => {
    const [id, title, markers, project, url] = q.slice(1, -1).split("|");
    return { id, title, markers: markers.split(","), project: project || "", url: url || "" };
  });
vm.runInContext(`var ByopopOtherGames = ${JSON.stringify(others)};`, ctx);
const I = vm.runInContext("ByopopImport", ctx);
const specOf = (t) => I.games.filter((g) => g.target === t)[0].spec;

let checks = 0, failed = 0;
function check(what, got, want) {
  checks++;
  const g = JSON.stringify(got), w = JSON.stringify(want);
  if (g !== w) { failed++; console.log(`FAIL ${what}: got ${g.slice(0, 2000)}, want ${w.slice(0, 2000)}`); }
}

// ---------- the library ----------
// A seeded random, so that the library is the same on every run
let seed = 20261116;
function rand() { seed = (seed + 0x6d2b79f5) | 0; let t = Math.imul(seed ^ (seed >>> 15), 1 | seed); t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t; return ((t ^ (t >>> 14)) >>> 0) / 4294967296; }
function pickOne(a) { return a[Math.floor(rand() * a.length)]; }
const SCALE = small ? 0.1 : 1;
const LIB = "SteamLibrary/steamapps";
// folder path -> [{name, dir}] in listing order. Names are told apart in any case, as Windows does: a folder or file whose name
// differs from one beside it only in case is that one (so that the library on the disk is the library in memory)
const tree = { dirs: new Map(), files: 0, folders: 0, cased: new Map() };
function addDir(p) {
  const low = p.toLowerCase();
  if (tree.cased.has(low)) return tree.cased.get(low);
  const at = p.lastIndexOf("/");
  if (at > 0) { const parent = addDir(p.slice(0, at)); p = parent + p.slice(at); tree.dirs.get(parent).push({ name: p.slice(at + 1), dir: true }); }
  tree.cased.set(low, p);
  tree.dirs.set(p, []);
  tree.folders++;
  return p;
}
function addFile(p) {
  const at = p.lastIndexOf("/");
  const kids = tree.dirs.get(addDir(p.slice(0, at))), name = p.slice(at + 1);
  if (kids.some((k) => k.name.toLowerCase() === name.toLowerCase())) return;
  kids.push({ name, dir: false });
  tree.files++;
}
// The site's games, each in a folder named as its copy names it, with its markers (an alternative's first name, "x.*" as x.png)
// and files in its game folders. Two copies of one game where Steam has them, and a game in a folder of its own one level down.
const SITE = [
  ["alchemy", "Alchemy Deluxe"], ["astropop", "AstroPop Deluxe"], ["bejeweled2", "Bejeweled 2 Deluxe"], ["bejeweledtwist", "Bejeweled Twist"],
  ["bookwormadv", "Bookworm Adventures Deluxe"], ["bookworm", "Bookworm Deluxe"], ["dynomite", "Dynomite Deluxe"], ["typershark", "Typer Shark Deluxe"],
  ["heavyweapon", "Heavy Weapon Deluxe"], ["hammerheads", "Hammer Heads Deluxe"], ["rocketmania", "Rocket Mania Deluxe"], ["bigmoney", "Big Money Deluxe"],
  ["venice", "Venice Deluxe"], ["pizzafrenzy", "Pizza Frenzy"], ["noahsark", "Noah's Ark"], ["mahjongjapan", "Mahjong Escape Ancient Japan"],
].filter(([t]) => finished.includes(t));
const OTHER = [["zuma", "Zuma Deluxe"], ["chuzzle", "Chuzzle Deluxe"], ["insaniquarium", "Insaniquarium Deluxe"], ["peggle", "Peggle Deluxe"],
  ["plantsvszombies", "Plants vs. Zombies"], ["bejeweled3", "Bejeweled 3"], ["zumasrevenge", "Zuma's Revenge"]];
function markerFile(m) { return m.split("|")[0].replace(/\.\*$/, ".png"); }
function siteGame(target, folder) {
  const s = specOf(target), base = `${LIB}/common/${folder}`;
  for (const m of s.markers) addFile(`${base}/${markerFile(m)}`);
  const dirs = s.gameDirs.length ? s.gameDirs : ["data"];
  const n = Math.round(150 * SCALE) + 20;
  for (let i = 0; i < n; i++) {
    const d = pickOne(dirs), cased = rand() < 0.5 ? d : d[0].toUpperCase() + d.slice(1);
    addFile(`${base}/${cased}/${rand() < 0.3 ? "sub" + (i % 4) + "/" : ""}file${i}.${pickOne(["png", "jpg", "ogg", "txt", "xml"])}`);
  }
  addFile(`${base}/unins000.exe`);
  addFile(`${base}/ReadMe.txt`);
}
for (const [t, f] of SITE) siteGame(t, f);
for (const [id, f] of OTHER) {
  const o = others.filter((x) => x.id === id)[0];
  for (const m of o.markers) addFile(`${LIB}/common/${f}/${m}`);
  for (let i = 0; i < 40 * SCALE + 5; i++) addFile(`${LIB}/common/${f}/data/x${i}.dat`);
}
// The other games: engines' folder shapes, from a few files to tens of thousands, with the names they share with the site's
// games' markers here and there (none makes a game: each lacks the rest of its markers)
const SHARED = ["config.xml", "resource.dll", "main.pak", "images/title.jpg", "properties/default.xml", "music/game.it",
  "data/twords.txt", "images/titlescreen.jpg", "levels/levels.txt", "wordlist.txt", "readme.txt", "images/intro.swf"];
const WORDS = ["Data", "Content", "Assets", "Textures", "Sounds", "Music", "Maps", "Levels", "Shaders", "Models", "Fonts", "Locale",
  "Scripts", "Managed", "Plugins", "Resources", "StreamingAssets", "Movies", "UI", "Config", "images", "sounds", "music", "data", "properties"];
const filesWanted = Math.round(146409 * SCALE), foldersWanted = Math.round(24158 * SCALE);
const otherCount = 101 - SITE.length - OTHER.length;
const sizes = [];
for (let i = 0; i < otherCount; i++) sizes.push(Math.pow(rand(), 3) + 0.01);
const sizeSum = sizes.reduce((a, b) => a + b, 0);
const filesBefore = tree.files, foldersBefore = tree.folders;
const otherFiles = filesWanted - filesBefore - 2 * otherCount - 60, otherFolders = foldersWanted - foldersBefore - 60;
for (let i = 0; i < otherCount; i++) {
  const name = `Game ${String(i + 1).padStart(3, "0")} ${pickOne(["Remastered", "Gold", "", "Classic", "HD", "Online", ""])}`.trim();
  const base = `${LIB}/common/${name}`;
  const files = Math.max(3, Math.round(otherFiles * sizes[i] / sizeSum));
  const folders = Math.max(1, Math.round(otherFolders * sizes[i] / sizeSum));
  const made = [base];
  for (let k = 1; k < folders; k++) {
    const parent = made[Math.floor(rand() * made.length)];
    const d = `${parent}/${pickOne(WORDS)}${rand() < 0.6 ? "_" + k : ""}`;
    made.push(addDir(d));
  }
  for (let k = 0; k < files; k++) addFile(`${pickOne(made)}/f${k}.${pickOne(["pak", "bin", "dat", "png", "ogg", "json", "dll", "assets", "bank"])}`);
  addFile(`${base}/${name.split(" ")[0]}${i}.exe`);
  addFile(`${base}/steam_api.dll`);
  if (rand() < 0.35) addFile(`${pickOne(made)}/${pickOne(SHARED)}`);
  if (rand() < 0.15) addFile(`${pickOne(made)}/${pickOne(SHARED)}`);
}
// Steam's own folders beside common
for (let i = 0; i < 101; i++) addFile(`${LIB}/appmanifest_${10000 + i * 37}.acf`);
addFile(`${LIB}/libraryfolders.vdf`);
for (let i = 0; i < 20; i++) addFile(`${LIB}/shadercache/${20000 + i}/fozpipelinesv6/steamapprun_pipeline_cache.foz`);
addDir(`${LIB}/downloading`);
addDir(`${LIB}/temp`);
for (let i = 0; i < 6; i++) addFile(`${LIB}/workshop/appworkshop_${30000 + i}.acf`);

// Each folder listed in the order Windows lists it (NTFS: the names in upper case, compared code by code), as a real drop
// and the disk give them
const ntfs = (a, b) => { const x = a.name.toUpperCase(), y = b.name.toUpperCase(); return x < y ? -1 : x > y ? 1 : 0; };
for (const kids of tree.dirs.values()) kids.sort(ntfs);

// ---------- the browser's entry API, standing in ----------
// readEntries answers at most 100 entries a call, each answer later (a macrotask, as the browser answers from its own process)
const later = (fn) => setImmediate(fn);
let reads = 0;
function entryOf(p, dir) {
  const name = p.slice(p.lastIndexOf("/") + 1);
  if (!dir) return { isFile: true, isDirectory: false, name, fullPath: "/" + p,
    file: (ok) => later(() => ok({ name, arrayBuffer: () => Promise.resolve(new ArrayBuffer(0)) })) };
  return { isFile: false, isDirectory: true, name, fullPath: "/" + p, createReader() {
    let at = 0, kids = null;
    return { readEntries(ok, fail) {
      reads++;
      const answer = () => { const b = kids.slice(at, at + 100); at += b.length; ok(b.map((k) => entryOf(p + "/" + k.name, k.dir))); };
      if (kids) { later(answer); return; }
      if (!diskAt) { kids = tree.dirs.get(p); later(answer); return; }
      fs.readdir(path.join(diskAt, p), { withFileTypes: true }, (err, list) => {
        if (err) { fail(err); return; }
        kids = list.map((d) => ({ name: d.name, dir: d.isDirectory() }));
        answer();
      });
    } };
  } };
}
const drop = () => ({ types: ["Files"], items: [{ kind: "file", webkitGetAsEntry: () => entryOf("SteamLibrary", true) }] });

// ---------- the library on the disk (--disk) ----------
function writeDisk() {
  const t = Date.now();
  for (const [p, kids] of tree.dirs) {
    fs.mkdirSync(path.join(diskAt, p), { recursive: true });
    for (const k of kids) if (!k.dir) fs.writeFileSync(path.join(diskAt, p, k.name), "");
  }
  console.log(`steamdroptest: wrote ${tree.files} files in ${tree.folders} folders under ${diskAt} in ${((Date.now() - t) / 1000).toFixed(1)} s`);
}

// ---------- the run ----------
const brief = (r) => ({
  found: r.found.map((f) => [f.folder, f.game ? f.game.target : null, f.root, f.choices.map((g) => g.target), f.entries.length]),
  rejected: r.rejected, others: (r.others || []).map((o) => [o.folder, o.game.id]) });
const ms = (t) => Math.round(performance.now() - t);
async function main() {
  if (diskAt) {
    if (!/[\\/]build[\\/]/.test(path.resolve(diskAt) + "/")) throw new Error("--disk wants a folder under build/");
    fs.rmSync(diskAt, { recursive: true, force: true });
    writeDisk();
  }
  try {
    console.log(`steamdroptest: a library of ${tree.files} files in ${tree.folders} folders${diskAt ? " on the disk" : ""}`);
    // the walk, with the count it reports
    let seen = null, calls = 0;
    reads = 0;
    let t = performance.now();
    const entries = await I.fromDrop(drop(), null, { progress: (folders, files) => { calls++; seen = [folders, files]; } });
    const walkMs = ms(t), walkReads = reads;
    check("the walk reads every file", entries.length, tree.files);
    if (I.sortSteps) check("the count ends at the library's folders and files", seen, [tree.folders, tree.files]);
    // the sort, at once and in steps
    t = performance.now();
    const sorted = I.sort(entries);
    const sortMs = ms(t);
    let stepsMs = null, longest = null;
    if (I.sortSteps) {
      t = performance.now();
      slices.length = 0;
      const stepped = await I.sortSteps(entries);
      stepsMs = ms(t);
      longest = Math.round(Math.max(...slices));
      check("the sort in steps is the sort", brief(stepped), brief(sorted));
    }
    const b = brief(sorted);
    if (args.includes("--print")) console.log(JSON.stringify(b, null, 1));
    // every site game found, at its folder, by itself, and no other game's folder taken for one. (In the full library a folder of
    // "Game 023 Classic" holds config.xml beside a folder named properties, Venice's marker and one of its folders' names, which
    // was taken as Venice while any of its folders stood for its main.pak.)
    const got = b.found.map((f) => JSON.stringify(f.slice(0, 3)));
    check("every game found where it is", SITE.filter(([target, folder]) => got.indexOf(JSON.stringify([folder, target, `${LIB}/common/${folder}/`])) < 0), []);
    check("no other game's folder found as one of the site's", b.found.map((f) => f[0]).filter((folder) => !SITE.some(([, f]) => f === folder)), []);
    check("the other games named", b.others.map((o) => o[1]).sort(), OTHER.map((o) => o[0]).sort());
    // each game's folder once, and shadercache and workshop (downloading and temp are empty)
    check("every folder sorted once", b.found.length + b.others.length + b.rejected.length, 101 + 2);
    check("the result the sort gave before", require("crypto").createHash("sha1").update(JSON.stringify(b)).digest("hex"), EXPECTED[small ? "small" : "full"]);
    // Cancel: the walk stops at the next folder, and rejects as cancelled
    if (I.sortSteps) {
      let stop = false, readsAtStop = 0;
      reads = 0;
      const p = I.fromDrop(drop(), null, { progress: (folders) => { if (folders >= 50 && !stop) { stop = true; readsAtStop = reads; } }, stopped: () => stop });
      const got = await p.then(() => "finished", (e) => (e && e.cancelled ? "cancelled" : "failed: " + e));
      check("Cancel stops the walk", got, "cancelled");
      check("no folder read after Cancel", reads - readsAtStop <= 1, true);
    }
    console.log(`steamdroptest: walk ${walkMs} ms (${walkReads} reads), sort ${sortMs} ms` + (stepsMs !== null ? `, sort in steps ${stepsMs} ms, ${slices.length} runs, the longest ${longest} ms` : "") +
      `; ${b.found.length} found, ${b.others.length} others, ${b.rejected.length} rejected`);
  } finally {
    if (diskAt) { const t = Date.now(); fs.rmSync(diskAt, { recursive: true, force: true }); console.log(`steamdroptest: deleted the library in ${((Date.now() - t) / 1000).toFixed(1)} s`); }
  }
  console.log(`steamdroptest: ${checks} checks, ${failed} failed`);
  process.exit(failed ? 1 : 0);
}
// The sort's result before it was made quicker: the SHA-1 of brief() of it as JSON, for each size (--print shows it), made by
// web/gameimport.js as it was at ea0c1d11 (--import). Since Venice's unpacked copy is known by its pak's own files (PAGE.unpacked),
// the full library's one difference from it is "Game 023 Classic", once taken as Venice, among the rejected folders.
const EXPECTED = { small: "05fac1940fa9a96c019c34ad94c34cbae0519307", full: "cd310ca21fd56d3bd3a5f824ed5a4c2aeec20d56" };
main().catch((e) => { console.log("steamdroptest: " + (e && e.stack || e)); process.exit(1); });
