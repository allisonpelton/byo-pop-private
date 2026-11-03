// Checks the pure parts of web/saveimport.js (the import of saves from an installed game, docs/WEB_PLAYER.md, "Importing saves from an
// installed game") and the table they read (platform/saveimport.json) under Node:
//   node tools/saveimporttest.js
// The table itself (every game, its patterns, no path that leaves the user folder), the .reg reader (version 5 in UTF-16 and REGEDIT4,
// string escapes, dword, hex, hex(2), hex(7), hex(b), long hex lines, subkeys, deleted keys and values), the merge of the VirtualStore
// copy over the machine's key, the conversion to the port's registry file (and that file's order and text, the same as
// SexyAppBase::GetRegistryText), the file patterns and the commands the page shows. It prints "saveimporttest: N checks, M failed" and
// exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const root = path.join(__dirname, "..");
require("./stringsload").intoGlobal();
const S = require(path.join(root, "web", "saveimport.js"));
const table = JSON.parse(fs.readFileSync(path.join(root, "platform", "saveimport.json"), "utf8"));
S.setTable(table);

let checks = 0, failed = 0;
function check(name, ok, detail) {
  checks++;
  if (!ok) { failed++; console.log("FAIL " + name + (detail !== undefined ? ": " + JSON.stringify(detail) : "")); }
}
function same(name, a, b) { check(name, JSON.stringify(a) === JSON.stringify(b), { got: a, want: b }); }

// a .reg file's bytes: version 5 is UTF-16 with a byte order mark, REGEDIT4 is ANSI
function reg5(text) {
  const u = new Uint8Array(2 + text.length * 2);
  u[0] = 0xff; u[1] = 0xfe;
  for (let i = 0; i < text.length; i++) { u[2 + i * 2] = text.charCodeAt(i) & 255; u[3 + i * 2] = text.charCodeAt(i) >> 8; }
  return u;
}
function reg4(text) { return Uint8Array.from(Buffer.from(text, "latin1")); }
const NL = "\r\n";

// ---------- the table ----------
const ids = Object.keys(table.games);
const gamesDir = fs.readdirSync(path.join(root, "games")).filter((d) => fs.statSync(path.join(root, "games", d)).isDirectory()).sort();
same("the table has every game", ids.slice().sort(), gamesDir);
ids.forEach((id) => {
  const g = table.games[id];
  check(id + " has a folder name", typeof g.folder === "string" && g.folder.length > 0);
  check(id + " has registry values or files", !!g.registry || (g.files && g.files.length > 0));
  (g.files || []).forEach((f) => {
    check(id + " file " + f.from + " is a relative path", !/^[\\/]|\.\.|\\/.test(f.from) && !/^[\\/]|\.\.|\\/.test(f.to), f);
    const nf = (f.from.match(/[*#]/g) || []).length, nt = (f.to.match(/[*#]/g) || []).length;
    check(id + " file " + f.from + " takes as many captures as it gives", nf === nt, f);
    check(id + " file " + f.from + " says what it is", typeof f.what === "string" && f.what.length > 0);
  });
  if (g.registry) {
    const r = S.registryOf(g);
    check(id + " has a registry key", /^[A-Za-z]+\\[A-Za-z0-9]+( [A-Za-z0-9]+)*$/.test(g.registry.key), g.registry.key);
    check(id + " hives are known", g.registry.hives.length > 0 && g.registry.hives.every((h) => h === "vs" || h === "hklm" || h === "hkcu"));
    check(id + " values have types", r.rules.every((x) => x.type === "sz" || x.type === "dword" || x.type === "hex"), r.rules.map((x) => x.name + "=" + x.type));
    // no installer value is a value the port takes
    ["InstallPath", "ReferId", "DownloadId", "RegisterLink", "DontUpdate", "NoPopLink", "Variation", "InProgress"].forEach((n) =>
      check(id + " does not take " + n, !r.rules.some((x) => x.re.test(n))));
  }
});

// ---------- the .reg reader ----------
const R5 = "Windows Registry Editor Version 5.00" + NL + NL +
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\BookWorm]" + NL +
  "\"InstallPath\"=\"C:\\\\Program Files\\\\PopCap Games\"" + NL +
  "\"MusicVolume\"=dword:00000032" + NL +
  "\"Muted\"=dword:ffffffff" + NL +
  "\"Odd\\\\Name\"=\"a \\\"quoted\\\" word\"" + NL +
  "@=\"default\"" + NL + NL +
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\BookWorm\\Users]" + NL +
  "\"User\"=\"Allison\"" + NL + NL +
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\BookWorm\\Users\\Allison]" + NL +
  "\"Alias\"=\"Allison\"" + NL +
  "\"ShowHelp\"=dword:00000001" + NL +
  "\"Long\"=hex:01,02,03,04,05,06,07,08,\\" + NL + "  09,0a,0b,0c" + NL +
  "\"Text\"=hex(2):41,00,42,00,00,00" + NL +
  "\"Many\"=hex(7):61,00,62,00,00,00,63,00,00,00,00,00" + NL +
  "\"Big\"=hex(b):01,00,00,00,00,00,00,00" + NL +
  "\"Odd\"=hex(5):00,00,00,01" + NL;
