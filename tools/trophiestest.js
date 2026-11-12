// Checks the trophy cabinet's reader (web/trophies.js, parse) under Node, on files made here in PopCap's high-score format:
//   node tools/trophiestest.js
// A record whose check holds keeps its score, one whose check fails shows the score % 256 as the games do, a day's best shows
// as "Today" only on today's day, Rocket Mania's names with their length twice, and Bejeweled 2's magic.
const path = require("path");
global.ByopopStrings = { t: (k) => k };
const R = require(path.join(__dirname, "..", "web", "trophies.js"));
let checks = 0, failed = 0;
function same(what, got, want) {
  checks++;
  const g = JSON.stringify(got), w = JSON.stringify(want);
  if (g !== w) { failed++; console.log("FAIL " + what + ": got " + g + ", want " + w); } else console.log("ok " + what);
}
function rec(name, score, good, twice) {
  const n = Buffer.from(name, "latin1");
  let x = 0; for (const c of n) x ^= ((c << 24) >> 24) * 0x11;
  const b = Buffer.alloc((twice ? 2 : 0) + 2 + n.length + 8);
  let at = 0;
  if (twice) { b.writeUInt16LE(n.length, 0); at = 2; }
  b.writeUInt16LE(n.length, at); n.copy(b, at + 2); at += 2 + n.length;
  b.writeInt32LE(score, at); b.writeInt32LE(good ? (score ^ 0x42beef ^ x) | 0 : 12345, at + 4);
  return b;
}
function day(d) { const b = Buffer.alloc(4); b.writeInt32LE(d, 0); return b; }
const now = new Date(), jan0 = new Date(now.getFullYear(), 0, 0);
const today = (now.getFullYear() - 1900) * 366 + Math.round((new Date(now.getFullYear(), now.getMonth(), now.getDate()) - jan0) / 86400000) - 1;
const table = (d, best, list) => Buffer.concat([day(d), best, ...list]);
const five = (s) => [0, 1, 2, 3, 4].map((i) => rec("P" + i, s - i * 100, true));
const bj = Buffer.concat([table(today, rec("Ann", 900, true), five(5000)), table(today - 1, rec("Old", 50, true), five(3000))]);
const t = R.parse(R.RECORDS.bejeweled, new Uint8Array(bj));
same("bejeweled: two tables by the game's names", t.map((x) => x.name), ["Normal", "Time Trial"]);
same("bejeweled: today's best first, then the five", t[0].rows.map((r) => [r.place, r.name, r.score]),
  [["trophies.today", "Ann", 900], ["1", "P0", 5000], ["2", "P1", 4900], ["3", "P2", 4800], ["4", "P3", 4700], ["5", "P4", 4600]]);
same("bejeweled: another day's best is not shown", t[1].rows.length, 5);
const bad = Buffer.concat([rec("Cheat", 99999, false), ...five(500).slice(1)]);
same("noahsark: a failed check shows score % 256", R.parse(R.RECORDS.noahsark, new Uint8Array(bad))[0].rows[0].score, 99999 % 256);
const rm = Buffer.concat([0, 1, 2].map(() => Buffer.concat([day(1), rec("X", 1, true, true), ...Array.from({ length: 10 }, (_, i) => rec("R" + i, 1000 - i, true, true))])));
same("rocketmania: the name's length twice", R.parse(R.RECORDS.rocketmania, new Uint8Array(rm)).map((x) => x.rows.length + x.rows[0].name), ["10R0", "10R0", "10R0"]);
// Atomica's HS.CFG: two fixed 208-byte tables; the day's best only on today's UTC day code
function atTable(scores, names, best, bestName, dayCode) {
  const b = Buffer.alloc(208);
  scores.forEach((v, i) => b.writeInt32LE(v, i * 4));
  names.forEach((n, i) => b.write(n, 20 + i * 30, "latin1"));
  b.writeInt32LE(best, 170); b.write(bestName, 174, "latin1"); b.writeInt32LE(dayCode, 204);
  return b;
}
const atToday = ((now.getUTCMonth() + 1) + now.getUTCFullYear() * 12) * 31 + now.getUTCDate();
const at = R.parse(R.RECORDS.atomica, new Uint8Array(Buffer.concat([
  atTable([40000, 25000, 15000, 10000, 5000], ["Ann", "Bo", "Cy", "Di", "Ed"], 777, "Zed", atToday),
  atTable([9, 8, 7, 6, 5], ["A", "B", "C", "D", "E"], 3, "Old", atToday - 1)])));
