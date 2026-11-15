// soundfilestest: every sound the Sounds boards name (web/soundboards.js: each group's buttons and each preset's steps) is
// found among its game's staged files (assets/<game>/, a resource of a Windows module, an entry of a zip or of Tip Top's
// archive, as web/sounds.js finds them) and is in a format every browser plays: Ogg Vorbis, MP3 or PCM WAV, which every
// browser's decoder reads, or MS ADPCM WAV, MP3 inside a WAV or Sun .au, which the site decodes itself (web/music.js's msAdpcm
// and wavMp3, web/sounds.js's decodeAu). The format is judged by the file's own header and format tag, never by one
// browser's decoder: Firefox reads no WAV but PCM (Word Harmony's sounds, 2026-11-12). A game not staged is skipped.
//
//   node tools/soundfilestest.js [assets]
"use strict";
const fs = require("fs"), path = require("path"), zlib = require("zlib"), vm = require("vm");

const ROOT = path.resolve(__dirname, "..");
const ASSETS = path.resolve(process.argv[2] || path.join(ROOT, "assets"));
const EXTS = [".ogg", ".wav", ".au", ".mp3"];   // a preset's step without an extension: the first the game has (web/sounds.js)

const sandbox = { window: {} };
vm.runInNewContext(fs.readFileSync(path.join(ROOT, "web", "soundboards.js"), "utf8") + "\n;this.T = ByopopSoundboards;", sandbox);
const TABLE = sandbox.T;

// ---------- a game's staged files, by their lower-case path ----------
function listing(game) {
  const base = path.join(ASSETS, game), out = {};
  if (!fs.existsSync(base)) return null;
  (function walk(d) {
    for (const e of fs.readdirSync(d, { withFileTypes: true })) {
      const p = path.join(d, e.name);
      if (e.isDirectory()) walk(p);
      else out[path.relative(base, p).split(path.sep).join("/").toLowerCase()] = p;
    }
  })(base);
  return out;
}

function peResource(b, type, id) {
  if (b.readUInt16LE(0) !== 0x5a4d) return null;
  const pe = b.readUInt32LE(0x3c), nsec = b.readUInt16LE(pe + 6), opt = pe + 24, optSize = b.readUInt16LE(pe + 20);
  const dirs = opt + (b.readUInt16LE(opt) === 0x20b ? 112 : 96), rsrc = b.readUInt32LE(dirs + 16), secs = [];
  for (let s = 0; s < nsec; s++) {
    const h = opt + optSize + s * 40;
    secs.push([b.readUInt32LE(h + 12), Math.max(b.readUInt32LE(h + 8), b.readUInt32LE(h + 16)), b.readUInt32LE(h + 20)]);
  }
  const at = rva => { for (const [va, size, raw] of secs) if (rva >= va && rva < va + size) return rva - va + raw; return -1; };
  const base = at(rsrc);
  if (base < 0) return null;
  const nameOf = v => {
    if (!(v & 0x80000000)) return String(v);
    const o = base + (v & 0x7fffffff);
    return b.toString("utf16le", o + 2, o + 2 + b.readUInt16LE(o) * 2);
  };
  let hit = null;
  (function walk(dir, p) {
    const n = b.readUInt16LE(base + dir + 12) + b.readUInt16LE(base + dir + 14);
    for (let i = 0; i < n && !hit; i++) {
      const e = base + dir + 16 + i * 8, name = nameOf(b.readUInt32LE(e)), to = b.readUInt32LE(e + 4);
      if (to & 0x80000000) { if (p.length < 2) walk(to & 0x7fffffff, p.concat([name])); continue; }
      if (p[0] === type && p[1] === id) {
        const off = at(b.readUInt32LE(base + to)), size = b.readUInt32LE(base + to + 4);
        hit = b.subarray(off, off + size);
      }
    }
  })(0, []);
  return hit;
}

