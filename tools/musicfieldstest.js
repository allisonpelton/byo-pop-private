// The end fields of web/soundtracks.js (loops, once, next, stings, gameOver and series: the user, 2026-11-22) and the Music
// player's Rip Style over them (web/music.js's ripStyle), through web/music.js's own listing of the staged files (tunesOf,
// tools/web/stageddb.js). For every game in the table:
//  - every tune the list shows (not unused, not a pattern played alone, not a series) is in exactly one of loops, once and
//    next, and every id the fields name is one of the game's tunes (a looped tune has its place, a lead-in its target);
//  - a next leads to another tune; a gameOver's tune is one the game loops, and leads to another tune by cut, fade or cross, a
//    fade's and a cross's time given;
//  - a series has two members or more, each a tune, and the list has its entry (series:<name>), one tune of all their segments
//    where they are pieces of one module;
//  - a title the table reads from one of the game's scripts (titles' luc) is found there.
// A game not staged has the table's own checks only. Then Rip Style, played unheard at 8000 Hz through the player's own sources
// (sounds) and ripStyle:
//  - the shortest tune the table loops of each kind (a module's subsong, a range of its orders, a piece cut from one, a decoded
//    tune, a joined one that goes round to a later piece, a MIDI song, a series of one module's pieces): it goes round
//    RIP_PASSES times, then fades out over RIP_FADE seconds from the start of the next pass, with sound until then and none at
//    the fade's end, and ends; with Repeat on it goes on past that. The shortest tune the table plays once ends after one pass.
//  - a Game Over Ending each way (cut, fade, cross) between two stand-in tunes: the game-over tune asked for during the last
//    pass, and handed over at the next pass's start, at once (cut) or after the game's time, the first faded out (fade) or the
//    two crossed (cross).
// A decoded piece is a stand-in of its own length (tools/musicrepeattest.js's), so its sound and its place are known.
// node tools/musicfieldstest.js <build/web/openmpt.js> <assets dir> [-v]
"use strict";
const path = require("path"), fs = require("fs");
const args = process.argv.slice(2), verbose = args.includes("-v");
const [jsPath, assets] = args.filter((a) => a !== "-v");
const RATE = 8000, CHUNK = 4096, LEVEL = 0.001;
if (!jsPath || !assets) { console.log("usage: node tools/musicfieldstest.js <build/web/openmpt.js> <assets dir> [-v]"); process.exit(2); }