same("atomica: Strategy then Action", at.map((x) => x.name), ["Strategy", "Action"]);
same("atomica: today's best, then the five", at[0].rows.map((r) => [r.place, r.name, r.score]),
  [["trophies.today", "Zed", 777], ["1", "Ann", 40000], ["2", "Bo", 25000], ["3", "Cy", 15000], ["4", "Di", 10000], ["5", "Ed", 5000]]);
same("atomica: another day's best is not shown", at[1].rows.length, 5);
// Dynomite: a file a table, five 28-byte rows; Time Trial's minus hundredths shown as m:ss
function dyFile(rows) {
  const b = Buffer.alloc(140);
  rows.forEach(([name, v], i) => { b.writeInt32LE(1000 + i, i * 28); b.write(name, i * 28 + 4, "latin1"); b.writeInt32LE(v, i * 28 + 24); });
  return new Uint8Array(b);
}
const dySpecs = R.RECORDS.dynomite.files;
same("dynomite: four files in the dialog's order", dySpecs.map((f) => f.file), ["STOMPED.HS", "ENDLESS.HS", "FOSSIL.HS", "TIMETRIAL.HS"]);
const dyEnd = R.parse(dySpecs[1], dyFile([["Ann", 70000], ["Bo", 55000], ["Cy", 30000], ["Di", 20000], ["Ed", 10000]]));
same("dynomite: Endless's points", dyEnd[0].rows.map((r) => [r.place, r.name, r.score, r.text]), [["1", "Ann", 70000, undefined], ["2", "Bo", 55000, undefined],
  ["3", "Cy", 30000, undefined], ["4", "Di", 20000, undefined], ["5", "Ed", 10000, undefined]]);
const dyTime = R.parse(dySpecs[3], dyFile([["Ann", -42000], ["Bo", -60000], ["Cy", -72050], ["Di", -120000], ["Ed", -168000]]));
same("dynomite: Time Trial as m:ss", [dyTime[0].name, dyTime[0].rows.map((r) => r.text)], ["Time Trial", ["7:00", "10:00", "12:00", "20:00", "28:00"]]);
// Bejeweled Twist: a long 9, then ten Classic and ten Blitz entries (a short count and a UTF-8 name, level, score)
function twEntry(name, level, score) { const n = Buffer.from(name, "utf8"), b = Buffer.alloc(2 + n.length + 8); b.writeUInt16LE(n.length, 0); n.copy(b, 2); b.writeInt32LE(level, 2 + n.length); b.writeInt32LE(score, 6 + n.length); return b; }
const nine = Buffer.alloc(4); nine.writeInt32LE(9, 0);
const tw = R.parse(R.RECORDS.bejeweledtwist, new Uint8Array(Buffer.concat([nine, ...Array.from({ length: 20 }, (_, i) => twEntry(i === 10 ? "Zoë" : "T" + (i % 10), 3, (i < 10 ? 90000 : 50000) - (i % 10) * 1000))])));
same("bejeweledtwist: Classic and Blitz, ten each", tw.map((x) => x.name + x.rows.length), ["Classic10", "Blitz10"]);
same("bejeweledtwist: a UTF-8 name and its score", [tw[1].rows[0].name, tw[1].rows[0].score, tw[0].rows[9].score], ["Zoë", 50000, 81000]);
same("bejeweledtwist: another file is none", R.parse(R.RECORDS.bejeweledtwist, new Uint8Array([1, 0, 0, 0])), []);
// AstroPop: two strings, then entries (name, score, second, pilot, kind) to the end: kind 1 Classic, kind 2 Survival in m:ss
function apStr(s) { const n = Buffer.from(s, "latin1"), b = Buffer.alloc(2 + n.length); b.writeUInt16LE(n.length, 0); n.copy(b, 2); return b; }
function apEntry(name, score, sec, pilot, kind) { const b = Buffer.alloc(16); [score, sec, pilot, kind].forEach((v, i) => b.writeInt32LE(v, i * 4)); return Buffer.concat([apStr(name), b]); }
const ap = R.parse(R.RECORDS.astropop, new Uint8Array(Buffer.concat([apStr("AstroPop"), apStr("1.0"),
  apEntry("Ace", 100000, 12, 0, 1), apEntry("Bea", 90000, 9, 1, 1), apEntry("Cal", 30000, 754, 2, 2), apEntry("Dee", 20000, 61, 3, 2)])));
