// Checks that a game page's start loads the saved files as Emscripten's IDBFS does (web/shell.html's loadStorage against
// syncfs(true); docs/WEB_PLAYER.md, item 8): loadStorage leans on IDBFS's internals (getDB, DB_STORE_NAME, storeLocalEntry) and
// MEMFS's (node_ops, stream_ops), so an Emscripten update that changes them must fail here, not on a player's return visit.
//   node tools/storageloadtest.js [build/web]
// Under Node, with a built game's own module (the smallest game wasm in build/web; its IDBFS and MEMFS are the ones the pages
// run) in three fresh instances, and an IndexedDB of our own in memory (below) that all three share:
//   1. the first writes a tree of folders, files and a link with chosen modes and times, and syncfs(false) stores it;
//   2. the second mounts the store and loads it with syncfs(true);
//   3. the third mounts it and loads it with loadStorage, taken from web/shell.html as the page has it.
// The second and third must then hold the same: every entry in the same order (each folder's listing), with the same mode,
// times and contents, and the same entries that the next syncfs(false) would find changed. Both run on one fixed clock, so
// the times that the loading itself sets (a folder's, when an entry is made in it) compare too. An entry the store holds
// without a timestamp is left out by both (IDBFS lists the entries through its timestamp index).
// It prints "storageloadtest: N checks, M failed" and exits 1 when any failed.
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const crypto = require("crypto");

const root = path.join(__dirname, "..");
const web = path.resolve(process.argv[2] || path.join(root, "build", "web"));
const PERSIST = "/persisttest";
const NOW = 1795000000000;  // the fixed clock of every instance (2026-11-17)

let checks = 0, failed = 0;
function check(what, got, want) {
  checks++;
  const g = JSON.stringify(got), w = JSON.stringify(want);
  if (g !== w) { failed++; console.log(`FAIL ${what}:\n  got  ${g.slice(0, 600)}\n  want ${w.slice(0, 600)}`); }
}

