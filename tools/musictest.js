// The music player's decoder (web/music/openmpt_web.c, build/web/openmpt.js) under Node: the tracker view's position and rows and
// the seek bar's seek, on a small ProTracker module made here (libxmp-lite's side) and, when the main checkout's staged files are
// there, a game's MO3 (libopenmpt's side). node tools/musictest.js <build/web/openmpt.js> [assets dir]
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
function protracker() {
  const pat = 64 * 4 * 4, sampleWords = 32;
  const b = Buffer.alloc(1084 + 2 * pat + sampleWords * 2);
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
  const data = 1084 + 2 * pat;
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
  M._bm_free(m);

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
    const len = M._bm_duration(t), half = Math.min(len / 2, 30);
    at = M._bm_seek(t, RATE, half);
    check("mo3: seeks", Math.abs(at - half) < 0.5 && Math.abs(M._bm_position(t) - half) < 0.5, [at, half]);
    M._bm_free(t);
  }
  console.log("musictest: " + checks + " checks, " + failed + " failed" + (mo3 ? "" : " (no MO3 staged: libxmp-lite only)"));
  process.exit(failed ? 1 : 0);
})().catch((e) => { console.log("musictest: " + (e && e.stack || e)); process.exit(1); });