const p5 = S.parseReg(reg5(R5));
check("version 5 reads", p5.ok && p5.version === 5, p5);
const ev = p5.events.filter((e) => e.t === "value");
same("five values of the first key", ev.filter((e) => e.key.endsWith("\\BookWorm")).map((e) => e.name), ["InstallPath", "MusicVolume", "Muted", "Odd\\Name", ""]);
same("string escapes", ev.find((e) => e.name === "InstallPath").text, "C:\\Program Files\\PopCap Games");
same("quotes and backslashes in a name and a value", ev.find((e) => e.name === "Odd\\Name").text, "a \"quoted\" word");
same("dword", ev.find((e) => e.name === "MusicVolume").num, 50);
same("dword ffffffff is -1", ev.find((e) => e.name === "Muted").num, -1);
same("a long hex value goes on in the next line", S.bytesToBinary(S.binaryToBytes(ev.find((e) => e.name === "Long").bytes)).length, 12);
same("hex(2) is text", ev.find((e) => e.name === "Text"), { t: "value", key: "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\BookWorm\\Users\\Allison", name: "Text", kind: "expand", text: "AB" });
same("hex(7) is bytes", ev.find((e) => e.name === "Many").kind, "multi");
same("hex(b) is a 64-bit number", ev.find((e) => e.name === "Big").kind, "qword");
same("another kind of hex", ev.find((e) => e.name === "Odd" && e.kind === "other").type, 5);
check("the default value is read", ev.some((e) => e.name === "" && e.text === "default"));

const R4 = "REGEDIT4" + NL + NL + "[HKEY_CURRENT_USER\\Software\\PopCap\\WinBAD]" + NL +
  "\"LastUser\"=\"Ren\xe9e\"" + NL + "\"AppTime\"=dword:00000819" + NL + "\"Text\"=hex(2):41,42,00" + NL + NL +
  "[-HKEY_CURRENT_USER\\Software\\PopCap\\WinBAD\\Test3D]" + NL + "[HKEY_CURRENT_USER\\Software\\PopCap\\WinBAD]" + NL + "\"AppTime\"=-" + NL;
const p4 = S.parseReg(reg4(R4));
check("REGEDIT4 reads", p4.ok && p4.version === 4, p4);
same("REGEDIT4 is ANSI", p4.events.find((e) => e.name === "LastUser").text, "Ren\u00e9e");
same("REGEDIT4 hex(2) is ANSI", p4.events.find((e) => e.name === "Text").text, "AB");
check("a deleted key and a deleted value are read", p4.events.some((e) => e.t === "delkey") && p4.events.some((e) => e.t === "delvalue" && e.name === "AppTime"));
check("a file that is no .reg is refused", !S.parseReg(Uint8Array.from(Buffer.from("hello"))).ok);
check("a .reg of another version is refused", !S.parseReg(reg5("Windows Registry Editor Version 6.00" + NL)).ok);
const pu = S.parseReg(Uint8Array.from(Buffer.concat([Buffer.from([0xef, 0xbb, 0xbf]), Buffer.from("Windows Registry Editor Version 5.00\r\n\r\n[HKEY_CURRENT_USER\\Software\\PopCap\\WinBAD]\r\n\"LastUser\"=\"\u00e9\"\r\n", "utf8")])));
same("a UTF-8 .reg reads", pu.ok && pu.events.find((e) => e.name === "LastUser").text, "\u00e9");

