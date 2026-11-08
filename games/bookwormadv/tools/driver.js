// Bookworm Adventures' driver for the web build: plays with real DOM events on the canvas, reading what to do from
// the port's -boardlog (GameApp::LogBoardForTests, TestHooks.cpp). Pasted into the page (the Browser pane's
// javascript_tool, or fetched and eval'd), it defines window.bwa:
//
//   bwa.setup()        before Play: the requestAnimationFrame shim (a hidden pane runs none) and -boardlog=/tmp/board.log
//                      on the game's command line (the page passes it through Module.callMain)
//   bwa.click(x, y)    a move, then a press and a release at game pixel (x, y) (SDL puts a button at the last move)
//   bwa.key(name)      keydown, keypress, keyup on the canvas ("Enter", "a", ...)
//   bwa.lines()        the log's lines, parsed: {kind: "board"|"dialog"|"screen"|"talk", n, ...}
//   bwa.screen()       the screen the log last named (title, menu, map, battle, minigame, treasure, ...)
//   await bwa.waitScreen(names, seconds)
//                      until the screen is one of names (an array); clicks conversations on meanwhile
//   await bwa.settle(seconds)
//                      clicks a conversation on and answers dialogs until neither has shown for 2 s
//   await bwa.minigame(seconds)
//                      the same for a mini-game, each time with the longest word not yet used in it
//   await bwa.battle(seconds)
//                      one whole battle: from the map it clicks Enter, then spells each board's word (the longest the
//                      game's own word list allows), scrambles a board without one, answers dialogs with their first
//                      button and clicks conversations on, until the screen is no longer the battle. Returns a summary:
//                      {result: the screen after, words: [...], scrambles, dialogs, seconds}
//
// The native driver (realrun.py) does the same with posted click bursts. Volume 2 shares it
// (games/bookwormadv2/tools/). Game pixels are 800x600; the canvas may be any size.
(function () {
  const W = 800, H = 600, LOG = '/tmp/board.log';
  const sleep = ms => new Promise(r => setTimeout(r, ms));
  const canvas = () => document.getElementById('canvas');
  function at(x, y) { const r = canvas().getBoundingClientRect(); return [r.left + x * r.width / W, r.top + y * r.height / H]; }
  function mouse(type, x, y, buttons) {
    const [cx, cy] = at(x, y);
    canvas().dispatchEvent(new MouseEvent(type, {clientX: cx, clientY: cy, button: 0, buttons, bubbles: true, cancelable: true, view: window}));
  }
  async function click(x, y) {
    mouse('mousemove', x, y, 0); await sleep(60);
    mouse('mousedown', x, y, 1); await sleep(60);
    mouse('mouseup', x, y, 0); await sleep(60);
  }
  async function key(name) {
    const code = name.length === 1 ? 'Key' + name.toUpperCase() : name;
    const kc = name.length === 1 ? name.toUpperCase().charCodeAt(0) : ({Enter: 13, Escape: 27, Backspace: 8, Space: 32})[name] || 0;
    for (const type of ['keydown', 'keypress', 'keyup']) {
      canvas().dispatchEvent(new KeyboardEvent(type, {key: name, code, keyCode: kc, which: kc,
        charCode: type === 'keypress' && name.length === 1 ? name.charCodeAt(0) : 0, bubbles: true, cancelable: true}));
      await sleep(30);
    }
  }
  const pt = t => t && t.includes(',') ? t.split(',').map(Number) : null;
  function lines() {
    let text = '';
    try { text = Module.FS.readFile(LOG, {encoding: 'utf8'}); } catch (e) { return []; }
    return text.trim().split('\n').filter(Boolean).map(l => {
      const f = l.split(/\s+/), kind = f[0].replace(':', ''), n = +f[1];
      if (kind === 'screen') return {kind, n, name: f[2]};
      if (kind === 'wordmaster') {
        const k = f.includes('submit') ? f.indexOf('submit') : f.indexOf('rows');
        return {kind, n, word: f[2], tiles: f.slice(3, k).map(pt), attack: f[k] === 'submit' ? pt(f[k + 1]) : null, scramble: null};
      }
      if (kind === 'talk') return {kind, n, on: f[2] === '1'};
      if (kind === 'dialog') return {kind, n, id: +f[2], ok: pt(f[4])};
      const k = f.indexOf('attack');
      const w = f.indexOf('words'), le = f.indexOf('letters');
      return {kind, n, word: f[2], tiles: f.slice(3, k).map(pt), attack: pt(f[k + 1]), scramble: pt(f[k + 3]),
        words: w > 0 && f[w + 1] !== '-' ? f[w + 1].split(',') : [],
        letters: le > 0 && f[le + 1] !== '-' ? f[le + 1].split(';').map(e => [e.split('@')[0], pt(e.split('@')[1])]) : []};
    });
  }
  function screen() { const s = lines().filter(l => l.kind === 'screen'); return s.length ? s[s.length - 1].name : 'none'; }
  async function waitScreen(names, seconds) {
    const end = Date.now() + seconds * 1000;
    while (Date.now() < end) {
      if (names.includes(screen())) return true;
      await sleep(500);
    }
    return false;
  }
  // the tiles that spell word from letters [[letter(s), [x, y]]], the first that fits at each place; null if none
  function spell(word, letters) {
    const used = new Set(), order = [];
    for (let i = 0; i < word.length;) {
      const t = letters.findIndex(([l], j) => !used.has(j) && word.startsWith(l, i));
      if (t < 0) return null;
      used.add(t); order.push(letters[t][1]); i += letters[t][0].length;
    }
    return order;
  }
  function talking() { const t = lines().filter(l => l.kind === 'talk'); return t.length > 0 && t[t.length - 1].on; }
  async function settle(seconds) {
    const t0 = Date.now();
    let used = lines().reduce((m, l) => Math.max(m, l.n), 0), quiet = Date.now(), clicks = 0;
    while (Date.now() - t0 < (seconds || 60) * 1000) {
      const fresh = lines().filter(l => l.n > used);
      if (fresh.length) used = fresh[fresh.length - 1].n;
      const dialog = fresh.filter(l => l.kind === 'dialog').pop();
      if (dialog) { await click(...dialog.ok); clicks++; quiet = Date.now(); }
      else if (talking()) { await click(400, 150); clicks++; quiet = Date.now(); }
      else if (Date.now() - quiet > 2000) break;
      await sleep(800);
    }
    return {clicks, seconds: Math.round((Date.now() - t0) / 1000), screen: screen()};
  }
  async function battle(seconds) { return play(seconds, 'battle'); }
  async function minigame(seconds) { return play(seconds, 'minigame'); }
  async function play(seconds, target) {
    const t0 = Date.now(), end = t0 + (seconds || 600) * 1000;
    const out = {result: '', words: [], scrambles: 0, dialogs: 0, seconds: 0};
    const start = lines();
    let used = start.reduce((m, l) => Math.max(m, l.n), 0), seen = false, nextTalk = 0;
    if (start.length && ['board', 'wordmaster'].includes(start[start.length - 1].kind)) used--;  // a board already waiting
    while (Date.now() < end) {
      const all = lines(), fresh = all.filter(l => l.n > used);
      const scr = screen();
      if (scr === target) seen = true;
      else if (seen && scr !== 'none') { out.result = scr; break; }
      if (fresh.length) used = fresh[fresh.length - 1].n;
      const dialog = fresh.filter(l => l.kind === 'dialog').pop();
      const board = fresh.filter(l => l.kind === 'board' || l.kind === 'wordmaster').pop();
      if (dialog) { out.dialogs++; await click(...dialog.ok); await sleep(1500); continue; }
      if (board && scr === target) {
        let tiles = board.tiles;
        if (target === 'minigame' && board.kind === 'board') {
          // a mini-game wants a word it has not had yet (Letter Rip deals one word to take others from)
          const w = board.words.find(w => !out.words.includes(w) && spell(w, board.letters));
          board.word = w || '-';
          tiles = w ? spell(w, board.letters) : [];
        }
        if (board.word !== '-') {
          out.words.push(board.word);
          for (const [x, y] of tiles) { await click(x, y); await sleep(300); }
          await sleep(400);
          if (board.attack) await click(...board.attack);
        } else if (board.scramble) { out.scrambles++; await click(...board.scramble); }
        await sleep(target === 'battle' ? 2500 : 1000);
        continue;
      }
      if (Date.now() >= nextTalk) {
        // a conversation goes on with a click; on the map, Enter starts the chapter
        await click(400, scr === 'map' && !seen ? 575 : 150);
        nextTalk = Date.now() + 3000;
      }
      await sleep(300);
    }
    out.seconds = Math.round((Date.now() - t0) / 1000);
    if (!out.result) out.result = 'timeout in ' + screen();
    return out;
  }
  function setup() {
    window.requestAnimationFrame = cb => setTimeout(() => cb(performance.now()), 16);
    if (!Module.__bwaArgs) {
      Module.__bwaArgs = true;
      const callMain = Module.callMain;
      Module.callMain = args => callMain(args.concat(['-boardlog=' + LOG]));
    }
  }
  window.bwa = {setup, click, key, lines, screen, talking, settle, waitScreen, battle, minigame, spell, sleep};
})();
