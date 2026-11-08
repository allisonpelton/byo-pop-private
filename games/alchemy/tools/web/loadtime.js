// The web build's load time (games/alchemy/docs/GAME_NOTES.md, "Performance"): milliseconds from the page's navigation start to the
// first state in which the title takes input (state's "ready": the play link is shown), read from performance.now(), which counts
// from the navigation start; plus the WebAssembly heap and the JS heap at that moment, which are not a process's working set.
// Opened as .../alchemy.html?player=1&assets=1&silent=1&driver=games/alchemy/tools/web/loadtime.js&run=web-load
var sleep = function (ms) { return new Promise(function (r) { setTimeout(r, ms); }); };
var tReady = null, sample = null;
var t0 = performance.now();
while (tReady === null && performance.now() - t0 < 120000) {
  var s = await t.state();
  if (s.ready) {
    tReady = performance.now();
    var heap = (typeof Module !== "undefined" && Module.HEAP8) ? Module.HEAP8.length : null;
    sample = { updates: t.updates(), wasmHeapMB: heap ? Math.round(heap / 1048576 * 10) / 10 : null,
      jsHeapMB: performance.memory ? Math.round(performance.memory.usedJSHeapSize / 1048576 * 10) / 10 : null };
    break;
  }
  await sleep(10);
}
var nav = performance.getEntriesByType("navigation")[0];
return { readyMs: tReady === null ? null : Math.round(tReady), driverStartedMs: Math.round(t0),
  domContentLoadedMs: nav ? Math.round(nav.domContentLoadedEventEnd) : null, loadEventMs: nav ? Math.round(nav.loadEventEnd) : null, sample: sample };