// ---------- the places and the merge ----------
const bw = table.games.bookworm;
same("a key of the machine", S.hiveOf("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\BookWorm\\Users\\X", "PopCap\\BookWorm"), { hive: "hklm", rel: "Users\\X" });
same("a key of the machine on a 32-bit Windows", S.hiveOf("HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap\\BookWorm", "PopCap\\BookWorm"), { hive: "hklm", rel: "" });
same("the VirtualStore", S.hiveOf("HKEY_CURRENT_USER\\Software\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\BookWorm", "PopCap\\BookWorm"), { hive: "vs", rel: "" });
same("the user's key", S.hiveOf("HKEY_CURRENT_USER\\Software\\PopCap\\WinBAD\\Test3D", "PopCap\\WinBAD"), { hive: "hkcu", rel: "Test3D" });
same("a key of another game", S.hiveOf("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\BookWormX", "PopCap\\BookWorm"), null);
same("a key of another company", S.hiveOf("HKEY_LOCAL_MACHINE\\SOFTWARE\\Other\\PopCap\\BookWorm", "PopCap\\BookWorm"), null);

const VS = "Windows Registry Editor Version 5.00" + NL + NL +
  "[HKEY_CURRENT_USER\\Software\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\BookWorm]" + NL +
  "\"MusicVolume\"=dword:00000008" + NL + "\"TimesPlayed\"=dword:00000065" + NL + NL +
  "[HKEY_CURRENT_USER\\Software\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\Chuzzle]" + NL + "\"MusicVolume\"=dword:00000001" + NL;
const col = S.collect(bw, [S.parseReg(reg5(R5)), S.parseReg(reg5(VS))]);
const plan = S.planRegistry(bw, col, "");
const got = {};
plan.items.forEach((i) => { got[i.path] = i; });
same("the VirtualStore wins over the machine key, value by value", got["MusicVolume"].value, "dword:8");
check("the VirtualStore's value comes from the VirtualStore", got["MusicVolume"].hive === "vs" && got["Muted"].hive === "hklm");
same("a value only the machine has is taken", got["Muted"].value, "dword:-1");
same("the last player", got["Users\\User"].value, "sz:Allison");
same("a player's value", got["Users\\Allison\\ShowHelp"].value, "dword:1");
check("an installer value is not taken", !got["InstallPath"] && plan.skipped.some((s) => s.path === "InstallPath" && /installer/.test(s.why)), plan.skipped);
check("a value that is not in the table is left out with the reason", plan.skipped.some((s) => s.path === "Users\\Allison\\Long" && /^saveimport\.skip\.not_kept$/.test(s.why)));
same("one value was found in two places", plan.overridden, 1);
check("a key of another game is reported, not taken", col.foreign.length === 1 && /Chuzzle/.test(col.foreign[0]), col.foreign);
// a deleted key and value
const colDel = S.collect(table.games.bookwormadv, [S.parseReg(reg4(R4))]);
same("a deleted value is gone, a deleted key's values too", Object.keys(colDel.maps.hkcu).sort(), ["lastuser", "text"]);
// the type must match the table's
const typed = "Windows Registry Editor Version 5.00" + NL + NL + "[HKEY_CURRENT_USER\\Software\\PopCap\\WinBAD]" + NL + "\"LastUser\"=dword:00000001" + NL + "\"AppTime\"=\"7\"" + NL;
const pt = S.planRegistry(table.games.bookwormadv, S.collect(table.games.bookwormadv, [S.parseReg(reg5(typed))]), "");
check("a value of the wrong type is left out", pt.items.length === 0 && pt.skipped.length === 2 && /^saveimport\.skip\.not_(number|text) /.test(pt.skipped[0].why), pt);
// the binary values (BookWorm's scores are REG_MULTI_SZ)
const scores = "Windows Registry Editor Version 5.00" + NL + NL + "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\BookWorm\\Scores]" + NL + "\"Classic\"=hex(7):41,00,00,00,00" + NL;
const ps = S.planRegistry(bw, S.collect(bw, [S.parseReg(reg5(scores))]), "");
same("REG_MULTI_SZ is kept as the same bytes", ps.items.map((i) => i.value), ["hex:4100000000"]);
// the games that keep their values in the user's key and only there
const wh = table.games.wordharmony;
const whReg = "Windows Registry Editor Version 5.00" + NL + NL + "[HKEY_CURRENT_USER\\Software\\GameHouse\\WordHarmony]" + NL + "\"FullScreen\"=dword:00000000" + NL + "\"InstallCheck\"=\"true\"" + NL +
  "\"DefaultScoreName\"=\"Zo\xeb\"" + NL + "\"PuzzleSavedGame\"=hex(1):3c,00,61,00,3e,00,0a,00,00,00" + NL + NL + "[HKEY_CURRENT_USER\\Software\\GameHouse\\WordHarmony\\scores\\normal]" + NL + "\"scoreName0\"=\"Ann\"" + NL + "\"scoreValue0\"=dword:000003e8" + NL;
