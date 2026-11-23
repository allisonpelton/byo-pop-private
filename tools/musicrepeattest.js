// Every tune of every staged game played with repeat on, unheard and fast, past its end (the user, 2026-11-22: "Repeat seems to
// break on some tunes, like BookWorm's game over 2, and in any case the position indicator doesn't loop back to the start").
// The tunes are web/music.js's own listing (tunesOf, over the staged files through tools/web/stageddb.js) and their sound its own
// sources (sounds), rendered at 8000 Hz. For each tune:
//  - it goes on: next() never ends, and there is sound within 20 seconds of where it went round;
//  - the position goes back: position() falls back once the tune has played its length (the length the list and the seek bar
//    show, trimmed where a long silent ending is cut), and never runs past it;
//  - from the right place: a module's sound after it went round is where its position says. The order and row it plays at a
//    position are those it played at that position the first time through (a second source of the tune, played once, within
//    WITHIN seconds), in an order that time through played. And a subsong goes round where the decoder itself goes on once it
//    has played through (goesOn), where that is a place its time through played with sound after it, not at its start (an
//    intro, then the loop); else at its start (a loop into another tune, or into a silent pattern, plays it again from its
//    start). A recorded tune goes round to its loop's start (web/soundtracks.js's loopFrom: the piece it names), a MIDI song
//    to its start.
// A recorded tune is not decoded here (the browser decodes it; stb_vorbis under Node would take minutes for them all): each piece
// stands in as sound of its own length (oggSeconds, wavSeconds), every frame telling its piece and place, so where it went round
// to is read from the sound itself.
// node tools/musicrepeattest.js <build/web/openmpt.js> <assets dir> [game ...]   (-v: every tune's line)
"use strict";
const path = require("path"), fs = require("fs");
const args = process.argv.slice(2), verbose = args.includes("-v");
const [jsPath, assets, ...only] = args.filter((a) => a !== "-v");
const RATE = 8000, CHUNK = 4096, LEVEL = 0.001, HEAR = 20, WITHIN = 0.15;
if (!jsPath || !assets) { console.log("usage: node tools/musicrepeattest.js <build/web/openmpt.js> <assets dir> [game ...] [-v]"); process.exit(2); }

