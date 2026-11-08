// For long runs in a hidden Browser pane (tools/paneplay.js): a hidden page gets no animation frames, and after five
// minutes Chrome lets its timers run about once a minute, so the game (whose waits are setTimeout, under ASYNCIFY)
// crawls. A dedicated worker's timers are not held back, so this page's setTimeout and clearTimeout go through one,
// and requestAnimationFrame becomes a 16 ms timeout. Load it right after the page loads, before Play.
(function () {
  if (window.__paneTimers) return;
  window.__paneTimers = true;
  const src = 'const t = {};' +
    'onmessage = e => { const [kind, id, ms] = e.data;' +
    ' if (kind === "s") t[id] = setTimeout(() => { delete t[id]; postMessage(id); }, ms);' +
    ' else { clearTimeout(t[id]); delete t[id]; } };';
  const worker = new Worker(URL.createObjectURL(new Blob([src], { type: 'text/javascript' })));
  const pending = new Map();
  let next = 1e6;
  worker.onmessage = e => {
    const f = pending.get(e.data);
    pending.delete(e.data);
    if (f) f();
  };
  window.setTimeout = function (fn, ms, ...args) {
    const id = ++next;
    pending.set(id, typeof fn === 'function' ? () => fn(...args) : () => (0, eval)(fn));
    worker.postMessage(['s', id, Math.max(0, +ms || 0)]);
    return id;
  };
  window.clearTimeout = function (id) {
    if (!pending.has(id)) return;
    pending.delete(id);
    worker.postMessage(['c', id]);
  };
  window.requestAnimationFrame = cb => window.setTimeout(() => cb(performance.now()), 16);
  window.cancelAnimationFrame = id => window.clearTimeout(id);
})();