same("astropop: Classic by points, Survival by its time", ap.map((t) => [t.name, t.rows.map((r) => [r.place, r.name, r.text || r.score])]),
  [["Classic", [["1", "Ace", 100000], ["2", "Bea", 90000]]], ["Survival", [["1", "Cal", "12:34"], ["2", "Dee", "1:01"]]]]);
// Mahjong Escape: a player a file, big-endian; a record a u16 time (36000: none) and an i32 score; tables a layout or level won
function mjPlayer(name, classic, escape, nC, nE) {
  const n = Buffer.from(name, "latin1"), head = Buffer.concat([Buffer.from([0x4d, 0x45, 1, 0, 7]), n, Buffer.alloc(1 + 9)]);
  const recs = (list, count) => { const b = Buffer.alloc(count * 6); for (let i = 0; i < count; i++) { const r = list[i] || [36000, 0]; b.writeUInt16BE(r[0], i * 6); b.writeInt32BE(r[1], i * 6 + 2); } return b; };
  return new Uint8Array(Buffer.concat([head, recs(classic, nC), Buffer.alloc(3), recs(escape, nE), Buffer.alloc(5)]));
}
const mj = R.RECORDS.mahjongjapan;
const pa = R.parseMahjongPlayer(mj, mjPlayer("Ann", { 0: [125, 4000], 161: [3725, 900] }, { 2: [61, 50] }, 175, 60));
const pb = R.parseMahjongPlayer(mj, mjPlayer("Bo", { 0: [99, 5000] }, {}, 175, 60));
same("mahjong: a player's name and records", [pa.name, pa.classic.length, pa.escape.length, pa.classic[0], pa.escape[2]], ["Ann", 175, 60, { time: 125, score: 4000 }, { time: 61, score: 50 }]);
same("mahjong: another file is none", R.parseMahjongPlayer(mj, new Uint8Array([1, 2, 3, 4, 5, 6])), null);
const mjt = R.mahjongTables(mj, [pa, pb], ["First", "Second"]);
same("mahjong: layouts won, then levels, players by score with their times", mjt.map((t) => [t.name, t.rows.map((r) => [r.place, r.name, r.score, r.time])]),
  [["1. First", [["1", "Bo", 5000, "0:01:39"], ["2", "Ann", 4000, "0:02:05"]]], ["162. ", [["1", "Ann", 900, "1:02:05"]]], ["trophies.escape", [["1", "Ann", 50, "0:01:01"]]]]);
same("mahjong: Japan's layout ids", [mj.layoutId(0), mj.layoutId(124), mj.layoutId(125), mj.layoutId(161), R.RECORDS.mahjongchina.layoutId(125)], [437, 561, 600, 636, 562]);
same("mahjong: a LAYOUTS resource's name", R.layoutName(new Uint8Array([0x12, 0x34, 1, 0x41, 0x62, 0, 9])), "Ab");
const magic = Buffer.alloc(4); magic.writeUInt32LE(0xb9e7e3f8, 0);
const b2 = Buffer.concat([magic, ...Array.from({ length: 90 }, (_, i) => rec("B" + (i % 10), 100 - (i % 10), true))]);
same("bejeweled2: nine tables of ten after the magic", R.parse(R.RECORDS.bejeweled2, new Uint8Array(b2)).map((x) => x.rows.length), [10, 10, 10, 10, 10, 10, 10, 10, 10]);
same("bejeweled2: another file is none", R.parse(R.RECORDS.bejeweled2, new Uint8Array(b2.subarray(4))), []);
same("a short file gives what it holds", R.parse(R.RECORDS.mummymaze, new Uint8Array(Buffer.concat(five(500).slice(0, 2))))[0].rows.length, 2);
// Cosmic Bugs' and Water Bugs' cb.dat: exactly 49052 bytes, eight 36-byte high scores first
const cb = Buffer.alloc(0xbf9c);
for (let i = 0; i < 8; i++) { cb.write("Bug" + i, i * 36, "latin1"); cb.writeInt32LE(8000 - i * 1000, i * 36 + 32); }
same("cosmicbugs: the eight high scores", R.parse(R.RECORDS.cosmicbugs, new Uint8Array(cb))[0].rows.map((r) => r.name + r.score).slice(0, 2), ["Bug08000", "Bug17000"]);
same("waterbugs: a file of another size is none", R.parse(R.RECORDS.waterbugs, new Uint8Array(cb.subarray(1))), []);
// Typer Shark's users/user.idx: framework strings, the numbers as text
const fs16 = (t) => { const n = Buffer.from(t, "latin1"), b = Buffer.alloc(2 + n.length); b.writeUInt16LE(n.length, 0); n.copy(b, 2); return b; };
const tsEntry = (score, name) => [fs16(String(score)), fs16("1"), fs16(name), fs16("3")];
const ts = (v) => Buffer.concat([fs16(v), fs16("100"), ...Array.from({ length: 20 }, (_, i) => tsEntry(20000 - i * 100, "T" + i)).flat()]);
const tst = R.parse(R.RECORDS.typershark, new Uint8Array(ts("1.0")));
same("typershark: the score and abyss tables", tst.map((t) => [t.name, t.rows.length, t.rows[0].name, t.rows[0].score]), [["High Scores", 10, "T0", 20000], ["Abyss", 10, "T10", 19000]]);
same("typershark: a file before 0.48 is the defaults, so none", R.parse(R.RECORDS.typershark, new Uint8Array(ts("0.40"))), []);
// The registry file
const hex = (b) => Buffer.from(b).toString("hex");
const multi = (list) => Buffer.from(list.join("\0") + "\0\0", "latin1");
const ini = (lines) => new Uint8Array(Buffer.from(lines.join("\r\n") + "\r\n", "latin1"));
same("registry: sz escapes, dword, hex", (() => { const m = R.parseIni(ini(["; Settings", "", "[SOFTWARE\\A]", "S=sz:a\\\\b\\nc", "D=dword:-5", "H=hex:00ff"])); return [m["software\\a\\s"], m["software\\a\\d"], Array.from(m["software\\a\\h"])]; })(), ["a\\b\nc", -5, [0, 255]]);
const bw = R.parse(R.RECORDS.bookworm, ini(["[SOFTWARE\\PopCap\\BookWorm\\Scores]", "Action=hex:" + hex(multi(["Lex", "500", "Ann", "900"])),
  "Best=hex:" + hex(multi(["Ann", "QUIZ", "220"])), "Longest=hex:" + hex(multi(["Bo", "BOOKWORMS", "Ann", "ZEBRAS"]))]));
