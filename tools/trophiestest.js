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
const magic = Buffer.alloc(4); magic.writeUInt32LE(0xb9e7e3f8, 0);
const b2 = Buffer.concat([magic, ...Array.from({ length: 90 }, (_, i) => rec("B" + (i % 10), 100 - (i % 10), true))]);
same("bejeweled2: nine tables of ten after the magic", R.parse(R.RECORDS.bejeweled2, new Uint8Array(b2)).map((x) => x.rows.length), [10, 10, 10, 10, 10, 10, 10, 10, 10]);
same("bejeweled2: another file is none", R.parse(R.RECORDS.bejeweled2, new Uint8Array(b2.subarray(4))), []);
same("a short file gives what it holds", R.parse(R.RECORDS.mummymaze, new Uint8Array(Buffer.concat(five(500).slice(0, 2))))[0].rows.length, 2);
console.log("trophiestest: " + checks + " checks, " + failed + " failed");
process.exit(failed ? 1 : 0);
