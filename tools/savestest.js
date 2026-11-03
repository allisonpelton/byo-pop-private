// Checks the pure parts of web/saves.js (the save backup, docs/WEB_PLAYER.md, "Save backup") under Node:
//   node tools/savestest.js
// The zip it writes and reads (store only, checksums, a damaged one, an encrypted one, one that is no zip), the rule for a path
// inside a save folder, the sorting of a zip's entries (a path outside a game's save folder, a game that is not listed, a name
// twice), the merge of a game's files with its localStorage backup (what the game would find at its next start) and the file's name.
// It prints "savestest: N checks, M failed" and exits 1 when any failed.
"use strict";
const path = require("path");
require("./stringsload").intoGlobal();
const S = require(path.join(__dirname, "..", "web", "saves.js"));

let checks = 0, failed = 0;
function check(name, ok, detail) {
  checks++;
  if (!ok) { failed++; console.log("FAIL " + name + (detail !== undefined ? ": " + JSON.stringify(detail) : "")); }
}
function same(name, a, b) { check(name, JSON.stringify(a) === JSON.stringify(b), { got: a, want: b }); }
function bytes(s) { return new TextEncoder().encode(s); }
function text(u) { return new TextDecoder().decode(u); }
function throws(name, fn, pattern) {
  let msg = null;
  try { fn(); } catch (e) { msg = String(e.message || e); }
  check(name, msg !== null && (!pattern || pattern.test(msg)), msg);
}

