// Bonnie's in the Browser pane with real input: tools/realplay.py's steps as DOM mouse and key events on the page's
// canvas (the page's own input path, as a player's mouse and keyboard send them). Paste it into the page
// (bonnies.html?silent=1), then:
//     panePlay(stepsText, onShot)   runs the steps; onShot(name) is called at each "shot" (the pane screenshots it)
//     panePlayLog                   what the run did, a line each, for reading while it runs
// Steps: wait, move, click, type, key, focus, shot, as realplay.py has them, and
//     play <s>   plays the classic game on screen for up to s seconds: answers dialogs with their first button that
//                neither leaves the game nor opens anything, clicks on through the story's pages, spells the word the
//                tutorial's balloon asks for, and otherwise the longest word on the board by the game's own
//                dictionary, then Submit. The board, the buttons and the word come from the page's test export
//                (bonnies_test_state, src/BonniesHost.cpp), which only reads the game.
window.panePlayLog = [];
window.panePlay = async function (text, onShot) {
  const s = ms => new Promise(z => setTimeout(z, ms));
  const c = document.getElementById('canvas');
  const t0 = performance.now();
  const log = line => window.panePlayLog.push(((performance.now() - t0) / 1000).toFixed(1) + ' ' + line);
  const at = (x, y) => {
    const r = c.getBoundingClientRect();
    return { clientX: r.left + x * r.width / 800, clientY: r.top + y * r.height / 600, bubbles: true, button: 0 };
  };
  const mouse = (t, x, y, b) => c.dispatchEvent(new MouseEvent(t, Object.assign(at(x, y), { buttons: b || 0 })));
  const click = async (x, y) => {
    mouse('mousemove', x, y); await s(150);
    mouse('mousedown', x, y, 1); await s(80);
    mouse('mouseup', x, y);
  };
  const KEYS = { RETURN: ['Enter', 'Enter', 13], ESCAPE: ['Escape', 'Escape', 27], BACK: ['Backspace', 'Backspace', 8],
    SPACE: [' ', 'Space', 32] };
  // the page reads keyCode, which a script's KeyboardEvent leaves 0, so the codes are given as a keyboard's are
  const kev = (t, o, codes) => {
    const e = new KeyboardEvent(t, o);
    for (const k in codes) Object.defineProperty(e, k, { get: () => codes[k] });
    c.dispatchEvent(e);
  };
  const key = (k, code, kc, ch) => {
    const o = { key: k, code: code, bubbles: true, cancelable: true };
    kev('keydown', o, { keyCode: kc, which: kc, charCode: 0 });
    if (ch) kev('keypress', o, { keyCode: ch, which: ch, charCode: ch });
    kev('keyup', o, { keyCode: kc, which: kc, charCode: 0 });
  };

  const DENY = ['quit', 'menu', 'more', 'buy', 'web', 'exit', 'cancel', 'register', 'option', 'help', 'back', 'high',
    'score', 'credit', 'no'];
  const allowed = t => {
    const l = t.toLowerCase();
    return l.trim() !== '' && !DENY.some(w => l.split(/\s+/).includes(w) || (w.length > 3 && l.includes(w)));
  };
  const state = () => {
    const st = { modal: 0, buttons: [], game: null, balloon: null, says: '', marked: [], hint: [0, []], word: [] };
    for (const line of Module.ccall('bonnies_test_state', 'string', [], []).split('\n')) {
      const a = line.split(' ');
      if (a[0] === 'modal') st.modal = +a[1];
      else if (a[0] === 'button') st.buttons.push({ id: +a[2], x: a[3] * 800, y: a[4] * 600, text: a.slice(5).join(' ') });
      else if (a[0] === 'game') st.game = a.slice(1).map(Number);
      else if (a[0] === 'balloon') st.balloon = a.slice(1).map(Number);
      else if (a[0] === 'says') st.says = a.slice(1).join(' ');
      else if (a[0] === 'marked')
        for (let i = 1; i + 4 < a.length; i += 5) st.marked.push({ ch: a[i], col: +a[i + 1], row: +a[i + 2], x: a[i + 3] * 800, y: a[i + 4] * 600 });
      else if (a[0] === 'hint')
        st.hint = [+a[1], Array.from({ length: +a[2] }, (_, i) => ({ ch: a[3 + 3 * i], x: a[4 + 3 * i] * 800, y: a[5 + 3 * i] * 600 }))];
      else if (a[0] === 'word' && a.length > 1 && a[1])
        for (let i = 0; i < a[1].length; i++) st.word.push({ ch: a[1][i], x: a[2 + 2 * i] * 800, y: a[3 + 2 * i] * 600 });
    }
    return st;
  };
  // odd columns sit half a cell lower (game/Game.h)
  const touch = (a, b) => {
    if (a.col === b.col) return Math.abs(a.row - b.row) === 1;
    if (Math.abs(a.col - b.col) !== 1) return false;
    const low = a.col % 2 ? a.row : b.row, high = a.col % 2 ? b.row : a.row;
    return low === high || low === high - 1;
  };
  const tutorialPath = (says, marked) => {
    const w = says.split(/\s+/).map(x => x.replace(/[^A-Za-z]/g, '')).find(x => x.length >= 3 && x === x.toUpperCase());
    if (!w || !marked.length) return null;
    const walk = path => {
      if (path.length === w.length) return path;
      for (const m of marked) {
        if (path.includes(m) || m.ch !== w[path.length]) continue;
        if (path.length && !touch(path[path.length - 1], m)) continue;
        const f = walk(path.concat([m]));
        if (f) return f;
      }
      return null;
    };
    return walk([]);
  };
  const shoot = async name => { if (onShot) await onShot(name); };

  const play = async seconds => {
    const end = performance.now() + seconds * 1000;
    let last = '', still = performance.now(), level = 0, words = 0, seen = false, lastWord = '', lastGame = '', refused = 0;
    // the level's filled cells (the test command "state"), and the words since that number last rose
    let filled = -1, idle = 0, fillLevel = 0;
    const fillState = () => {
      try { const j = JSON.parse(Module.ccall('bonnies_test_command', 'string', ['string'], ['state']).slice(6)); return [j.filled, j.fill]; }
      catch (e) { return null; }
    };
    while (performance.now() < end) {
      const st = state();
      const g = st.game;
      const summary = JSON.stringify([st.modal, g, st.modal > 0 ? st.buttons.map(b => b.text) : 0, st.balloon]);
      if (summary !== last) {
        log('state ' + summary);
        last = summary; still = performance.now();
      }
      if (g && g[0] !== level) {
        if (level) await shoot('level' + String(g[0]).padStart(2, '0'));
        level = g[0];
      }
      if (st.modal > 0) {
        const b = st.buttons.find(b => allowed(b.text));
        if (b) { log('dialog: ' + b.text); await click(b.x, b.y); }
        else if (performance.now() - still > 3000) { log('dialog without a button to press: RETURN'); key('Enter', 'Enter', 13, 13); still = performance.now(); }
        await s(1500);
        continue;
      }
      if (!g) {
        if (seen) { log('the game screen is gone: the play ends'); return words; }
        if (performance.now() - still > 6000) { log('before the board: a click on the next arrow'); await click(745, 565); still = performance.now(); }
        await s(300);
        continue;
      }
      seen = true;
      const [lvl, score, goal, inprog, phase, busy, c60] = g;
      // a balloon's own button (Continue): any button but the board's Menu, Submit and Scramble
      const own = st.buttons.find(b => b.id !== 0xc9 && b.id !== 0xca && b.id !== 0xcb && allowed(b.text));
      if (own) {
        log('button: ' + own.text);
        await click(own.x, own.y);
        still = performance.now();
        await s(1200);
        continue;
      }
      const balloon = st.balloon && st.balloon[0];
      let tiles = null;
      if (phase === 1 && c60 === 0) {
        if (balloon) {
          const path = tutorialPath(st.says, st.marked);
          if (path) { tiles = path; log('the tutorial asks for ' + path.map(m => m.ch).join('')); }
          // a balloon that marks no tiles leaves the word to the player (TALE: "Now find the word...")
          else if (!st.marked.length && st.word.length >= 3) tiles = st.word;
          else if (!st.marked.length && st.hint[0] && st.hint[1].length >= 3) tiles = st.hint[1];
        } else if (st.word.length >= 3) tiles = st.word;
        // the game's own hint, when it shows one and the finder has nothing (a word of other tiles than plain letters)
        else if (st.hint[0] && st.hint[1].length >= 3) tiles = st.hint[1];
      }
      // the same word refused twice with nothing changed: the game's own hint, when it shows one
      const spelled = tiles ? tiles.map(t => t.ch).join('') : '';
      if (tiles && spelled === lastWord && JSON.stringify(g) === lastGame && ++refused >= 2) {
        tiles = st.hint[0] && st.hint[1].length >= 3 ? st.hint[1] : null;
        if (!tiles) { await s(500); continue; }
      } else if (tiles && spelled !== lastWord) refused = 0;
      // the latest words fill none of the cells left: Scramble, as a player would
      const fs = phase === 1 && !balloon ? fillState() : null;
      if (fs) {
        // the best count so far on this level (a level's toggling cells take the count down as well as up)
        if (lvl !== fillLevel) { fillLevel = lvl; filled = -1; }
        if (fs[0] > filled) { filled = fs[0]; idle = 0; }
        const scramble = st.buttons.find(b => b.id === 0xcb);
        // (three words without a new cell when three or fewer are left, eight when more are)
        if (idle >= (fs[1] - filled <= 3 ? 3 : 8) && scramble) {
          log('scramble: ' + (fs[1] - fs[0]) + ' cells left, none filled in ' + idle + ' words');
          await click(scramble.x, scramble.y);
          idle = 0;
          still = performance.now();
          await s(1500);
          continue;
        }
      }
      if (tiles) {
        idle++;
        lastWord = spelled; lastGame = JSON.stringify(g);
        log('word ' + tiles.map(t => t.ch).join('') + ' at level ' + lvl + ' (score ' + score + ' of ' + goal + ')');
        // a click off the board first, as a player clears a path left over (Game::pick off the board)
        await click(20, 300); await s(100);
        for (const t of tiles) { await click(t.x, t.y); await s(60); }
        const submit = st.buttons.find(b => b.id === 0xca);
        await click(submit ? submit.x : 450, submit ? submit.y : 557);
        words++;
        // the next move waits for the game to take this one (its score, turns or phase change), up to 8 s
        const before = JSON.stringify(g);
        // (read through the test command "state", which runs no word search, where the command is there)
        const quick = () => { try { const j = JSON.parse(Module.ccall('bonnies_test_command', 'string', ['string'], ['state']).slice(6)); return [j.level, j.score, j.moves, j.phase, j.modal, j.board + '']; } catch (e) { return null; } };
        const q0 = quick();
        if (q0) for (let w = 0; w < 40 && JSON.stringify(quick().slice(0, 5)) === JSON.stringify(q0.slice(0, 5)); w++) await s(200);
        else for (let w = 0; w < 40 && JSON.stringify(state().game) === before; w++) await s(200);
        // and for the board to settle (the tiles fall after the score), so that the next word's clicks find their
        // tiles where the finder saw them: the same board twice 400 ms apart, up to 6 s
        await s(600);
        const board = () => { const q = quick(); return q ? q[5] : ''; };
        for (let w = 0, b = board(); w < 15; w++) { await s(400); const n = board(); if (n === b) break; b = n; }
        still = performance.now();
        continue;
      }
      // a page between levels waits for a click on the story's next arrow; on the board a click would take a tile, so
      // there the player only waits
      if (performance.now() - still > 6000 && phase !== 1) {
        log('nothing moves (phase ' + phase + '): a click on the next arrow');
        await click(745, 565);
        still = performance.now();
      }
      await s(300);
    }
    log('the time is up');
    return words;
  };

  let count = 0;
  for (let line of text.split('\n')) {
    line = line.split('#')[0].trim();
    if (!line) continue;
    const a = line.split(/\s+/);
    const n = a.slice(1).map(Number);
    if (a[0] === 'wait') await s(n[0]);
    else if (a[0] === 'move') mouse('mousemove', n[0], n[1]);
    else if (a[0] === 'click') await click(n[0], n[1]);
    else if (a[0] === 'type') {
      for (const ch of line.slice(5)) {
        const up = ch.toUpperCase();
        key(ch, /[a-z]/i.test(ch) ? 'Key' + up : '', up.charCodeAt(0), ch.charCodeAt(0));
        await s(60);
      }
    } else if (a[0] === 'key') { const k = KEYS[a[1]]; key(k[0], k[1], k[2], k[2] === 13 || k[2] === 32 ? k[2] : 0); }
    else if (a[0] === 'focus') { window.dispatchEvent(new Event(a[1] === 'lose' ? 'blur' : 'focus')); document.dispatchEvent(new Event('visibilitychange')); }
    else if (a[0] === 'shot') { await s(100); await shoot(a[1]); }
    else if (a[0] === 'play') log('played ' + await play(n[0]) + ' words');
    else log('unknown step ' + line);
    log(line);
    count++;
  }
  return count;
};
