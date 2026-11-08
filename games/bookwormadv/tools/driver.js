// Bookworm Adventures' driver for the web build: plays with real DOM events on the canvas, reading what to do from
// the port's -boardlog (GameApp::LogBoardForTests, TestHooks.cpp). Pasted into the page (the Browser pane's
// javascript_tool, or fetched and eval'd), it defines window.bwa:
//
//   bwa.setup()        before Play: the requestAnimationFrame shim (a hidden pane runs none) and -boardlog=/tmp/board.log
//                      on the game's command line (the page passes it through Module.callMain)
//   bwa.click(x, y)    a move, then a press and a release at game pixel (x, y) (SDL puts a button at the last move)
//   bwa.key(name)      keydown, keypress, keyup on the canvas ("Enter", "a", ...)
//   bwa.lines()        the log's lines, parsed: {kind: "board"|"dialog"|"screen", n, ...}
//   bwa.screen()       the screen the log last named (title, menu, map, battle, minigame, treasure, ...)
//   await bwa.waitScreen(names, seconds)
//                      until the screen is one of names (an array); clicks conversations on meanwhile
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
      if (kind === 'dialog') return {kind, n, id: +f[2], ok: pt(f[4])};
      const k = f.indexOf('attack');
      return {kind, n, word: f[2], tiles: f.slice(3, k).map(pt), attack: pt(f[k + 1]), scramble: pt(f[k + 3])};
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
  async function battle(seconds) {
    const t0 = Date.now(), end = t0 + (seconds || 600) * 1000;
    const out = {result: '', words: [], scrambles: 0, dialogs: 0, seconds: 0};
    let used = lines().reduce((m, l) => Math.max(m, l.n), 0), seen = false, nextTalk = 0;
    while (Date.now() < end) {
      const all = lines(), fresh = all.filter(l => l.n > used);
      const scr = screen();
      if (scr === 'battle') seen = true;
      else if (seen && scr !== 'none') { out.result = scr; break; }
      if (fresh.length) used = fresh[fresh.length - 1].n;
      const dialog = fresh.filter(l => l.kind === 'dialog').pop();
      const board = fresh.filter(l => l.kind === 'board').pop();
      if (dialog) { out.dialogs++; await click(...dialog.ok); await sleep(1500); continue; }
      if (board && scr === 'battle') {
        if (board.word !== '-') {
          out.words.push(board.word);
          for (const [x, y] of board.tiles) { await click(x, y); await sleep(300); }
          await sleep(400);
          if (board.attack) await click(...board.attack);
        } else if (board.scramble) { out.scrambles++; await click(...board.scramble); }
        await sleep(2500);
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
  window.bwa = {setup, click, key, lines, screen, waitScreen, battle, sleep};
})();