(async () => {
  const started = Date.now();
  const ByopopOpenMPT = require(path.resolve(jsPath));
  const M = await ByopopOpenMPT({ locateFile: (f) => path.join(path.dirname(path.resolve(jsPath)), f) });
  const staged = require("./web/stageddb.js").install(assets);
  const src = fs.readFileSync(path.join(__dirname, "../web/music.js"), "utf8");
  const Music = require("vm").runInThisContext(src + "\n;ByopopMusic", { filename: "web/music.js" });
  const table = new Function(fs.readFileSync(path.join(__dirname, "../web/soundtracks.js"), "utf8") + "\nreturn ByopopSoundtracks;")();
  const T = Music.test;

  // a recorded piece's stand-in: left the piece's number (1, 2, ...) a hundredth each, right its frame's place in it, 0 to 1
  let pieces = [];
  function pieceFrames(bytes) {
    const ogg = bytes[0] === 0x4f && bytes[1] === 0x67 && bytes[2] === 0x67 && bytes[3] === 0x53;
    return Math.max(1, Math.round((ogg ? T.oggSeconds(bytes) : T.wavSeconds(bytes)) * RATE));
  }
  function standIn(bytes) {
    const n = pieceFrames(bytes);
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
  const loud = (c) => { for (let i = 0; i < c.length; i++) if (c[i] > LEVEL || c[i] < -LEVEL) return true; return false; };

  // where the decoder itself goes on once a subsong has played through, its own loop on: a copy of the module played across the
  // end its time through reached (end), the place where its time goes back or on past that end, and whether there is sound
  // from there within as long as the tune is, up to a minute (heard); null where it stops
  function goesOn(t, bytes, end) {
    const p = M._malloc(bytes.length);
    M.HEAPU8.set(bytes, p);
    const m = M._bm_load(p, bytes.length);
    M._free(p);
    if (!m) return null;
    const STEP = 128, out = M._malloc(STEP * 8), vb = M._malloc(20);
    try {
      M._bm_select(m, t.sub);
      M._bm_loop(m, 1);
      M._bm_render(m, RATE, 1, out);
      let was = M._bm_seek(m, RATE, Math.max(0, end - 0.5));
      for (let k = 0; k < RATE * 4 / STEP; k++) {
        if (M._bm_render(m, RATE, STEP, out) < STEP) return null;
        const now = M._bm_position(m);
        if (now >= end + 0.004 || now < was - 0.001) {
          M._bm_view(m, vb);
          const on = { order: M.HEAP32[(vb >> 2) + 4], row: M.HEAP32[(vb >> 2) + 1], heard: false };
          for (let left = Math.min(end, 60) * RATE; left > 0 && !on.heard; left -= STEP) {
            const g = M._bm_render(m, RATE, STEP, out);
            on.heard = loud(M.HEAPF32.subarray(out >> 2, (out >> 2) + g * 2));
            if (g < STEP) break;
          }
          return on;
        }
        was = now;
      }
      return null;
    } finally { M._free(out); M._free(vb); M._bm_free(m); }
  }

  // one tune: a list of what is wrong with it (empty for none), and what it did
  async function tryTune(t, files) {
    const sound = T.sounds(M, RATE, how), bad = [];
    pieces = files.slice();
    let s;
    try { s = await Promise.resolve().then(() => sound.sourceOf(t, files)); } catch (e) { sound.free(); return { skip: String(e && e.message || e) }; }
    const length = t.seconds, isModule = t.kind === "module";
    const kind = t.kind === "module" ? (t.range ? "range" : t.segs ? "segments" : "subsong") : t.kind;
    // a module's first time through: a second source played once from its start, repeat off; where it was when (its marks' times
    // its own positions), the orders it played, and where and after how many frames it ended
    const first = { marks: [], orders: new Set(), end: null, frames: 0 };
    if (isModule) {
      let s2 = null;
      try { s2 = await Promise.resolve().then(() => sound.sourceOf(t, files)); } catch (e) { bad.push("a second source: " + e); }
      if (s2) {
        // (its end: the furthest its frames took it, as libopenmpt renders on a little past the end, its time already back at
        // the loop's place)
        let furthest = 0, was = 0, ended = false;
        for (let k = 0; k < (length * 1.05 + 6) * RATE / CHUNK; k++) {
          const at = s2.position(), m2 = [], c = s2.next(CHUNK, m2);
          if (!c) { ended = true; break; }
          if (at >= was - 0.05) furthest = Math.max(furthest, at + c.length / 2 / RATE);
          was = at;
          for (const mk of m2) { first.marks.push([at + mk[0] / RATE, mk[5], mk[2]]); first.orders.add(mk[5]); }
          first.frames += c.length / 2;
          if (c.length / 2 < CHUNK) { ended = true; break; }
        }
        if (ended) first.end = furthest;
        s2.free();
      }
    }
    s.repeat(true);
    let frames = 0, fall = null, heardAt = -1, after = null, most = 0, ended = false, lastRight = Infinity;   // (lastRight: the stand-in's place at the chunk's end)
    const cap = Math.ceil((length * 1.05 + 6) * RATE);
    for (;;) {
      const before = s.position(), marks = isModule ? [] : null, c = s.next(CHUNK, marks);
      if (!c) { ended = true; break; }
      const got = c.length / 2, now = s.position(), lastBefore = lastRight;
      lastRight = c[c.length - 1];
      most = Math.max(most, before);
      if (!fall && now < before + got / RATE - 0.05) {
        fall = { frames: frames, before: before, now: now, got: got };
        // a recorded tune: where in the sound it went round to (the first frame of a piece's start after the one it ended on)
        if (!isModule && t.kind !== "midi") {
          for (let i = 0, was = lastBefore; i < got; was = c[i * 2 + 1], i++) if (c[i * 2 + 1] < was) { fall.piece = Math.round(c[i * 2] * 100) - 1; fall.place = c[i * 2 + 1]; fall.into = got - i; break; }
        }
      } else if (fall && !after && isModule && frames - fall.frames >= 2 * CHUNK) after = { at: before, mark: marks[0] };
      // (the chunk after the one that went round is passed over: libxmp-lite's view of a subsong chosen again tells its new place
      // only once a frame has played)
      if (fall && fall.frames !== frames && heardAt < 0 && loud(c)) heardAt = frames;
      frames += got;
      if (!fall && frames > cap) break;
      if (fall && (after || !isModule) && (heardAt >= 0 || frames - fall.frames > HEAR * RATE)) break;
    }
    // where it should go round: where its first time through ended (a module's: libopenmpt's length is a little off for a few
    // subsongs), else at its length
    const end = first.end !== null ? first.end : length, endFrames = first.end !== null ? first.frames : length * RATE;
    const playedTo = fall ? fall.before + fall.got / RATE : most, res = { kind: kind, length: length, to: fall ? fall.now - fall.got / RATE : null };
    if (first.end !== null && Math.abs(first.end - length) > 0.6) res.note = "it plays " + first.end.toFixed(2) + " s through, its length shown " + length.toFixed(2) + " s";
    if (ended) bad.push("it ended at " + (frames / RATE).toFixed(2) + " s with repeat on");
    else if (!fall) bad.push("the position did not go back: " + most.toFixed(2) + " s after " + (frames / RATE).toFixed(2) + " s played (it ends at " + end.toFixed(2) + " s)");
    else {
      if (playedTo < end - 0.6) bad.push("went back early, at " + playedTo.toFixed(2) + " s of " + end.toFixed(2));
      if (fall.frames > endFrames + (0.6 + end * 0.02) * RATE) bad.push("went back late, after " + (fall.frames / RATE).toFixed(2) + " s played (it ends after " + (endFrames / RATE).toFixed(2) + " s)");
      if (heardAt < 0) bad.push("silent for " + HEAR + " s after it went round, to " + fall.now.toFixed(2) + " s");
    }
    if (most > Math.max(length, end) + 0.6) bad.push("the position ran past its length: " + most.toFixed(2) + " s of " + length.toFixed(2));
    if (fall && isModule && after && after.mark) {
      // where it is: an order its first time through played, at the same row within WITHIN seconds of the same time
      const order = after.mark[5], row = after.mark[2];
      res.to = after.at;
      res.order = order;
      if (!first.orders.has(order)) bad.push("went round into order " + order + ", which its first time through did not play");
      else {
        let best = null, near = null;
        for (const mk of first.marks) {
          const d = Math.abs(mk[0] - after.at);
          if (mk[1] === order && Math.abs(mk[2] - row) <= 1 && (!best || d < best.d)) best = { d: d, when: mk[0] };
          if (!near || d < near.d) near = { d: d, order: mk[1], row: mk[2] };
        }
        if (!best || best.d > WITHIN) bad.push("at " + after.at.toFixed(2) + " s it plays order " + order + " row " + row + (best ? ", which it played first at " + best.when.toFixed(2) + " s" : "") +
          "; first time through, " + after.at.toFixed(2) + " s was order " + (near ? near.order + " row " + near.row : "none"));
      }
      // where it went round to: where the decoder goes on, where its time through played that place before its last half second
      // and the decoder has sound from there, else its start ((fall.now - fall.got / RATE, fall.now]: the position it went back
      // to, within the chunk it went back in)
      if (kind === "subsong" && first.end !== null) {
        const on = goesOn(t, files[0], first.end), firstAt = on && first.marks.find((mk) => mk[1] === on.order && mk[2] >= on.row);
        let want = 0;
        if (firstAt && firstAt[0] < first.end - 0.5 && on.heard) want = firstAt[0];
        if (want < fall.now - fall.got / RATE - WITHIN || want > fall.now + WITHIN)
          bad.push("went round to " + (fall.now - fall.got / RATE).toFixed(2) + "-" + fall.now.toFixed(2) + " s, not " + want.toFixed(2) + " s" +
            (on ? " (the decoder goes on at order " + on.order + " row " + on.row + (firstAt ? ", first played at " + firstAt[0].toFixed(2) + " s" : ", not played first time through") + ")" : ""));
        res.wanted = want;
      }
    } else if (fall && t.kind === "midi") {
      if (fall.now > fall.got / RATE + 0.6) bad.push("went round to " + (fall.now - fall.got / RATE).toFixed(2) + " s or later, not its start");
    } else if (fall && !isModule) {
      // the piece that loopFrom names, from its start; the position there
      const want = t.loopFrom || 0, wantPiece = files.indexOf(files[want]);
      let wantAt = 0;
      for (let i = 0; i < want; i++) wantAt += pieceFrames(files[i]);
      if (fall.piece === undefined) bad.push("went round, but not to a piece's start");
      else if (fall.piece !== wantPiece || fall.place > 1e-6) bad.push("went round to piece " + (fall.piece + 1) + " at " + fall.place.toFixed(3) + ", not piece " + (wantPiece + 1) + "'s start");
      const shown = fall.now - fall.into / RATE;
      if (Math.abs(shown - wantAt / RATE) > 0.01) bad.push("its position went back to " + shown.toFixed(2) + " s, its loop's start being " + (wantAt / RATE).toFixed(2) + " s");
      res.to = shown;
    }
    s.free();
    sound.free();
    res.bad = bad;
    return res;
  }

  let tunes = 0, games = 0, failed = 0, skipped = 0;
  const kinds = {}, wrong = [], skips = [], notes = [];
  for (const id of Object.keys(table)) {
    if (only.length && !only.includes(id)) continue;
    if (!staged(id)) continue;
    const game = { id: id, title: id, persist: "staged:" + id };
    const list = (await T.tunesOf(M, game, table[id])).filter((t) => t.solo === undefined);
    if (!list.length) continue;
    games++;
    const db = await Music.files.open(game.persist);
    for (const t of list) {
      const name = id + " " + t.id;
      const got = {};   // (a piece played twice is one file, as the player reads it)
      for (const k of t.keys) if (!(k in got)) got[k] = await Music.files.read(db, k);
      const files = t.keys.map((k) => got[k]);
      if (files.some((f) => !f)) { skipped++; skips.push(name + ": a file missing"); continue; }
      const r = await tryTune(t, files);
      if (r.skip) { skipped++; skips.push(name + ": " + r.skip); continue; }
      tunes++;
      kinds[r.kind] = (kinds[r.kind] || 0) + 1;
      const line = name + " (" + r.kind + ", " + r.length.toFixed(2) + " s" + (r.to !== null ? ", after going round " + r.to.toFixed(2) + " s" : "") +
        (r.order !== undefined ? " order " + r.order : "") + ")";
      if (r.note) notes.push(name + ": " + r.note);
      if (r.bad.length) { failed++; wrong.push(line + ": " + r.bad.join("; ")); }
      else if (verbose) console.log("ok   " + line);
    }
    db.close();
  }
  for (const w of wrong) console.log("FAIL " + w);
  for (const w of skips) console.log("skip " + w);
  for (const w of notes) console.log("note " + w);
  console.log("musicrepeattest: " + tunes + " tunes in " + games + " games (" + Object.keys(kinds).sort().map((k) => kinds[k] + " " + k).join(", ") +
    "), " + failed + " failed" + (skipped ? ", " + skipped + " skipped" : "") + "; " + ((Date.now() - started) / 1000).toFixed(0) + " s");
  process.exit(failed || !tunes ? 1 : 0);
})().catch((e) => { console.log("musicrepeattest: " + (e && e.stack || e)); process.exit(1); });