// ---------- an IndexedDB in memory: what IDBFS and loadStorage use of it, with its ordering and its asynchrony ----------
// Keys order as IndexedDB orders them: dates by time, strings by code unit (as JS compares strings); values are stored and
// returned as structured clones; every request answers on a later turn, and a transaction completes once no request of it is
// left, as a real one does.
function cmpKey(a, b) {
  const da = a instanceof Date, db = b instanceof Date;
  if (da !== db) return da ? -1 : 1;  // (dates before strings)
  const x = da ? a.getTime() : a, y = db ? b.getTime() : b;
  return x < y ? -1 : x > y ? 1 : 0;
}
function memoryIndexedDB() {
  const dbs = new Map();
  function later(fn) { setImmediate(fn); }
  function transaction(data, mode) {
    const tx = { pending: 0, done: false, error: null, mode, oncomplete: null, onerror: null, onabort: null };
    function settle() {
      later(() => {
        if (tx.pending || tx.done) return;
        tx.done = true;
        if (tx.oncomplete) tx.oncomplete({ target: tx });
      });
    }
    function request(run) {
      const req = { result: undefined, error: null, onsuccess: null, onerror: null, transaction: tx };
      tx.pending++;
      later(() => {
        try { req.result = run(); } catch (e) { req.error = e; }
        tx.pending--;
        if (req.error) { if (req.onerror) req.onerror({ target: req, preventDefault() {} }); }
        else if (req.onsuccess) req.onsuccess({ target: req });
        settle();
      });
      return req;
    }
    tx.objectStore = (name) => {
      const store = data.stores.get(name);
      if (!store) throw new Error("no store " + name);
      const sortedKeys = () => Array.from(store.rows.keys()).sort(cmpKey);
      const writable = () => { if (tx.mode !== "readwrite" && tx.mode !== "versionchange") throw new Error("read-only transaction"); };
      return {
        indexNames: { contains: (n) => store.indexes.has(n) },
        createIndex(n, keyPath) { store.indexes.set(n, keyPath); },
        index(n) {
          const keyPath = store.indexes.get(n);
          return {
            // the entries whose value has the key path, by that key then by their own key, as a key cursor
            openKeyCursor() {
              const list = sortedKeys().map((k) => [store.rows.get(k)[keyPath], k])
                .filter(([ik]) => ik instanceof Date || typeof ik === "string" || typeof ik === "number")
                .sort((p, q) => cmpKey(p[0], q[0]) || cmpKey(p[1], q[1]));
              let at = 0;
              const req = { onsuccess: null, onerror: null };
              function next() {
                tx.pending++;
                later(() => {
                  tx.pending--;
                  const row = list[at++];
                  req.result = row ? { key: structuredClone(row[0]), primaryKey: row[1], continue: next } : null;
                  if (req.onsuccess) req.onsuccess({ target: req });
                  settle();
                });
              }
              next();
              return req;
            }
          };
        },
        get: (k) => request(() => store.rows.has(k) ? structuredClone(store.rows.get(k)) : undefined),
        put: (v, k) => { writable(); const copy = structuredClone(v); return request(() => { store.rows.set(k, copy); return k; }); },
        delete: (k) => { writable(); return request(() => { store.rows.delete(k); }); },
        getAll: () => request(() => sortedKeys().map((k) => structuredClone(store.rows.get(k)))),
        getAllKeys: () => request(() => sortedKeys())
      };
    };
    tx.abort = () => { tx.done = true; if (tx.onabort) later(() => tx.onabort({ target: tx, preventDefault() {} })); };
    settle();
    return tx;
  }
  function handle(data) {
    return {
      objectStoreNames: { contains: (n) => data.stores.has(n) },
      createObjectStore(n) { data.stores.set(n, { rows: new Map(), indexes: new Map() }); return transaction(data, "versionchange").objectStore(n); },
      transaction: (names, mode) => transaction(data, mode || "readonly"),
      close() {}
    };
  }
  return {
    dbs,
    open(name, version) {
      const req = { result: null, onsuccess: null, onerror: null, onupgradeneeded: null };
      later(() => {
        let data = dbs.get(name);
        const fresh = !data || data.version < version;
        if (!data) { data = { version: 0, stores: new Map() }; dbs.set(name, data); }
        const db = handle(data);
        if (fresh) {
          data.version = version;
          if (req.onupgradeneeded) req.onupgradeneeded({ target: { result: db, transaction: transaction(data, "versionchange") } });
        }
        req.result = db;
        if (req.onsuccess) req.onsuccess({ target: req });
      });
      return req;
    }
  };
}