const TIPTOP_SRC = [2, 6, 0, 7, 5, 4, 1, 3], dec = Buffer.alloc(256);
for (let c = 0; c < 256; c++) { let enc = 0; for (let k = 0; k < 8; k++) enc |= ((c >> TIPTOP_SRC[k]) & 1) << k; dec[enc] = c; }
function tipTopEntry(b, name) {
  const want = name.replace(/\//g, "\\").toLowerCase();
  const u32 = o => (dec[b[o]] | dec[b[o + 1]] << 8 | dec[b[o + 2]] << 16 | dec[b[o + 3]] << 24) >>> 0;
  const end = 4 + u32(0);
  let p = 4;
  while (p < end) {
    let e = p, n = "";
    while (dec[b[e]]) n += String.fromCharCode(dec[b[e++]]);
    if (!n) break;
    const off = u32(e + 1), size = u32(e + 5);
    if (n.toLowerCase() === want) { const out = Buffer.alloc(size); for (let k = 0; k < size; k++) out[k] = dec[b[off + k]]; return out; }
    p = e + 9;
  }
  return null;
}

function zipEntry(b, name) {
  let end = -1;
  for (let i = b.length - 22; i >= Math.max(0, b.length - 65557); i--) if (b.readUInt32LE(i) === 0x06054b50) { end = i; break; }
  if (end < 0) return null;
  let c = b.readUInt32LE(end + 16);
  for (let k = 0, n = b.readUInt16LE(end + 10); k < n; k++) {
    const method = b.readUInt16LE(c + 10), size = b.readUInt32LE(c + 20), nl = b.readUInt16LE(c + 28);
    const el = b.readUInt16LE(c + 30), cl = b.readUInt16LE(c + 32), local = b.readUInt32LE(c + 42);
    if (b.toString("latin1", c + 46, c + 46 + nl).toLowerCase() === name.toLowerCase()) {
      const data = local + 30 + b.readUInt16LE(local + 26) + b.readUInt16LE(local + 28), packed = b.subarray(data, data + size);
      return method === 0 ? packed : zlib.inflateRawSync(packed);   // size: the stored (compressed) size
    }
    c += 46 + nl + el + cl;
  }
  return null;
}

// a board's file reference as [bytes, name] or [null, why]
function bytesOf(files, ref, preset) {
  const bar = ref.indexOf("|");
  if (bar < 0) {
    let p = files[ref.toLowerCase()];
    if (!p && preset) for (const x of EXTS) if ((p = files[ref.toLowerCase() + x])) break;
    return p ? [fs.readFileSync(p), p] : [null, "not among the staged files"];
  }
  const host = files[ref.slice(0, bar).toLowerCase()], pick = ref.slice(bar + 1);
  if (!host) return [null, "its container is not staged"];
  const b = fs.readFileSync(host);
  let out = null;
  try {
    if (pick.startsWith("pe:")) { const [, t, id] = pick.split(":"); out = peResource(b, t, id); }
    else if (pick.startsWith("tiptop:")) out = tipTopEntry(b, pick.slice(7));
    else if (pick.startsWith("zip:")) out = zipEntry(b, pick.slice(4));
  } catch (e) { return [null, "unreadable container: " + e.message]; }
  return out ? [out, ref] : [null, "not in its container"];
}

// the format of a sound's bytes, and whether the site plays it in every browser
function format(b) {
  if (b.length >= 4 && b.toString("latin1", 0, 4) === "OggS") {
    const head = b.toString("latin1", 0, Math.min(b.length, 200));
    return head.indexOf("\x01vorbis") >= 0 ? ["Ogg Vorbis", true] : ["Ogg, not Vorbis", false];
  }
  if (b.length >= 3 && (b.toString("latin1", 0, 3) === "ID3" || (b[0] === 0xff && (b[1] & 0xe0) === 0xe0))) return ["MP3", true];
  if (b.length >= 4 && b.toString("latin1", 0, 4) === ".snd") return ["Sun .au", true];
  if (b.length >= 12 && b.toString("latin1", 0, 4) === "RIFF" && b.toString("latin1", 8, 12) === "WAVE") {
    let at = 12, tag = -1, data = false;
    while (at + 8 <= b.length) {
      const id = b.toString("latin1", at, at + 4), len = b.readUInt32LE(at + 4);
      if (id === "fmt ") tag = b.readUInt16LE(at + 8);
      else if (id === "data") data = true;
      at += 8 + len + (len & 1);
    }
    if (!data) return ["WAV without data", false];
    if (tag === 1 || tag === 3) return ["PCM WAV", true];
    if (tag === 2) return ["MS ADPCM WAV", true];
    if (tag === 0x55) return ["MP3 in WAV", true];
    return ["WAV format 0x" + tag.toString(16), false];
  }
  return ["unknown (" + b.toString("hex", 0, 4) + ")", false];
}

let checks = 0, failed = 0, skipped = 0;
const kinds = {};
for (const game of Object.keys(TABLE)) {
  const files = listing(game);
  if (!files) { skipped++; continue; }
  const seen = new Set();
  const refs = [];
  for (const g of TABLE[game].groups || []) for (const s of g.sounds) refs.push([s[0], false, g.name + ": " + s[1]]);
  for (const p of TABLE[game].presets || []) for (const s of p.steps) refs.push([s[0], true, "preset " + p.name]);
  for (const [ref, preset, where] of refs) {
    if (seen.has(ref)) continue;
    seen.add(ref);
    checks++;
    const [b, why] = bytesOf(files, ref, preset);
    if (!b) { failed++; console.log("FAIL " + game + ": " + ref + " (" + where + "): " + why); continue; }
    const [kind, ok] = format(b);
    kinds[kind] = (kinds[kind] || 0) + 1;
    if (!ok) { failed++; console.log("FAIL " + game + ": " + ref + " (" + where + "): " + kind + ", which not every browser plays"); }
  }
}
console.log("formats: " + Object.keys(kinds).sort().map(k => k + " " + kinds[k]).join(", "));
console.log("soundfilestest: " + checks + " sounds, " + failed + " failed, " + skipped + " games not staged");
process.exit(failed ? 1 : 0);
