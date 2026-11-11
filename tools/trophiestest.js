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
console.log("trophiestest: " + checks + " checks, " + failed + " failed");
process.exit(failed ? 1 : 0);