(async () => {
  // ---------- the zip ----------
  check("crc32 of 123456789", S.crc32(bytes("123456789")) === 0xcbf43926);
  check("crc32 of nothing", S.crc32(new Uint8Array(0)) === 0);
  const entries = [{ name: "a/userdata/save.dat", bytes: bytes("hello") }, { name: "b/replays/\u00e9.bjr", bytes: new Uint8Array([0, 1, 2, 255]) },
    { name: "empty", bytes: new Uint8Array(0) }];
  const zip = S.writeZip(entries, new Date(2026, 9, 27, 4, 5, 6));
  const read = S.readZip(zip);
  same("names", read.map((e) => e.name), entries.map((e) => e.name));
  same("sizes", read.map((e) => e.size), [5, 4, 0]);
  for (let i = 0; i < read.length; i++) {
    const b = await read[i].read();
    same("bytes of " + read[i].name, Array.from(b), Array.from(entries[i].bytes));
  }
  // a zip made by another tool: a directory entry is skipped (made by hand below), a changed byte fails its checksum
  const bad = zip.slice();
  bad[30 + "a/userdata/save.dat".length] ^= 1;   // the first byte of the first file's data
  const badRead = S.readZip(bad);
  let rejected = false;
  try { await badRead[0].read(); } catch (e) { rejected = /checksum/.test(String(e.message)); }
  check("a changed byte fails the checksum", rejected);
  throws("not a zip", () => S.readZip(bytes("this is not a zip file at all, just text")), /not a \.zip/);
  throws("a short file", () => S.readZip(new Uint8Array(5)), /not a \.zip/);
  const enc = zip.slice();
  enc[6] |= 1;   // the first local header's flags: encrypted (the central directory's flags matter here)
  const dirStart = new DataView(zip.buffer, zip.byteOffset, zip.length).getUint32(zip.length - 22 + 16, true);
  enc[dirStart + 8] |= 1;
  throws("an encrypted entry", () => S.readZip(enc), /encrypted/);
  const trunc = zip.slice(0, zip.length - 10);
  throws("a zip cut short", () => S.readZip(trunc), /not a \.zip/);
  // zip64 markers
  const z64 = zip.slice();
  new DataView(z64.buffer).setUint16(z64.length - 22 + 10, 0xffff, true);
  throws("zip64", () => S.readZip(z64), /zip64/);

  // ---------- paths ----------
  check("a plain file", S.safeRel("savegame.dat") && S.safeRel("players/Alice/classic.dat"));
  check("up and down", !S.safeRel("../x") && !S.safeRel("a/../b") && !S.safeRel("a/./b"));
  check("absolute and empty", !S.safeRel("/etc/passwd") && !S.safeRel("") && !S.safeRel("a//b") && !S.safeRel("a/"));
  check("backslash and control characters", !S.safeRel("a\\b") && !S.safeRel("a\u0000b") && !S.safeRel("a\nb"));
  check("a long path", !S.safeRel("x".repeat(300)) && S.safeRel("x".repeat(200)));
  check("not a string", !S.safeRel(null) && !S.safeRel(7));

  // ---------- sorting a zip's entries ----------
  const ids = ["bejeweled", "pixelus"];
  const ok = S.classify([{ name: "byopop-saves.json" }, { name: "byopop-settings.json" }, { name: "byopop-profiles.json" },
    { name: "pixelus/userdata/users/1" }, { name: "pixelus/replays/a.rep" }, { name: "pixelus/browser-backup.json" }, { name: "bejeweled/userdata/x" }], ids);
  check("a good backup", ok.ok && ok.files.length === 3 && ok.manifest && ok.settings && ok.profiles && ok.backups.pixelus, ok.error);
  same("kinds", ok.files.map((f) => [f.id, f.kind, f.rel]), [["pixelus", "userdata", "users/1"], ["pixelus", "replays", "a.rep"], ["bejeweled", "userdata", "x"]]);
  const refused = (name) => !S.classify([{ name: "byopop-saves.json" }, { name }], ids).ok;
  check("a path outside the game's save folders", refused("pixelus/game/main.pak") && refused("pixelus/mods/m.json") && refused("pixelus/userdata"));
  check("a path that climbs", refused("pixelus/userdata/../game/x") && refused("pixelus/userdata/a/../../b"));
  check("a game that is not listed", refused("evil/userdata/x") && refused("evil/browser-backup.json"));
  check("a stray file", refused("x.exe") && refused("userdata/x") && refused("/pixelus/userdata/x") && refused("pixelus/USERDATA/x"));
  const later = S.classify([{ name: "byopop-saves.json" }, { name: "byopop-play.json" }, { name: "byopop-later_thing.json" }, { name: "pixelus/userdata/a" }], ids);
  check("a top-level byopop-*.json this build does not know is skipped, not refused", later.ok && later.files.length === 1 &&
    later.skipped.join() === "byopop-play.json,byopop-later_thing.json", later);
  // the play times ride in the manifest (an older build reads past a field it does not know), and come back out of it
  same("play: into the manifest", S.playToManifest('{"tiptop":{"plays":1,"seconds":2,"last":"2026-11-02"}}'), { tiptop: { plays: 1, seconds: 2, last: "2026-11-02" } });
  check("play: none, or damaged, leaves the field out", S.playToManifest(null) === null && S.playToManifest("{nope") === null && S.playToManifest("[1]") === null);
  same("play: out of the manifest", [S.playFromManifest({ play: { a: { plays: 1 } } }), S.playFromManifest({}), S.playFromManifest({ play: [1] })],
    ['{"a":{"plays":1}}', null, null]);
  check("but not any top-level name", refused("byopop-x.txt") && refused("byopop-.json") && refused("other.json"));
  // the play times: a backup's added to this browser's
  same("play: the larger counts and the later day", JSON.parse(S.mergedPlay(
    JSON.stringify({ pixelus: { plays: 5, seconds: 100, last: "2026-11-02" }, tiptop: { plays: 1, seconds: 9, last: "2026-10-01" } }),
    JSON.stringify({ pixelus: { plays: 3, seconds: 400, last: "2026-10-30" }, ningpo: { plays: 2, seconds: 50, last: "2026-09-09" }, "../x": { plays: 9 } }))),
    { pixelus: { plays: 5, seconds: 400, last: "2026-11-02" }, tiptop: { plays: 1, seconds: 9, last: "2026-10-01" }, ningpo: { plays: 2, seconds: 50, last: "2026-09-09" } });
  same("play: with nothing here", JSON.parse(S.mergedPlay(null, JSON.stringify({ tiptop: { plays: 1, seconds: 2, last: "bad" } }))),
    { tiptop: { plays: 1, seconds: 2, last: "" } });
  same("play: a damaged backup", S.mergedPlay("{}", "{nope"), "{}");
  check("a name twice", !S.classify([{ name: "pixelus/userdata/a" }, { name: "pixelus/userdata/a" }], ids).ok);
  check("an absolute path", refused("/etc/passwd") && refused("C:/x") && refused("..\\x"));

  // ---------- the merge of a game's files with its localStorage backup ----------
  const b64 = (s) => Buffer.from(s).toString("base64");
  const files = { "old.dat": { bytes: bytes("old"), m: 1000 }, "same.dat": { bytes: bytes("same"), m: 2000 }, "gone.dat": { bytes: bytes("gone"), m: 500 },
    "newer-in-folder.dat": { bytes: bytes("folder"), m: 5000 } };
  const item = JSON.stringify({ v: 1, time: 3000, files: {
    "old.dat": { m: 2500, d: b64("newer in backup") },        // newer than the folder's: written
    "same.dat": { m: 2000, d: b64("different") },             // not newer: the folder's stays
    "new.dat": { m: 2800, d: b64("only in backup") },         // the folder lacks it: written
    "newer-in-folder.dat": { m: 4000, d: b64("stale") },       // the folder's is newer
    "big.dat": { m: 2900 },                                    // listed without data: never taken for deleted, and not written
    "../evil": { m: 9999, d: b64("x") },                       // an unsafe path is never taken
  } });
  const m = S.merged(files, item);
  same("merged: the files", Object.keys(m.files).sort(), ["new.dat", "newer-in-folder.dat", "old.dat", "same.dat"]);
  same("merged: from the backup", [text(m.files["old.dat"].bytes), text(m.files["new.dat"].bytes)], ["newer in backup", "only in backup"]);
  same("merged: the folder's stays", [text(m.files["same.dat"].bytes), text(m.files["newer-in-folder.dat"].bytes)], ["same", "folder"]);
  check("merged: a file the backup does not list and that is older than it is gone", !("gone.dat" in m.files) && m.dropped === 1 && m.used === 2, m);
  same("merged: with no backup", Object.keys(S.merged(files, null).files).sort(), Object.keys(files).sort());
  same("merged: with a damaged backup", Object.keys(S.merged(files, "{nope").files).sort(), Object.keys(files).sort());
  same("merged: with a backup of another version", Object.keys(S.merged(files, JSON.stringify({ v: 2, time: 1, files: {} })).files).sort(), Object.keys(files).sort());
  check("merged: does not change its input", Object.keys(files).length === 4 && text(files["old.dat"].bytes) === "old");

  // ---------- names ----------
  check("the file's name", S.backupName(new Date(2026, 9, 27)) === "byo-pop saves 2026-10-27.zip", S.backupName(new Date(2026, 9, 27)));
  same("sizes", [S.size(5), S.size(2048), S.size(5 * 1048576)], ["5 bytes", "2.0 KB", "5.0 MB"]);

  console.log("savestest: " + checks + " checks, " + failed + " failed");
  process.exit(failed ? 1 : 0);
})();