const pw = S.planRegistry(wh, S.collect(wh, [S.parseReg(reg5(whReg))]), "");
same("Word Harmony's values", pw.items.map((i) => i.path + "=" + i.value), ["DefaultScoreName=sz:Zo\xeb", "FullScreen=dword:0", "PuzzleSavedGame=sz:<a>\\n", "scores\\normal\\scoreName0=sz:Ann", "scores\\normal\\scoreValue0=dword:1000"]);
check("a promotional value is left out", S.planRegistry(wh, S.collect(wh, [S.parseReg(reg5(whReg.replace("\"FullScreen\"", "\"ShowLink\"=dword:00000001" + NL + "\"FullScreen\"")))]), "").skipped.some((s) => s.path === "ShowLink" && /promotional/.test(s.why)));
// text the games cannot keep
const wide = S.planRegistry(wh, S.collect(wh, [S.parseReg(reg5(whReg.replace("Zo\xeb", "\u4e2d")))]), "");
check("a character that Windows-1252 does not have is left out", wide.skipped.some((s) => s.path === "DefaultScoreName" && /^saveimport\.skip\.charset$/.test(s.why)));
same("a backslash and a line end are escaped as the port does", S.convert({ kind: "sz", text: "a\\b\nc\rd" }, "sz").value, "sz:a\\\\b\\nc\\rd");
same("the euro sign is the byte 0x80", S.toAnsi("\u20ac").charCodeAt(0), 0x80);

// ---------- the port's registry file ----------
const ini1 = "; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\\PopCap\\BookWorm]\nMusicVolume=dword:85\nTimesPlayed=dword:3\n\n[SOFTWARE\\PopCap\\BookWorm\\Users]\nUser=sz:Old\n";
const planIni = S.planRegistry(bw, col, ini1);
const bwIni = planIni.items.reduce((m, i) => { m[i.path] = i.status; return m; }, {});
same("a value that differs is a change, a missing one is new", [bwIni["MusicVolume"], bwIni["Muted"], bwIni["Users\\User"]], ["changes", "new", "changes"]);
const after = S.applyRegistry(planIni);
check("the registry file keeps its first line and starts a key with a blank line", after.indexOf("; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\\PopCap\\BookWorm]\n") === 0, after.slice(0, 120));
const reread = S.parseIni(after);
same("the file reads back", [reread["software\\popcap\\bookworm\\musicvolume"].value, reread["software\\popcap\\bookworm\\users\\user"].value], ["dword:8", "sz:Allison"]);
check("the file is the same when read and written again", S.writeIni(S.parseIni(after)) === after);
// the port's own file, read and written, is unchanged
const sample = path.join(root, "games", "bookwormadv", "tests", "menu", "user", "registry.ini");
if (fs.existsSync(sample)) {
  const text = fs.readFileSync(sample, "latin1").replace(/\r\n/g, "\n");
  check("the port's registry file is written as the port writes it", S.writeIni(S.parseIni(text)) === text, text.slice(0, 200));
}
// a value of another case keeps the existing name
const ini2 = "; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\\popcap\\bookworm]\nmusicvolume=dword:1\n";
const kept = S.applyRegistry(S.planRegistry(bw, col, ini2));
check("a value is found without regard to case and keeps its name", /\[SOFTWARE\\popcap\\bookworm\]\nmusicvolume=dword:8\n/.test(kept) && !/MusicVolume/.test(kept), kept);

