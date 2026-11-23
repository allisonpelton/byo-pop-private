// The music player's decoder (web/music/openmpt_web.c, build/web/openmpt.js) under Node: the tracker view's position and rows and
// the seek bar's seek, on a small ProTracker module made here (libxmp-lite's side) and, when the main checkout's staged files are
// there, a game's MO3 (libopenmpt's side); the MIDI player on a small file made here, and TipTop's song when its archive is
// staged; and with the staged files, every unused pattern the Music window lists (web/music.js's own tunesOf over each staged game),
// each one's tracker view at its first frame. node tools/musictest.js <build/web/openmpt.js> [assets dir]
"use strict";
const path = require("path"), fs = require("fs");
const [jsPath, assets] = process.argv.slice(2);
let checks = 0, failed = 0;
function check(name, ok, detail) {
  checks++;
  if (!ok) { failed++; console.log("FAIL " + name + (detail !== undefined ? ": " + JSON.stringify(detail) : "")); }
}

// A 4-channel ProTracker module: two patterns of 64 rows at speed 6, 125 BPM (each row 0.12 s, the tune 15.36 s); row 0 of
// pattern 0 plays sample 1 at period 428 with effect F06, and every 8th row of either pattern a note on channel 2
// hidden: a third pattern, in the order table past the song's two orders (a pattern no order plays), with a note on channel
// 1 every 16th row
function protracker(hidden) {
  const pat = 64 * 4 * 4, sampleWords = 32, pats = hidden ? 3 : 2;
  const b = Buffer.alloc(1084 + pats * pat + sampleWords * 2);
  b.write("byo-pop test", 0, "latin1");
  const s = 20;  // sample 1
  b.write("square", s, "latin1");
  b.writeUInt16BE(sampleWords, s + 22);
  b[s + 25] = 64;
  b.writeUInt16BE(0, s + 26);
  b.writeUInt16BE(sampleWords, s + 28);
  for (let i = 2; i <= 31; i++) b.writeUInt16BE(1, 20 + (i - 1) * 30 + 28);
  b[950] = 2; b[951] = 127;
  b[952] = 0; b[953] = 1;
  b.write("M.K.", 1080, "latin1");
  function cell(p, row, ch, period, sample, fx, fxp) {
    const at = 1084 + p * pat + (row * 4 + ch) * 4;
    b[at] = (sample & 0xf0) | ((period >> 8) & 0x0f);
    b[at + 1] = period & 0xff;
    b[at + 2] = ((sample & 0x0f) << 4) | (fx & 0x0f);
    b[at + 3] = fxp;
  }
  cell(0, 0, 0, 428, 1, 0xf, 6);
  for (let p = 0; p < 2; p++) for (let r = 0; r < 64; r += 8) cell(p, r, 2, 214, 1, 0, 0);
  if (hidden) { b[954] = 2; for (let r = 0; r < 64; r += 16) cell(2, r, 1, 320, 1, 0, 0); }
  const data = 1084 + pats * pat;
  for (let i = 0; i < sampleWords * 2; i++) b[data + i] = i < sampleWords ? 0x40 : 0xc0;
  return b;
}

