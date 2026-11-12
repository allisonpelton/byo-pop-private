// tools/web/seamrender.js: the seconds either side of a seam in a module tune, rendered by the site's own player (build/web/
// openmpt.js, web/music/openmpt_web.c), as 16-bit WAV files for tools/web/soundcompare.py's Seams list. Development only.
//
//   node tools/web/seamrender.js <openmpt.js> <jobs.json>
//
// Each job is {out, file, kind, ...} with `around` seconds kept either side (4 by default):
//   cut    {sub, order}  subsong `sub` played from its start; the seam is where order `order` first plays (web/soundtracks.js,
//          "cuts": the game starts a different tune there)
//   chain  {a, b}        the end of subsong `a` played to its end, then the start of subsong `b` ("chains": one track)
// Prints one line a job: "<out> <seam seconds> ok", or "<out> error <why>".
"use strict";
const fs = require("fs");
const path = require("path");

const RATE = 44100, CHUNK = 1024;

function wav(file, frames) {
  // frames: Float32Array, interleaved stereo
  const n = frames.length, buf = Buffer.alloc(44 + n * 2);
  buf.write("RIFF", 0); buf.writeUInt32LE(36 + n * 2, 4); buf.write("WAVE", 8); buf.write("fmt ", 12);
  buf.writeUInt32LE(16, 16); buf.writeUInt16LE(1, 20); buf.writeUInt16LE(2, 22); buf.writeUInt32LE(RATE, 24);
  buf.writeUInt32LE(RATE * 4, 28); buf.writeUInt16LE(4, 32); buf.writeUInt16LE(16, 34); buf.write("data", 36);
  buf.writeUInt32LE(n * 2, 40);
  for (let i = 0; i < n; i++) buf.writeInt16LE(Math.max(-32768, Math.min(32767, Math.round(frames[i] * 32767))), 44 + i * 2);
  fs.writeFileSync(file, buf);
}

function concat(parts) {
  let n = 0;
  parts.forEach(function (p) { n += p.length; });
  const out = new Float32Array(n);
  let at = 0;
  parts.forEach(function (p) { out.set(p, at); at += p.length; });
  return out;
}

async function main() {
  const factory = require(path.resolve(process.argv[2]));
  const M = await (typeof factory === "function" ? factory : globalThis.ByopopOpenMPT)({
    locateFile: function (f) { return path.join(path.dirname(path.resolve(process.argv[2])), f); } });
  const jobs = JSON.parse(fs.readFileSync(process.argv[3], "utf8"));
  const out = M._malloc(CHUNK * 8);

  function load(file) {
    const data = fs.readFileSync(file), p = M._malloc(data.length);
    M.HEAPU8.set(data, p);
    const m = M._bm_load(p, data.length);
    M._free(p);
    return m;
  }
  // renders a subsong from its start (no repeat); calls each(chunk, orderBefore) until it returns false or the tune ends
  function play(m, sub, each) {
    M._bm_select(m, sub);
    M._bm_loop(m, 0);
    for (;;) {
      const order = M._bm_order(m);
      const got = M._bm_render(m, RATE, CHUNK, out);
      if (got <= 0) return;
      const chunk = M.HEAPF32.slice(out >> 2, (out >> 2) + got * 2);
      if (each(chunk, order) === false) return;
    }
  }

  for (const job of jobs) {
    const around = (job.around || 4) * RATE * 2;
    const m = load(job.file);
    if (!m) { console.log(job.out + " error not loaded"); continue; }
    try {
      if (job.kind === "cut") {
        let before = [], kept = 0, after = null, afterLen = 0, at = 0, seam = -1;
        play(m, job.sub, function (chunk) {
          if (after === null && M._bm_order(m) >= job.order) { after = []; seam = at; }
          at += chunk.length / 2;
          if (after === null) {
            before.push(chunk); kept += chunk.length;
            while (kept - before[0].length >= around) { kept -= before[0].length; before.shift(); }
            return true;
          }
          after.push(chunk); afterLen += chunk.length;
          return afterLen < around;
        });
        if (seam < 0) { console.log(job.out + " error order " + job.order + " never reached"); continue; }
        let pre = concat(before);
        pre = pre.subarray(Math.max(0, pre.length - around));
        wav(job.out, concat([pre, concat(after).subarray(0, around)]));
        console.log(job.out + " " + (seam / RATE).toFixed(2) + " " + (pre.length / 2 / RATE).toFixed(2) + " ok");
      } else if (job.kind === "chain") {
        let before = [], kept = 0, at = 0;
        play(m, job.a, function (chunk) {
          at += chunk.length / 2;
          before.push(chunk); kept += chunk.length;
          while (kept - before[0].length >= around) { kept -= before[0].length; before.shift(); }
          return true;
        });
        let pre = concat(before);
        pre = pre.subarray(Math.max(0, pre.length - around));
        const after = [];
        let afterLen = 0;
        play(m, job.b, function (chunk) { after.push(chunk); afterLen += chunk.length; return afterLen < around; });
        wav(job.out, concat([pre, concat(after).subarray(0, around)]));
        console.log(job.out + " " + (at / RATE).toFixed(2) + " " + (pre.length / 2 / RATE).toFixed(2) + " ok");
      } else console.log(job.out + " error unknown kind " + job.kind);
    } finally {
      M._bm_free(m);
    }
  }
}

main().catch(function (e) { console.error(e); process.exit(1); });