// ---------- the files ----------
function files(list) { return list.map((r) => ({ rel: r, size: 10, m: 1, read: () => Promise.resolve(new Uint8Array(10)) })); }
let pf = S.planFiles(table.games.bigmoney, [files(["savegame.dat", "HISCORES.DAT", "data/x.xml", "images/a.png"])]);
same("Big Money's files", pf.take.map((f) => f.to), ["hiscores.dat", "savegame.dat"]);
same("other files are counted", pf.ignored, 2);
pf = S.planFiles(table.games.bejeweled, [files(["userdata/savegame.dat", "userdata/hiscores.dat"])]);
same("Bejeweled's files go to the user folder", pf.take.map((f) => f.to), ["hiscores.dat", "savegame.dat"]);
pf = S.planFiles(table.games.bookworm, [files(["players/Ann/classic.dat", "players/Ann/profile.dat", "players/Bo b/action.dat", "players/x/y/classic.dat"])]);
same("BookWorm's players", pf.take.map((f) => f.to), ["players/Ann/classic.dat", "players/Bo b/action.dat"]);
same("the Steam edition's files are left out with a reason", pf.skipped.map((s) => s.rel), ["players/Ann/profile.dat"]);
pf = S.planFiles(table.games.mummymaze, [files(["data/saves/adv0.sav", "data/saves/adv12.sav", "data/saves/advx.sav", "data/saves/classic.sav", "hiscores.dat"])]);
same("Mummy Maze's saves lose their folder", pf.take.map((f) => f.to), ["adv0.sav", "adv12.sav", "classic.sav", "hiscores.dat"]);
pf = S.planFiles(table.games.bookwormadv, [files(["users/Allison.bwa", "users/data/Allison.dat", "users/data/x.txt", "users/a/b.bwa"])]);
same("Bookworm Adventures' players", pf.take.map((f) => f.to), ["users/Allison.bwa", "users/data/Allison.dat"]);
pf = S.planFiles(table.games.noahsark, [files(["data/savegame.dat", "hiscores.dat"])]);
same("Noah's Ark's game in progress is left out", [pf.take.map((f) => f.to), pf.skipped.map((s) => s.rel)], [["hiscores.dat"], ["data/savegame.dat"]]);
pf = S.planFiles(table.games.bigmoney, [files(["savegame.dat"]), files(["savegame.dat"])]);
same("the second folder wins", pf.take.map((f) => f.folder), [1]);
pf = S.planFiles(table.games.dynomite, [files(["slot3.sav", "FOSSIL.HS", "SLOT.SAV"])]);
same("Dynomite's saved games are numbered", pf.take.map((f) => f.to), ["FOSSIL.HS", "SLOT3.SAV"]);

// ---------- nothing that the port's folder or registry file cannot hold ----------
pf = S.planFiles(table.games.bookworm, [files(["players/../classic.dat", "players/./action.dat", "players/Ann/classic.dat"])]);
same("a path that leaves the folder is left out", [pf.take.map((f) => f.to), pf.skipped.map((s) => s.rel)], [["players/Ann/classic.dat"], ["players/../classic.dat", "players/./action.dat"]]);
check("a name with an equals sign, a control character or a leading bracket is not held", S.badName("HS:0x1=x") && S.badName("a\u0001b") && S.badName("[x") && S.badName(";x") && S.badName("Users\\a\nb") && !S.badName("Users\\Ann\\Alias") && !S.badName("Sound Volume"));
const evil = "Windows Registry Editor Version 5.00" + NL + NL + "[HKEY_CURRENT_USER\\Software\\Pyrogon\\NingPo]" + NL + "\"HS:0x1=evil\"=hex:01" + NL + "\"HS:0x2\"=hex:02" + NL;
const pe = S.planRegistry(table.games.ningpo, S.collect(table.games.ningpo, [S.parseReg(reg5(evil))]), "");
same("a value whose name has an equals sign is left out", [pe.items.map((i) => i.path), pe.skipped.map((s) => s.path)], [["HS:0x2"], ["HS:0x1=evil"]]);

// ---------- what the page shows ----------
same("the commands for BookWorm", S.commands("bookworm").map((c) => c.text), [
  "reg export \"HKCU\\Software\\Classes\\VirtualStore\\MACHINE\\SOFTWARE\\WOW6432Node\\PopCap\\BookWorm\" bookworm-vs.reg /y",
  "reg export \"HKLM\\SOFTWARE\\WOW6432Node\\PopCap\\BookWorm\" bookworm-hklm.reg /y",
  "reg export \"HKCU\\Software\\PopCap\\BookWorm\" bookworm-hkcu.reg /y"]);
same("the commands for a game that uses only the user's key", S.commands("ningpo").map((c) => c.file), ["ningpo-hkcu.reg"]);
same("a game with no registry values has no commands", S.commands("tiptop"), []);

console.log("saveimporttest: " + checks + " checks, " + failed + " failed");
process.exitCode = failed ? 1 : 0;