same("bookworm: tables by score, words in their column, Longest by the word's length, no empty table",
  bw.map((t) => [t.name, t.rows.map((r) => [r.place, r.name, r.word, r.score])]),
  [["Action", [["1", "Ann", undefined, 900], ["2", "Lex", undefined, 500]]], ["Best Word", [["1", "Ann", "QUIZ", 220]]], ["Longest Word", [["1", "Bo", "BOOKWORMS", 9], ["2", "Ann", "ZEBRAS", 6]]]]);
const np = (name, time, tiles) => { const b = Buffer.alloc(0x30); b.writeInt32LE(0x300, 0); b.write(name, 4, "latin1"); b.writeInt32LE(time, 0x24); b.writeInt32LE(time, 0x28); b.writeInt32LE(tiles, 0x2c); return hex(b); };
const npt = R.parse(R.RECORDS.ningpo, ini(["[SOFTWARE\\Pyrogon\\NingPo]", "HS:0x0=hex:" + np("nobody", 5999, 144), "HS:2x0=hex:" + np("Ann", 125, 0), "HS:2x1=hex:" + np("nobody", 5999, 144)]));
same("ningpo: only played places, by layout, the time as m:ss", npt.map((t) => [t.name, t.rows.map((r) => [r.place, r.name, r.text])]), [["trophies.layout", [["1", "Ann", "2:05"]]]]);
const wh = R.parse(R.RECORDS.wordharmony, ini(["[SOFTWARE\\GameHouse\\WordHarmony\\scores\\action]", "scoreName0=sz:Ann", "scoreValue0=dword:700", "scoreName1=sz:Bo", "scoreValue1=dword:0"]));
same("wordharmony: a table's rows, a score of 0 skipped", wh.map((t) => [t.name, t.rows.map((r) => [r.place, r.name, r.score])]), [["Action", [["1", "Ann", 700]]]]);
// Bonnie's Data/HighScores.dat: ten entries (used, name and word as an i32 length and bytes, score), Classic then Action
const s32 = (t) => { const n = Buffer.from(t, "latin1"), b = Buffer.alloc(4 + n.length); b.writeInt32LE(n.length, 0); n.copy(b, 4); return b; };
const i32 = (v) => { const b = Buffer.alloc(4); b.writeInt32LE(v, 0); return b; };
const bon = Buffer.concat(Array.from({ length: 10 }, (_, i) => Buffer.concat([i32(i === 4 ? 0 : 1), s32("N" + i), s32("WORD" + i), i32(1000 - i)])));
same("bonnies: Classic then Action, the longest word, an unused entry skipped",
  R.parse(R.RECORDS.bonnies, new Uint8Array(bon)).map((t) => [t.name, t.rows.length, t.rows[0].name, t.rows[0].word, t.rows[0].score]),
  [["Classic", 4, "N0", "WORD0", 1000], ["Action", 5, "N5", "WORD5", 995]]);