(async () => {
  const started = Date.now();
  const ByopopOpenMPT = require(path.resolve(jsPath));
  const M = await ByopopOpenMPT({ locateFile: (f) => path.join(path.dirname(path.resolve(jsPath)), f) });
  const staged = require("./web/stageddb.js").install(assets);
  const src = fs.readFileSync(path.join(__dirname, "../web/music.js"), "utf8");
  const Music = require("vm").runInThisContext(src + "\n;ByopopMusic", { filename: "web/music.js" });
  const table = new Function(fs.readFileSync(path.join(__dirname, "../web/soundtracks.js"), "utf8") + "\nreturn ByopopSoundtracks;")();
  const T = Music.test, PASSES = T.RIP_PASSES, FADE = T.RIP_FADE;
  let passed = 0, failed = 0;
  const lines = [];
  function check(ok, what) {
    if (ok) { passed++; if (verbose) lines.push("ok   " + what); }
    else { failed++; lines.push("FAIL " + what); }
  }

  // a decoded piece's stand-in: left the piece's number a hundredth each, right its frame's place in it
  const pieces = [];
  function standIn(bytes) {
    const ogg = bytes[0] === 0x4f && bytes[1] === 0x67 && bytes[2] === 0x67 && bytes[3] === 0x53;
    const n = Math.max(1, Math.round((ogg ? T.oggSeconds(bytes) : T.wavSeconds(bytes)) * RATE));
    let k = pieces.indexOf(bytes);
    if (k < 0) { pieces.push(bytes); k = pieces.length - 1; }
    const out = new Float32Array(n * 2);
    for (let i = 0; i < n; i++) { out[i * 2] = (k + 1) / 100; out[i * 2 + 1] = i / n; }
    return Promise.resolve(out);
  }
  const how = {
    decode: standIn,
    bank: (p) => Promise.resolve(new Uint8Array(fs.readFileSync(path.join(path.dirname(path.resolve(jsPath)), p)))),
    trimmed: () => {}
  };

  // ---- the fields, game by game ----
  const low = (x) => String(x).toLowerCase();
  const samples = {};   // kind: {t, files}, the shortest looped tune of each kind; "once": the shortest played once
  function kindOf(t) {
    if (t.series) return t.kind === "module" ? "series of pieces" : null;
    if (t.kind === "midi") return "midi";
    if (t.kind === "module") return t.range ? "range" : t.segs ? "piece" : "subsong";
    return t.keys.length > 1 && t.loopFrom ? "joined" : t.keys.length === 1 ? "decoded" : null;
  }
  let games = 0, tunesChecked = 0;
  for (const id of Object.keys(table)) {
    const e = table[id], name = id;
    const named = [].concat(e.loops || [], e.once || [], Object.keys(e.next || {}), Object.values(e.next || {}), e.stings || [],
      Object.keys(e.gameOver || {}), Object.values(e.gameOver || {}).map((o) => o.to), ...Object.values(e.series || {}));
    for (const [from, o] of Object.entries(e.gameOver || {})) {
      check(["cut", "fade", "cross"].includes(o.how) && (o.how === "cut" || o.ms > 0), name + " gameOver " + from + ": how " + o.how + (o.ms ? ", " + o.ms + " ms" : ""));
      check((e.loops || []).some((x) => low(x) === low(from)), name + " gameOver " + from + ": a tune the game loops");
      check(low(o.to) !== low(from), name + " gameOver " + from + ": to another tune");
    }
    for (const [from, to] of Object.entries(e.next || {})) check(low(from) !== low(to), name + " next " + from + ": to another tune");
    for (const [sname, ids] of Object.entries(e.series || {})) check(ids.length >= 2, name + " series " + sname + ": two members or more");
    if (!staged(id)) { lines.push("note " + name + ": not staged, its table checked only"); continue; }
    games++;
    const game = { id: id, title: id, persist: "staged:" + id };
    const list = await T.tunesOf(M, game, e);
    const tunes = list.filter((t) => t.solo === undefined && !t.series);
    const has = (x) => tunes.some((t) => low(t.id) === low(x));
    for (const x of named) check(has(x), name + " " + x + ": one of the game's tunes");
    for (const t of tunes) {
      if (t.unused) continue;
      tunesChecked++;
      const n = ["loops", "once"].filter((f) => (e[f] || []).some((x) => low(x) === low(t.id))).length +
        Object.keys(e.next || {}).filter((x) => low(x) === low(t.id)).length;
      check(n === 1, name + " " + t.id + " (" + (t.where || "") + "): in exactly one of loops, once and next (" + n + ")");
    }
    for (const [sname, ids] of Object.entries(e.series || {})) {
      const row = list.find((t) => t.id === "series:" + sname), members = ids.map((x) => tunes.find((t) => low(t.id) === low(x)));
      check(!!row, name + " series " + sname + ": an entry in the list");
      if (!row || members.some((m) => !m)) continue;
      const pieces = members.every((m) => m.kind === "module" && m.segs && m.keys[0] === members[0].keys[0]);
      check(pieces ? row.kind === "module" && row.segs.length === members.reduce((k, m) => k + m.segs.length, 0) : row.kind === "series" && row.members.length === members.length,
        name + " series " + sname + ": " + (pieces ? "one tune of its " + members.length + " pieces' segments" : members.length + " tunes played in turn"));
    }
    // a title read from one of the game's scripts (titles' luc) is there (not printed: it is the game's own text)
    const lucs = Object.entries(e.titles || {}).filter(([, t]) => t && t.luc);
    if (lucs.length) {
      const db = await Music.files.open(game.persist), read = {};
      try {
        for (const [tid, t] of lucs) {
          if (!(t.luc in read)) { const b = await Music.files.read(db, game.persist + "/game/" + t.luc); read[t.luc] = b ? T.lucConstants(b) : null; }
          check(!!T.lucTitle(read[t.luc], t), name + " " + tid + ": its title in " + t.luc);
        }
      } finally { db.close(); }
    }
    // the samples for Rip Style
    for (const t of list) {
      if (t.unused || t.solo !== undefined) continue;
      const k = t.ends === "once" ? (t.kind === "module" && !t.range && !t.segs ? "once" : null) : (t.ends === "loop" || t.ends === "series") ? kindOf(t) : null;
      if (!k || t.seconds < 3) continue;
      if (!samples[k] || t.seconds < samples[k].t.seconds) samples[k] = { t: t, game: game };
    }
  }

  // ---- Rip Style played ----
  async function filesOf(s) {
    const db = await Music.files.open(s.game.persist), got = {};
    try { for (const k of s.t.keys) if (!(k in got)) got[k] = await Music.files.read(db, k); } finally { db.close(); }
    return s.t.keys.map((k) => got[k]);
  }
  const on = { rip: () => true, over: () => false };
  // plays a source through ripStyle until it ends or limit seconds: where its position went back before its fade (a short tune
  // goes round on during the fade, as a rip's does), its frames, its loudest, the loudest of its last 256 frames, whether it ended
  function ripPlay(source, looped, repeat, limit) {
    const r = T.ripStyle(source, RATE, looped, null, false, null, on);
    r.repeat(repeat);
    const out = { backs: [], frames: 0, peak: 0, tail: 0, ended: false, ripEnd: null };
    while (out.frames < limit * RATE) {
      const before = r.position(), fading = r.ripEnd !== null, c = r.next(CHUNK, null);
      if (!c) { out.ended = true; break; }
      if (!fading && r.position() < before - 0.05) out.backs.push(out.frames);
      const n = c.length >> 1;
      let tail = 0;
      for (let i = 0; i < n; i++) {
        const a = Math.max(Math.abs(c[2 * i]), Math.abs(c[2 * i + 1]));
        if (a > out.peak) out.peak = a;
        if (i >= n - 256 && a > tail) tail = a;
      }
      out.tail = tail;
      out.frames += n;
    }
    out.ripEnd = r.ripEnd;
    r.free();
    return out;
  }
  const kinds = Object.keys(samples).sort();
  for (const k of ["subsong", "range", "piece", "decoded", "joined", "midi", "series of pieces", "once"])
    if (!samples[k]) lines.push("note no staged tune for Rip Style's " + k);
  const fadeLen = Math.round(FADE * RATE);
  for (const k of kinds) {
    const s = samples[k], t = s.t, files = await filesOf(s), label = "Rip Style, " + k + ": " + s.game.id + " " + t.id + " (" + t.seconds.toFixed(1) + " s)";
    if (files.some((f) => !f)) { lines.push("note " + label + ": a file missing"); continue; }
    const sound = T.sounds(M, RATE, how), limit = t.seconds * (PASSES + 2) + FADE + 30;
    const looped = k !== "once";
    const a = ripPlay(await sound.sourceOf(t, files), looped, false, limit);
    if (!looped) {
      // (a module's decoder may run a little past the tune's end into its loop before it ends: its time then goes back)
      check(a.ended && a.frames <= (t.seconds + 1) * RATE && a.ripEnd === null, label + ": ends after one pass (" + (a.frames / RATE).toFixed(1) + " s)");
    } else {
      const after = a.backs.length >= PASSES ? a.frames - a.backs[PASSES - 1] : -1;
      check(a.ended, label + ": ends (" + (a.frames / RATE).toFixed(1) + " s)");
      check(a.backs.length === PASSES, label + ": goes round " + PASSES + " times (" + a.backs.length + ")");
      check(after >= fadeLen && after < fadeLen + CHUNK, label + ": fades over " + FADE + " s from the next pass's start (" + (after / RATE).toFixed(2) + " s)");
      check(a.peak > LEVEL && a.tail <= Math.max(LEVEL, a.peak * 0.01), label + ": sound, and none at the fade's end (" + a.peak.toFixed(3) + ", " + a.tail.toFixed(4) + ")");
      check(a.ripEnd === "fade", label + ": ended by its fade");
      const b = ripPlay(await sound.sourceOf(t, files), looped, true, t.seconds * (PASSES + 1) + FADE + 5);
      check(!b.ended && b.backs.length >= PASSES, label + ": with Repeat on it goes on (" + b.backs.length + " times round)");
    }
    sound.free();
  }

  // ---- Game Over Ending, each way, between stand-ins ----
  // a stand-in tune: every frame `level`, its position going back to 0 every `loop` frames (null: plays on)
  function standTune(level, loop) {
    let at = 0, done = 0, again = false;
    return {
      repeat: (o) => { again = o; },
      next: (frames) => {
        if (loop && at >= loop) { if (!again) return null; at = 0; }
        const n = loop ? Math.min(frames, loop - at) : frames, c = new Float32Array(n * 2).fill(level);
        at += n; done += n;
        return c;
      },
      position: () => at / RATE,
      seek: (s) => { at = Math.round(s * RATE); },
      free: () => {},
      played: () => done
    };
  }
  for (const way of ["cut", "fade", "cross"]) {
    const loop = 3 * CHUNK, ms = 500, len = Math.round(ms / 1000 * RATE), first = standTune(0.5, loop), go = standTune(0.25, null);
    let wanted = 0, taken = 0;
    const ahead = { want: () => { wanted++; }, take: () => { taken++; return go; }, free: () => {} };
    const r = T.ripStyle(first, RATE, true, { to: "x", how: way, ms: way === "cut" ? 0 : ms }, false, ahead, { rip: () => true, over: () => true });
    r.repeat(false);
    let frames = 0, wantedAt = -1, overAt = -1, handAt = -1, firstAfter = null, lastBefore = null;
    for (let k = 0; k < 40 && handAt < 0; k++) {
      const c = r.next(CHUNK, null);
      if (!c) break;
      if (wanted && wantedAt < 0) wantedAt = frames;
      if (r.ripEnd === "over" && overAt < 0) { overAt = frames; firstAfter = c[0]; }
      if (r.handover) { handAt = frames + (c.length >> 1); lastBefore = c[c.length - 2]; }
      frames += c.length >> 1;
    }
    const label = "Game Over Ending, " + way;
    check(wantedAt === loop * (PASSES - 1), label + ": the game-over tune asked for at the last pass's start (" + wantedAt + " frames)");
    check(overAt === loop * PASSES && taken === 1, label + ": the ending at the next pass's start (" + overAt + " frames)");
    check(r.handover === go, label + ": handed over to the game-over tune");
    if (way === "cut") check(handAt - overAt === CHUNK && firstAfter === 0.25 && go.played() === CHUNK, label + ": at once, the game-over tune from its start");
    else {
      check(handAt - overAt >= len && handAt - overAt < len + CHUNK, label + ": after " + ms + " ms (" + ((handAt - overAt) / RATE * 1000).toFixed(0) + " ms)");
      if (way === "fade") check(Math.abs(firstAfter - 0.5) < 0.01 && Math.abs(lastBefore) < 0.01 && go.played() === 0, label + ": the first faded out, the other not yet begun");
      else check(Math.abs(firstAfter - 0.5) < 0.01 && Math.abs(lastBefore - 0.25) < 0.01 && go.played() === handAt - overAt, label + ": the two crossed, the other heard rising under the first");
    }
  }

  for (const l of lines) console.log(l);
  console.log("musicfieldstest: " + passed + " passed, " + failed + " failed (" + tunesChecked + " tunes in " + games + " games; Rip Style on " +
    kinds.length + " kinds: " + kinds.join(", ") + ") in " + ((Date.now() - started) / 1000).toFixed(0) + " s");
  process.exit(failed || !games ? 1 : 0);
})().catch((e) => { console.log("musicfieldstest: " + (e && e.stack || e)); process.exit(1); });