(async () => {
  const ByopopOpenMPT = require(path.resolve(jsPath));
  const M = await ByopopOpenMPT({ locateFile: (f) => path.join(path.dirname(path.resolve(jsPath)), f) });
  function load(bytes) { const p = M._malloc(bytes.length); M.HEAPU8.set(bytes, p); const m = M._bm_load(p, bytes.length); M._free(p); return m; }
  const viewBuf = M._malloc(20);
  function view(m) { M._bm_view(m, viewBuf); const v = M.HEAP32.subarray(viewBuf >> 2, (viewBuf >> 2) + 5); return { pattern: v[0], row: v[1], rows: v[2], channels: v[3], order: v[4] }; }
  function rowText(m, p, r) { return M.UTF8ToString(M._bm_row_text(m, p, r)); }
  // bm_used's channels, and the same found from the rows' text: a channel with a note (A-G, then - or #, then an octave) anywhere
  function used(m) {
    const buf = M._malloc(256), n = M._bm_used(m, buf, 256), list = [];
    for (let c = 0; c < n; c++) if (M.HEAPU8[buf + c]) list.push(c);
    M._free(buf);
    return list;
  }
  function usedByText(m) {
    const found = new Set();
    for (let p = 0; p < 256; p++) {
      for (let r = 0; r < 256; r++) {
        const t = rowText(m, p, r);
        if (!t) break;
        t.split("|").forEach((cell, c) => { if (/^[A-G][-#]\d/.test(cell)) found.add(c); });
      }
    }
    return [...found].sort((a, b) => a - b);
  }
  const RATE = 48000, out = M._malloc(RATE * 8);
  function render(m, seconds) { let n = Math.round(seconds * RATE), got = 0; while (n > 0) { const k = Math.min(n, RATE); const g = M._bm_render(m, RATE, k, out); got += g; if (g < k) break; n -= k; } return got; }

  // ---------- libxmp-lite: the ProTracker module ----------
  const m = load(protracker());
  check("mod: loads", !!m);
  M._bm_select(m, 0);
  check("mod: its length", Math.abs(M._bm_duration(m) - 15.36) < 0.05, M._bm_duration(m));
  let v = view(m);
  check("mod: the view at the start", v.pattern === 0 && v.row === 0 && v.rows === 64 && v.channels === 4 && v.order === 0, v);
  let row = rowText(m, 0, 0).split("|");
  check("mod: a row has a cell a channel", row.length === 4, row);
  check("mod: row 0's first cell (note, sample 01, effect F06)", /^[A-G][-#]\d 01 \.\.\. 0F06$/.test(row[0]), row[0]);
  check("mod: an empty cell", row[1] === "... .. ... ....", row[1]);
  check("mod: channel 3's note on row 0", /^[A-G][-#]\d 01/.test(row[2]), row[2]);
  check("mod: an empty row", rowText(m, 0, 1).split("|").every((c) => c === "... .. ... ...."), rowText(m, 0, 1));
  render(m, 1.0);
  v = view(m);
  check("mod: about 8 rows in a second", v.order === 0 && v.row >= 7 && v.row <= 9, v);
  let at = M._bm_seek(m, RATE, 8.0);
  v = view(m);
  check("mod: seeks to 8 s", Math.abs(at - 8.0) < 0.05, at);
  check("mod: 8 s is pattern 1's row 2", v.order === 1 && v.pattern === 1 && v.row >= 2 && v.row <= 3, v);
  at = M._bm_seek(m, RATE, 1.0);
  check("mod: seeks back", Math.abs(at - 1.0) < 0.05 && view(m).order === 0, [at, view(m)]);
  check("mod: plays on after a seek", render(m, 0.5) === RATE / 2);
  check("mod: a row out of range is empty", rowText(m, 9, 0) === "", rowText(m, 9, 0));
  check("mod: the channels with notes", JSON.stringify(used(m)) === JSON.stringify(usedByText(m)) && used(m).length > 0, [used(m), usedByText(m)]);
  M._bm_free(m);

  // ---------- an unused pattern played alone (bm_solo; the music player's own section, the user, 2026-11-15) ----------
  const h = load(protracker(true));
  check("solo: the hidden pattern is a pattern no order plays", M._bm_patterns(h) === 3 && M._bm_orders(h) === 2 &&
        [0, 1].every((o) => M._bm_order_pattern(h, o) !== 2) && M._bm_pattern_notes(h, 2) === 1, [M._bm_patterns(h), M._bm_orders(h)]);
  check("solo: plays it alone, its orders that one", M._bm_solo(h, 2) === 64 && M._bm_orders(h) === 1 && M._bm_order_pattern(h, 0) === 2 &&
        view(h).pattern === 2);
  render(h, 1.0);
  check("solo: about 8 rows in a second, in the pattern", view(h).pattern === 2 && view(h).row >= 7 && view(h).row <= 9, view(h));
  at = M._bm_seek(h, RATE, 4.0);
  check("solo: seeks inside it", Math.abs(at - 4.0) < 0.05 && view(h).pattern === 2 && view(h).row >= 32 && view(h).row <= 34, [at, view(h)]);
  M._bm_seek(h, RATE, 0);
  let alone = render(h, 30) / RATE;
  check("solo: once through, then it ends (64 rows of 0.12 s)", Math.abs(alone - 7.68) < 0.05, alone);
  M._bm_solo(h, -1);
  check("solo: the module put back", M._bm_orders(h) === 2 && M._bm_order_pattern(h, 0) === 0 && M._bm_order_pattern(h, 1) === 1);
  M._bm_free(h);

  // ---------- libopenmpt: a game's MO3, when the staged files are there ----------
  let mo3 = null;
  const roots = assets ? [assets] : [];
  for (const root of roots) {
    const stack = [root];
    while (stack.length && !mo3) {
      const d = stack.pop();
      let names = [];
      try { names = fs.readdirSync(d, { withFileTypes: true }); } catch (e) { continue; }
      for (const n of names) {
        const p = path.join(d, n.name);
        if (n.isDirectory()) stack.push(p);
        else if (/\.mo3$/i.test(n.name)) { mo3 = p; break; }
      }
    }
  }
  if (mo3) {
    const t = load(fs.readFileSync(mo3));
    check("mo3: loads", !!t, mo3);
    M._bm_select(t, 0);
    v = view(t);
    check("mo3: the view at the start", v.row === 0 && v.rows > 0 && v.channels > 0, v);
    row = rowText(t, v.pattern, 0).split("|");
    check("mo3: a cell a channel", row.length === v.channels, [row.length, v.channels]);
    check("mo3: the channels with notes", JSON.stringify(used(t)) === JSON.stringify(usedByText(t)) && used(t).length > 0, [used(t), usedByText(t)]);
    const len = M._bm_duration(t), half = Math.min(len / 2, 30);
    at = M._bm_seek(t, RATE, half);
    check("mo3: seeks", Math.abs(at - half) < 0.5 && Math.abs(M._bm_position(t) - half) < 0.5, [at, half]);
    // its first order's pattern alone: libopenmpt's order list set aside for that one, then put back
    const orders = M._bm_orders(t), first = M._bm_order_pattern(t, 0), rows = M._bm_pattern_rows(t, first);
    check("mo3 solo: the pattern alone", M._bm_solo(t, first) === rows && M._bm_orders(t) === 1 && M._bm_order_pattern(t, 0) === first, [orders, first]);
    let secs = 0;
    for (let k = 0; k < 600; k++) { const g = M._bm_render(t, 8000, 8000, out); secs += g / 8000; if (g < 8000) break; }
    check("mo3 solo: once through, then it ends", secs > 0.5 && secs < 120, secs);
    M._bm_solo(t, -1);
    check("mo3 solo: the order list put back", M._bm_orders(t) === orders && M._bm_order_pattern(t, 0) === first);
    M._bm_free(t);
  }

  // ---------- music.js's readers of tunes inside a game's files: a Windows module's resources, a zip archive's entries ----------
  const src = fs.readFileSync(path.join(__dirname, "../web/music.js"), "utf8");
  function grab(name) {
    const a = src.indexOf("function " + name + "(");
    let d = 0, i = src.indexOf("{", a);
    for (; i < src.length; i++) { if (src[i] === "{") d++; else if (src[i] === "}" && --d === 0) break; }
    return src.slice(a, i + 1);
  }
  const R = new Function("var TIPTOP_SRC = [2, 6, 0, 7, 5, 4, 1, 3], tipTopDecode = null;" + grab("peResources") + grab("zipEntry") +
    grab("baseName") + grab("tipTopEntry") + "return { peResources, zipEntry, baseName, tipTopEntry };")();
  check("tiptop: an entry's name", R.baseName("TipTop.dat|tiptop:sounds/bkgd.mid") === "bkgd", R.baseName("TipTop.dat|tiptop:sounds/bkgd.mid"));

  // ---------- MIDI (web/music/midi_web.c): TipTop's sequencer into TinySoundFont, with the port's bank beside the pages ----------
  // A format 1 file made here: one track at 96 ticks a beat and 120 BPM, a note of a beat on channel 1, a drum on channel 10,
  // and the track's end at 4 beats (2 s)
  function midiFile() {
    const ev = [0x00, 0xff, 0x51, 0x03, 0x07, 0xa1, 0x20, 0x00, 0xb0, 0x07, 0x64, 0x00, 0x90, 0x3c, 0x64, 0x00, 0x99, 0x26, 0x64,
      0x60, 0x80, 0x3c, 0x00, 0x00, 0x89, 0x26, 0x00, 0x82, 0x20, 0xff, 0x2f, 0x00];
    const b = Buffer.alloc(22 + ev.length);
    b.write("MThd", 0, "latin1"); b.writeUInt32BE(6, 4); b.writeUInt16BE(1, 8); b.writeUInt16BE(1, 10); b.writeUInt16BE(96, 12);
    b.write("MTrk", 14, "latin1"); b.writeUInt32BE(ev.length, 18); Buffer.from(ev).copy(b, 22);
    return b;
  }
  function put(b) { const p = M._malloc(b.length); M.HEAPU8.set(b, p); return p; }
  function midiSeconds(b) { const p = put(b), s = M._bm_midi_seconds(p, b.length); M._free(p); return s; }
  const bankPath = path.join(path.dirname(jsPath), "soundfonts", "tiptop.sf2");
  const bank = fs.existsSync(bankPath) ? fs.readFileSync(bankPath) : null;
  check("midi: the bank beside the pages", !!bank, bankPath);
  function midiLoad(b) { const f = put(b), k = put(bank), h = M._bm_midi_load(f, b.length, k, bank.length, RATE); M._free(f); M._free(k); return h; }
  // renders until the end or for seconds; the frames and every chunk's length
  function midiRender(h, chunk, seconds) {
    const buf = M._malloc(chunk * 8), pcm = [], sizes = [];
    let frames = 0, n;
    while (frames < RATE * seconds && (n = M._bm_midi_render(h, chunk, buf)) > 0) {
      pcm.push(Float32Array.from(M.HEAPF32.subarray(buf >> 2, (buf >> 2) + n * 2)));
      sizes.push(n);
      frames += n;
    }
    M._free(buf);
    const all = new Float32Array(frames * 2);
    let at = 0;
    for (const a of pcm) { all.set(a, at); at += a.length; }
    return { frames, all, sizes };
  }
  const mf = midiFile();
  check("midi: the song's length as the sequencer plays it", Math.abs(midiSeconds(mf) - 2) < 0.002, midiSeconds(mf));
  check("midi: not a MIDI file", midiSeconds(Buffer.from("MThd not really a midi file")) === 0 && !midiLoad(Buffer.from("RIFF....")));
  if (bank) {
    let h = midiLoad(mf);
    check("midi: loads", !!h);
    M._bm_midi_loop(h, 0);
    const once = midiRender(h, 4096, 10);
    let peak = 0;
    for (const v of once.all) peak = Math.max(peak, Math.abs(v));
    check("midi: once through, then it ends", Math.abs(once.frames / RATE - 2) < 0.002 && M._bm_midi_render(h, 4096, 0) === 0, once.frames / RATE);
    check("midi: the notes sound", peak > 0.01 && peak < 1, peak);
    // the same frames whatever the chunks: each millisecond is rendered whole, as the port's musicwav renders it
    M._bm_midi_seek(h, 0);
    const odd = midiRender(h, 1000, 10);
    check("midi: the same in any chunks", odd.frames === once.frames && odd.all.every((v, i) => v === once.all[i]));
    // repeat on: the song plays on from its start, the chunk before the start again cut there, the position back to 0
    M._bm_midi_seek(h, 0);
    M._bm_midi_loop(h, 1);
    const twice = midiRender(h, 4096, 3);
    const cut = twice.sizes.findIndex((n) => n < 4096);
    check("midi: repeat plays on", twice.frames >= RATE * 3 && cut >= 0 && twice.sizes.slice(0, cut + 1).reduce((a, n) => a + n, 0) === once.frames &&
      Math.abs(M._bm_midi_position(h) - (twice.frames - once.frames) / RATE) < 0.001, [twice.frames, cut, M._bm_midi_position(h)]);
    M._bm_midi_seek(h, 1.5);
    check("midi: seeks", Math.abs(M._bm_midi_position(h) - 1.5) < 0.002, M._bm_midi_position(h));
    M._bm_midi_free(h);
    // TipTop's own song, when its archive is staged: 168.8 s once through (games/tiptop/docs/README.md)
    const dat = assets ? path.join(assets, "tiptop", "TipTop.dat") : "";
    if (dat && fs.existsSync(dat)) {
      const song = R.tipTopEntry(new Uint8Array(fs.readFileSync(dat)), "sounds/bkgd.mid");
      check("midi: TipTop's song from its archive", !!song && song.length > 0);
      if (song) check("midi: TipTop's song is 168.8 s", Math.abs(midiSeconds(Buffer.from(song)) - 168.843) < 0.01, midiSeconds(Buffer.from(song)));
    }
  }
  // places numbered as the game reaches them, in that order where the files' order put them; the others where they were
  const NB = new Function(grab("numbered") + "return numbered;")();
  check("numbered places in order", NB([{ where: "Main Menu" }, { where: "Level Theme 3" }, { where: "Level Theme 2" }, { where: "" }, { where: "Level Theme 1" }])
    .map((k) => k.where).join(",") === "Main Menu,Level Theme 1,Level Theme 2,,Level Theme 3");
  // an MS ADPCM WAV: one mono block (predictor 1 of 2, step 16, samples 100 then 50), codes 0, 1 and 15, and a fact chunk of 5
  const A = new Function(src.match(/var ADAPT = \[[^\]]*\];/)[0] + grab("msAdpcm") + "return msAdpcm;")();
  const fmtc = Buffer.alloc(30); fmtc.writeUInt16LE(2, 0); fmtc.writeUInt16LE(1, 2); fmtc.writeUInt32LE(22050, 4); fmtc.writeUInt16LE(9, 12);
  fmtc.writeUInt16LE(4, 14); fmtc.writeUInt16LE(4, 18); fmtc.writeUInt16LE(2, 20); fmtc.writeInt16LE(256, 22); fmtc.writeInt16LE(0, 24); fmtc.writeInt16LE(512, 26); fmtc.writeInt16LE(-256, 28);
  const blk = Buffer.from([1, 16, 0, 100, 0, 50, 0, 0x01, 0xf0]);
  const chunk = (id, b) => { const h = Buffer.alloc(8); h.write(id, 0, "latin1"); h.writeUInt32LE(b.length, 4); return Buffer.concat([h, b]); };
  const fact = Buffer.alloc(4); fact.writeUInt32LE(5, 0);
  const wbody = Buffer.concat([Buffer.from("WAVE", "latin1"), chunk("fmt ", fmtc), chunk("fact", fact), chunk("data", blk)]);
  const riff = Buffer.concat([Buffer.from("RIFF", "latin1"), Buffer.alloc(4), wbody]); riff.writeUInt32LE(wbody.length, 4);
  const ad = A(new Uint8Array(riff));
  // codes 0, 1, 15, 0: 2*100 - 50 = 150 (the step 16*230/256 = 14, kept at 16); 2*150 - 100 + 16 = 216; 2*216 - 150 - 16 = 266; the fact ends it
  check("adpcm: a block's samples", ad && ad.rate === 22050 && JSON.stringify(Array.from(ad.channels[0], (v) => Math.round(v * 32768))) === "[50,100,150,216,266]", ad && Array.from(ad.channels[0], (v) => Math.round(v * 32768)));
  check("adpcm: not a WAV of it", A(new Uint8Array(Buffer.from("RIFF\0\0\0\0WAVEfmt \x10\0\0\0\x01\0", "latin1"))) === null);
  // a PE32 module with one section (.rsrc at RVA 0x1000, file offset 0x200) holding type 10 > id 335 > language 0, 8 bytes
  const pe = Buffer.alloc(0x200 + 0x100);
  pe.write("MZ", 0, "latin1"); pe.writeUInt32LE(0x40, 0x3c); pe.write("PE\0\0", 0x40, "latin1");
  pe.writeUInt16LE(1, 0x46); pe.writeUInt16LE(224, 0x54); pe.writeUInt16LE(0x10b, 0x58);
  pe.writeUInt32LE(0x1000, 0x58 + 96 + 16); pe.writeUInt32LE(0x100, 0x58 + 96 + 20);
  const sec = 0x58 + 224;
  pe.writeUInt32LE(0x100, sec + 8); pe.writeUInt32LE(0x1000, sec + 12); pe.writeUInt32LE(0x100, sec + 16); pe.writeUInt32LE(0x200, sec + 20);
  function dir(at, id, to) { pe.writeUInt16LE(1, 0x200 + at + 14); pe.writeUInt32LE(id, 0x200 + at + 16); pe.writeUInt32LE(to, 0x200 + at + 20); }
  dir(0x00, 10, 0x80000018); dir(0x18, 335, 0x80000030); dir(0x30, 0, 0x48);
  pe.writeUInt32LE(0x1060, 0x248); pe.writeUInt32LE(8, 0x24c);
  pe.write("MO3 data", 0x260, "latin1");
  const res = R.peResources(new Uint8Array(pe));
  check("pe: one resource", res.length === 1 && res[0].type === 10 && res[0].id === 335, res.map((r) => [r.type, r.id]));
  check("pe: its bytes", res.length === 1 && Buffer.from(res[0].bytes).toString("latin1") === "MO3 data");
  check("pe: not a module", R.peResources(new Uint8Array(Buffer.from("not a module at all, really"))).length === 0);
  check("pe: a resource's name", R.baseName("Resource.dll|pe:10:335") === "Resource 335", R.baseName("Resource.dll|pe:10:335"));
  // a zip with a stored and a deflated entry
  const zlib = require("zlib"), body = Buffer.from("OggS ".repeat(40), "latin1"), entries = [["a.txt", body, 0], ["sub/Theme.ogg", zlib.deflateRawSync(body), 8]];
  const locals = [], centrals = [];
  let off = 0;
  for (const [name, data, method] of entries) {
    const l = Buffer.alloc(30); l.writeUInt32LE(0x04034b50, 0); l.writeUInt16LE(method, 8); l.writeUInt32LE(data.length, 18); l.writeUInt32LE(body.length, 22); l.writeUInt16LE(name.length, 26);
    const c = Buffer.alloc(46); c.writeUInt32LE(0x02014b50, 0); c.writeUInt16LE(method, 10); c.writeUInt32LE(data.length, 20); c.writeUInt32LE(body.length, 24); c.writeUInt16LE(name.length, 28); c.writeUInt32LE(off, 42);
    locals.push(l, Buffer.from(name, "latin1"), data); centrals.push(c, Buffer.from(name, "latin1"));
    off += 30 + name.length + data.length;
  }
  const cd = Buffer.concat(centrals), eocd = Buffer.alloc(22);
  eocd.writeUInt32LE(0x06054b50, 0); eocd.writeUInt16LE(2, 10); eocd.writeUInt32LE(cd.length, 12); eocd.writeUInt32LE(off, 16);
  const zip = new Uint8Array(Buffer.concat(locals.concat([cd, eocd])));
  const stored = await R.zipEntry(zip, "a.txt"), deflated = await R.zipEntry(zip, "SUB/theme.ogg"), none = await R.zipEntry(zip, "b.txt");
  check("zip: a stored entry", stored && Buffer.from(stored).equals(body));
  check("zip: a deflated entry, its name in any case", deflated && Buffer.from(deflated).equals(body));
  check("zip: no such entry", none === null);
  check("zip: an entry's name", R.baseName("ningpo.pyz|zip:theme_np.ogg") === "theme_np", R.baseName("ningpo.pyz|zip:theme_np.ogg"));

  // ---------- the places in web/soundtracks.js: rip style (the user, 2026-11-11: "Rip style please!") ----------
  const table = new Function(fs.readFileSync(path.join(__dirname, "../web/soundtracks.js"), "utf8") + "\nreturn ByopopSoundtracks;")();
  const places = [];
  Object.keys(table).forEach((g) => Object.keys(table[g].where || {}).forEach((k) => places.push([g, k, table[g].where[k]])));
  check("places: some", places.length > 100, places.length);
  const long = places.filter((p) => p[2].length > 20);
  check("places: at most 20 characters", long.length === 0, long);
  // Title Case: every word starts with a capital or a digit, but for "of" and "to" after the first (Hall of Fame, Venice's
  // Welcome to Venice)
  const lower = places.filter((p) => p[2].split(/[\s-]+/).some((w, i) => !(i > 0 && (w === "of" || w === "to")) && !/^[A-Z0-9]/.test(w)));
  check("places: Title Case", lower.length === 0, lower);
  const unusedPlaced = Object.keys(table).reduce((a, g) => a.concat((table[g].unused || []).filter((k) => (table[g].where || {})[k])), []);
  check("places: no unused tune placed", unusedPlaced.length === 0, unusedPlaced);
  // every "<file>#<n>@<order>" named anywhere is a piece some cut makes, and no chained part is placed on its own
  const orphans = [], chained = [];
  for (const g of Object.keys(table)) {
    const e = table[g], cuts = Object.assign({}, e.cuts || {});
    for (const k of Object.keys(e.starts || {})) cuts[k] = (cuts[k] || []).concat(e.starts[k]);
    const keys = Object.keys(e.where || {}).concat(e.unused || [], e.skip || [], Object.keys(e.names || {}));
    for (const k of keys) {
      const at = k.lastIndexOf("@");
      if (at >= 0 && !(cuts[k.slice(0, at)] || []).includes(+k.slice(at + 1))) orphans.push(g + " " + k);
    }
    for (const head of Object.keys(e.chains || {})) for (const k of e.chains[head]) if ((e.where || {})[k]) chained.push(g + " " + k);
  }
  check("cuts: every piece named is cut or started", orphans.length === 0, orphans);
  check("chains: no part placed on its own", chained.length === 0, chained);

  // the soundboards' presets (web/soundboards.js): rip-style names (Title Case, at most 20 characters), steps in time order,
  // and every step's file a sound on the game's own board (with or without its extension)
  const boards = new Function(fs.readFileSync(path.join(__dirname, "../web/soundboards.js"), "utf8") + "\nreturn ByopopSoundboards;")();
  const badNames = [], unordered = [], offBoard = [];
  let presetCount = 0;
  for (const g of Object.keys(boards)) {
    const listed = new Set();
    for (const gr of boards[g].groups) for (const s of gr.sounds) listed.add(s[0].toLowerCase().replace(/\.(ogg|wav|au|mp3)$/, ""));
    for (const pr of boards[g].presets || []) {
      presetCount++;
      // (a count, "x5", is the one word that starts small: "Cascade x5", the coordinator's example of 2026-11-11)
      if (pr.name.length > 20 || pr.name.split(" ").some((w) => !/^([A-Z0-9]|x\d+$)/.test(w) && w !== "of")) badNames.push(g + " " + pr.name);
      for (let i = 1; i < pr.steps.length; i++) if (pr.steps[i][1] < pr.steps[i - 1][1]) { unordered.push(g + " " + pr.name); break; }
      for (const st of pr.steps) if (!listed.has(st[0].toLowerCase().replace(/\.(ogg|wav|au|mp3)$/, ""))) offBoard.push(g + " " + pr.name + " " + st[0]);
    }
  }
  check("presets: " + presetCount + " named rip style", presetCount > 0 && badNames.length === 0, badNames);
  check("presets: steps in time order", unordered.length === 0, unordered);
  check("presets: every file on the game's board", offBoard.length === 0, offBoard);

  // ---------- every unused pattern of every staged game, as the Music window lists it and shows it ----------
  // (the user, 2026-11-22, of Ancient China's resource 339: "pattern 12 ... only shows an empty channel in the music player")
  // web/music.js's own listing (tunesOf) reads the staged files through a stand-in for the game's database (tools/web/stageddb.js).
  // Each entry is then played as the player plays it (a fresh module, bm_solo), and the tracker view's first frame must show the
  // pattern the entry names, from its first row, with the channels it draws (tuneUsed, else bm_used; channelsShown) each holding a
  // note in that pattern. (Pattern 12 passes: its one note, on row 0, starts a sample that plays through its 128 rows, so after
  // the first rows the one column is empty, as the file has it.)
  let patternEntries = 0, patternGames = 0;
  if (assets) {
    const staged = require("./web/stageddb.js").install(assets);
    const Music = require("vm").runInThisContext(src + "\n;ByopopMusic", { filename: "web/music.js" });
    const wrong = [];
    for (const id of Object.keys(table)) {
      if (!staged(id)) continue;
      const game = { id: id, title: id, persist: "staged:" + id };
      const solos = (await Music.test.tunesOf(M, game, table[id])).filter((t) => t.solo !== undefined);
      if (!solos.length) continue;
      patternGames++;
      const db = await Music.files.open(game.persist);
      for (const t of solos) {
        patternEntries++;
        const name = id + " " + t.id;
        const bytes = await Music.files.read(db, t.keys[0]), m = bytes ? load(bytes) : 0;
        if (!m || !M._bm_solo(m, t.solo)) { wrong.push(name + ": does not play"); if (m) M._bm_free(m); continue; }
        const v = view(m), want = M._bm_pattern_rows(m, t.solo);
        const rowsOf = (o) => { const p = M._bm_order_pattern(m, o); return p >= 0 ? M._bm_pattern_rows(m, p) : 0; };
        const usedHere = Music.test.tuneUsed(t, (p, r) => rowText(m, p, r), rowsOf) || used(m);
        const shown = Music.test.channelsShown(usedHere, rowText(m, v.pattern, v.row).split("|").length);
        const noted = new Set();
        for (let r = 0; r < want; r++) rowText(m, t.solo, r).split("|").forEach((cell, c) => { if (/^[A-G][-#]\d/.test(cell)) noted.add(c); });
        if (v.pattern !== t.solo || v.row !== 0 || v.rows !== want || !want) wrong.push(name + ": the first frame is " + JSON.stringify(v));
        else if (!shown.length || shown.some((c) => !noted.has(c))) wrong.push(name + ": channels " + shown.join(",") + ", notes in " + [...noted].join(","));
        else if (!(t.seconds > 0)) wrong.push(name + ": " + t.seconds + " s");
        M._bm_free(m);
      }
      db.close();
    }
    check("unused patterns: each entry's first frame its own pattern, every channel shown with a note", patternEntries > 0 && wrong.length === 0, wrong);
  }
  console.log("musictest: " + checks + " checks, " + failed + " failed" + (mo3 ? "" : " (no MO3 staged: libxmp-lite only)") +
    (assets ? "; " + patternEntries + " unused patterns in " + patternGames + " games" : ""));
  process.exit(failed ? 1 : 0);
})().catch((e) => { console.log("musictest: " + (e && e.stack || e)); process.exit(1); });