// BookWorm Adventures' users/<name>.bwa: key and typed value records; tables of {another follows, string key, key, value}
const u16s = (t) => { const n = Buffer.from(t, "latin1"), b = Buffer.alloc(2 + n.length); b.writeUInt16LE(n.length, 0); n.copy(b, 2); return b; };
const u32 = (v) => { const b = Buffer.alloc(4); b.writeUInt32LE(v, 0); return b; };
const vInt = (v) => Buffer.concat([u32(0), i32(v)]);
const vTable = (entries) => Buffer.concat([u32(4), ...entries.map(([k, v], i) => Buffer.concat([Buffer.from([i < entries.length - 1 ? 1 : 0, typeof k === "string" ? 1 : 0]), typeof k === "string" ? u16s(k) : i32(k), v]))]);
const fields = (o) => vTable(Object.keys(o).map((k) => [k, typeof o[k] === "string" ? Buffer.concat([u32(3), u16s(o[k])]) : vInt(o[k])]));
const bwa = Buffer.concat([u16s("XPLevel"), vInt(12), u16s("XPPercent"), u32(1), u16s("37.500000"), u16s("AdventureComplete"), u32(2), Buffer.from([1]),
  u16s("TopWordsEverSpelled"), vTable([[0, fields({ mWord: "QUIXOTIC", mValue: 90 })], [1, fields({ mWord: "ZAP", mValue: 40 })], [2, fields({ mWord: "ZAP", mValue: 40 })]]),
  u16s("TM_Top10Times"), vTable([[0, fields({ mTime: 372500, mStage: 7 })]]), u16s("Empty"), vTable([[0, u32(5)]])]);
const bp = R.parseBwa(new Uint8Array(bwa));
same("bwa: ints, floats, bools and tables", [bp.XPLevel, bp.XPPercent, bp.AdventureComplete, bp.TopWordsEverSpelled.length, bp.Empty], [12, 37.5, true, 3, [{ key: 0, value: null }]]);
same("bwa: a cut file is none", R.parseBwa(new Uint8Array(bwa.subarray(0, bwa.length - 3))), null);
const bwt = R.bwaTables(R.RECORDS.bookwormadv2, [{ name: "Ann", data: bp }, { name: "Bo", data: { TM_Top10Times: [{ key: 0, value: [{ key: "mTime", value: 100 }, { key: "mStage", value: 9 }] }] } }]);
same("bwa: top words once a player, best first; arena runs by stage with their times",
  bwt.map((t) => [t.name, t.rows.map((r) => [r.place, r.name, r.word, r.score, r.time])]),
  [["Top Words", [["1", "Ann", "QUIXOTIC", 90, undefined], ["2", "Ann", "ZAP", 40, undefined]]], ["Arena", [["1", "Bo", undefined, 9, "0:00:01"], ["2", "Ann", undefined, 7, "1:02:05"]]]]);
// Tip Top's Preferences.dat: four tables of five 28-byte entries, then the settings
const tt = Buffer.alloc(0x230 + 8);
for (let t = 0; t < 4; t++) for (let i = 0; i < 5; i++) {
  const at = (t * 5 + i) * 0x1c;
  tt.write(i < 2 ? "T" + t + i : "----", at, "latin1"); tt.writeInt32LE(i < 2 ? 900 - i * 100 + t : 0, at + 0x14); tt.writeInt32LE(1, at + 0x18);
}
same("tiptop: Regular, Speed and Strategy without Timed, empty places left out",
  R.parse(R.RECORDS.tiptop, new Uint8Array(tt)).map((t) => [t.name, t.rows.map((r) => [r.place, r.name, r.score])]),
  [["Regular", [["1", "T00", 900], ["2", "T01", 800]]], ["Speed", [["1", "T20", 902], ["2", "T21", 802]]], ["Strategy", [["1", "T30", 903], ["2", "T31", 803]]]]);
same("tiptop: a file shorter than the tables is none", R.parse(R.RECORDS.tiptop, new Uint8Array(tt.subarray(0, 0x22f))), []);
console.log("trophiestest: " + checks + " checks, " + failed + " failed");
process.exit(failed ? 1 : 0);