// ---------- a game's module, a fresh instance on the shared store and the fixed clock ----------
// the smallest of the site's finished games (import-games.js names them), whose module the pages run as built
function pickModule() {
  let listed = "";
  try { listed = fs.readFileSync(path.join(web, "import-games.js"), "utf8"); } catch (e) { return null; }
  const games = Array.from(listed.matchAll(/ByopopImport\.add\("([a-z0-9_]+)"/g), (m) => m[1])
    .filter((g) => fs.existsSync(path.join(web, g + ".wasm")) && fs.existsSync(path.join(web, g + ".js")))
    .map((g) => ({ name: g, size: fs.statSync(path.join(web, g + ".wasm")).size }))
    .sort((a, b) => a.size - b.size);
  return games.length ? games[0].name : null;
}
const game = pickModule();
if (!game) { console.log("storageloadtest: no game module in " + web + " (build-web first): 1 checks, 1 failed"); process.exit(1); }
const glue = fs.readFileSync(path.join(web, game + ".js"), "utf8");
const compiled = new WebAssembly.Module(fs.readFileSync(path.join(web, game + ".wasm")));
const idb = memoryIndexedDB();

function instance() {
  return new Promise((resolve, reject) => {
    const ctx = {
      console: { log() {}, warn() {}, error() {}, info() {}, debug() {} }, process, require, Buffer, URL, TextDecoder, TextEncoder,
      setTimeout, clearTimeout, setInterval, clearInterval, setImmediate, queueMicrotask, structuredClone, performance, WebAssembly,
      __dirname: web, __filename: path.join(web, game + ".js"), indexedDB: idb
    };
    ctx.Module = {
      noInitialRun: true, print() {}, printErr() {},
      instantiateWasm(imports, done) { WebAssembly.instantiate(compiled, imports).then((inst) => done(inst, compiled), reject); return {}; },
      onRuntimeInitialized() { resolve(ctx); }
    };
    vm.createContext(ctx);
    vm.runInContext(`Date.now = function () { return ${NOW}; };`, ctx);
    try { vm.runInContext(glue, ctx, { filename: game + ".js" }); } catch (e) { reject(e); }
  });
}
function mount(ctx) {
  const FS = ctx.Module.FS;
  try { FS.mkdir(PERSIST); } catch (e) {}
  FS.mount(FS.filesystems.IDBFS, {}, PERSIST);
  return FS;
}
const syncfs = (FS, populate) => new Promise((resolve, reject) => FS.syncfs(populate, (err) => err ? reject(err) : resolve()));

// web/shell.html's loadStorage as the page has it, in the instance's own scope
function loadStorageOf(ctx) {
  const shell = fs.readFileSync(path.join(root, "web", "shell.html"), "utf8");
  const m = /\n  function loadStorage\(fs\) \{[\s\S]*?\n  \}\n/.exec(shell);
  if (!m) throw new Error("web/shell.html has no loadStorage(fs) at two spaces' indent");
  return vm.runInContext(`(function () { var PERSIST = ${JSON.stringify(PERSIST)};\n${m[0]}\nreturn loadStorage; })()`, ctx);
}

// What a loaded tree holds: every entry under the mount, each folder listed in its own order, with its mode, times and contents
// (a hash), and for a link its target
function fingerprint(FS) {
  const out = [];
  (function walk(dir) {
    for (const name of FS.readdir(dir)) {
      if (name === "." || name === "..") continue;
      const p = dir + "/" + name, st = FS.lstat(p);
      const row = { path: p, mode: st.mode, mtime: +st.mtime, atime: +st.atime };
      if (FS.isLink(st.mode)) row.link = FS.readlink(p);
      else if (FS.isFile(st.mode)) row.sha1 = crypto.createHash("sha1").update(FS.readFile(p)).digest("hex");
      out.push(row);
      if (FS.isDir(st.mode)) walk(p);
    }
  })(PERSIST);
  return out;
}
// What the next syncfs(false) would store and remove: IDBFS's own sets, compared as its reconcile compares them
function nextPersist(FS) {
  const IDBFS = FS.filesystems.IDBFS, mnt = FS.lookupPath(PERSIST).node.mount;
  return new Promise((resolve, reject) => IDBFS.getLocalSet(mnt, (e1, local) => e1 ? reject(e1) : IDBFS.getRemoteSet(mnt, (e2, remote) => {
    if (e2) return reject(e2);
    const create = Object.keys(local.entries).filter((k) => !remote.entries[k] || +remote.entries[k].timestamp !== +local.entries[k].timestamp).sort();
    const remove = Object.keys(remote.entries).filter((k) => !local.entries[k]).sort();
    resolve({ create, remove });
  })));
}

(async () => {
  // 1. the tree, stored by IDBFS itself
  const a = await instance(), FA = mount(a);
  const bytes = (n, seed) => { const b = new Uint8Array(n); for (let i = 0; i < n; i++) b[i] = (i * 31 + seed) & 255; return b; };
  const T = (d) => Date.UTC(2026, 0, d, 12, 0, 0);
  FA.mkdirTree(PERSIST + "/game/images");
  FA.mkdirTree(PERSIST + "/game/a");
  FA.mkdirTree(PERSIST + "/user/Players");
  FA.mkdirTree(PERSIST + "/mods/one/images");
  FA.writeFile(PERSIST + "/game/a.txt", "a.txt sorts before a/ (\".\" before \"/\")");
  FA.writeFile(PERSIST + "/game/a/b.dat", bytes(70000, 1));
  FA.writeFile(PERSIST + "/game/a-b.dat", bytes(3, 2));
  FA.writeFile(PERSIST + "/game/Main.PAK", bytes(4096, 3));
  FA.writeFile(PERSIST + "/game/images/Zed.png", bytes(1000, 4));
  FA.writeFile(PERSIST + "/game/images/alpha.png", bytes(10, 5));
  FA.writeFile(PERSIST + "/game/images/é – unicode.jpg", bytes(7, 6));
  FA.writeFile(PERSIST + "/game/empty.txt", new Uint8Array(0));
  FA.writeFile(PERSIST + "/user/Players/users.dat", bytes(200, 7));
  FA.writeFile(PERSIST + "/mods/one/mod.txt", "a mod");
  FA.writeFile(PERSIST + "/mods/one/images/x.png", bytes(64, 8));
  FA.symlink(PERSIST + "/game/Main.PAK", PERSIST + "/game/link.pak");
  FA.chmod(PERSIST + "/game/empty.txt", 0o444);
  FA.chmod(PERSIST + "/mods/one", 0o755);
  [["/game/a.txt", 1], ["/game/a/b.dat", 2], ["/game/a-b.dat", 3], ["/game/Main.PAK", 4], ["/game/images/Zed.png", 5], ["/game/images/alpha.png", 6],
    ["/game/images/é – unicode.jpg", 7], ["/game/empty.txt", 8], ["/user/Players/users.dat", 9], ["/mods/one/mod.txt", 10], ["/mods/one/images/x.png", 11],
    ["/game/images", 12], ["/game/a", 13], ["/game", 14], ["/user/Players", 15], ["/user", 16], ["/mods/one/images", 17], ["/mods/one", 18], ["/mods", 19]]
    .forEach(([p, d]) => FA.utime(PERSIST + p, T(d), T(d)));
  await syncfs(FA, false);
  // an entry the store holds without a timestamp: IDBFS's index leaves it out, and loadStorage must too
  const store = idb.dbs.get(PERSIST).stores.get("FILE_DATA").rows;
  store.set(PERSIST + "/game/no-timestamp.dat", { mode: 0o100644, contents: bytes(5, 9) });
  const stored = Array.from(store.keys()).sort(cmpKey);
  check("the store holds the tree", stored.length, 21);

  // 2. IDBFS's own load
  const b = await instance(), FB = mount(b);
  await syncfs(FB, true);
  const byIdbfs = fingerprint(FB), nextIdbfs = await nextPersist(FB);
  // 3. loadStorage
  const c = await instance(), FC = mount(c);
  await loadStorageOf(c)(FC);
  const byLoad = fingerprint(FC), nextLoad = await nextPersist(FC);

  check("IDBFS loaded every entry with a timestamp", byIdbfs.length, 20);
  check("loadStorage: the same entries, in the same order", byLoad.map((r) => r.path), byIdbfs.map((r) => r.path));
  check("loadStorage: the same modes", byLoad.map((r) => [r.path, r.mode]), byIdbfs.map((r) => [r.path, r.mode]));
  check("loadStorage: the same times", byLoad.map((r) => [r.path, r.mtime, r.atime]), byIdbfs.map((r) => [r.path, r.mtime, r.atime]));
  check("loadStorage: the same contents and links", byLoad.map((r) => [r.path, r.sha1 || r.link || ""]), byIdbfs.map((r) => [r.path, r.sha1 || r.link || ""]));
  check("loadStorage: the next syncfs(false) finds the same changed", nextLoad, nextIdbfs);
  check("loadStorage: the entry without a timestamp is left out", byLoad.some((r) => /no-timestamp/.test(r.path)), false);
  // The files keep their stored times, and the folders, where entries were made after them, take the clock's. All but the
  // link's target: IDBFS restores a link with chmod and utime, which follow the link, so its own load gives the target the
  // link's time (and mode), and the next syncfs(false) stores the target again. loadStorage does the same through
  // storeLocalEntry; the games make no links.
  const target = PERSIST + "/game/Main.PAK";
  const fileTimes = byLoad.filter((r) => !FC.isDir(r.mode) && r.path !== target).every((r) => store.get(r.path) && r.mtime === +store.get(r.path).timestamp);
  check("loadStorage: every file and link keeps its stored time, but the link's target", fileTimes, true);
  check("the next syncfs(false) would store the folders and the link's target only", nextLoad.remove.length === 0 &&
    nextLoad.create.every((p) => p === target || FC.isDir(FC.lstat(p).mode)), true);

  console.log(`storageloadtest: ${checks} checks, ${failed} failed (${game}'s module, ${byLoad.length} entries)`);
  process.exit(failed ? 1 : 0);
})().catch((e) => { console.log("storageloadtest: " + (e && e.stack || e)); console.log(`storageloadtest: ${checks + 1} checks, ${failed + 1} failed`); process.exit(1); });
