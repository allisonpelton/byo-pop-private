// Whether a page asks for a game's terms (docs/WEB_PLAYER.md, "The terms"), shared by the game pages (their terms card) and the
// listing page (its agreement for each game it adds). Every build has the card. A page skips it only:
//   - on a development host: 127.0.0.1, localhost, [::1], a *.localhost or *.test name, or a file:// page, which a deployed site
//     never is, so local work in the Browser pane, headless browsers and Node harnesses never meet it, with nothing to switch;
//     ?terms=1 brings the card back there, to look at it;
//   - with the site's test key: a page opened with ?testkey=<key> whose SHA-256 is the hash the build baked in (from the
//     git-ignored secrets/web-testkey.txt; a build without that file has no test key, and its card cannot be skipped). The skip
//     lasts for the tab (sessionStorage), and the key is taken out of the address bar.
// A skip stores no agreement. A page that skipped counts no play (web/playcount.js). The agreement itself is kept per game in
// localStorage, "byopop-terms-<id>", holding the version of its terms (a hash of their text), so changed terms ask again.
var ByopopTerms = (function () {
  "use strict";
  var KEY_HASH = "@BYOPOP_TESTKEY_HASH@";  // hex SHA-256 of the test key, "" for none
  var SESSION = "byopop-testkey";
  if (!/^[0-9a-f]{64}$/.test(KEY_HASH)) KEY_HASH = "";
  var reason = null;   // "dev", "testkey", "pending" (a key to check) or null (the card shows)
  var ready;

  function params() { try { return new URLSearchParams(location.search); } catch (e) { return new URLSearchParams(""); } }
  function devHost(loc) {
    if (!loc) return false;
    if (loc.protocol === "file:") return true;
    var h = String(loc.hostname || "").toLowerCase().replace(/^\[|\]$/g, "");
    return h === "localhost" || h === "127.0.0.1" || h === "::1" || /\.localhost$/.test(h) || /\.test$/.test(h);
  }
  function hex(buffer) {
    return Array.prototype.map.call(new Uint8Array(buffer), function (b) { return ("0" + b.toString(16)).slice(-2); }).join("");
  }
  // the test key out of the address bar, so that it is not bookmarked, shared or shown
  function dropKey() {
    try {
      var u = new URL(location.href);
      if (!u.searchParams.has("testkey")) return;
      u.searchParams.delete("testkey");
      history.replaceState(history.state, "", u.pathname + u.search + u.hash);
    } catch (e) {}
  }

  (function () {
    var p = params(), key = p.get("testkey");
    var loc = typeof location !== "undefined" ? location : null;
    if (p.get("terms") === "1") { reason = null; ready = Promise.resolve(null); dropKey(); return; }
    if (devHost(loc)) { reason = "dev"; ready = Promise.resolve(reason); return; }
    try { if (KEY_HASH && sessionStorage.getItem(SESSION) === KEY_HASH) reason = "testkey"; } catch (e) {}
    if (reason || !key || !KEY_HASH) { dropKey(); ready = Promise.resolve(reason); return; }
    reason = "pending";
    ready = Promise.resolve().then(function () {
      return crypto.subtle.digest("SHA-256", new TextEncoder().encode(key));
    }).then(function (d) {
      if (hex(d) !== KEY_HASH) return null;
      try { sessionStorage.setItem(SESSION, KEY_HASH); } catch (e) {}
      return "testkey";
    }, function () { return null; }).then(function (r) { reason = r; dropKey(); return r; });
  })();

  // why the page skips the card ("dev", "testkey"), "pending" while a test key is being checked, or null
  function skipped() { return reason; }
  // whether the game's terms at this version are agreed to here (or skipped)
  function agreed(id, version) {
    if (reason === "dev" || reason === "testkey") return true;
    try { return localStorage.getItem("byopop-terms-" + id) === version; } catch (e) { return false; }
  }
  // the player agreed: kept (false where storage fails, and the page remembers it until it closes)
  function agree(id, version) {
    try { localStorage.setItem("byopop-terms-" + id, version); return true; } catch (e) { return false; }
  }
  return { ready: ready, skipped: skipped, agreed: agreed, agree: agree, devHost: devHost };
})();
