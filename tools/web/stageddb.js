// A stand-in under Node for the browser's database of a game's files, as web/music.js reads them (files.open, files.read and
// tunesOf), over the staged files under assets/<game>: the database "staged:<game>" holds the store FILE_DATA, its keys
// "staged:<game>/game/<rel>" and each value { contents }. install(assets) puts indexedDB, IDBKeyRange and ByopopStrings on global
// and returns staged(id), a game's staged files as { rel: path }, or null where none are staged. (tools/musictest.js and
// tools/musicrepeattest.js)
"use strict";
const path = require("path"), fs = require("fs");

exports.install = function (assets) {
  const stagedOf = {};
  function staged(id) {
    if (id in stagedOf) return stagedOf[id];
    const base = path.join(assets, id), out = {};
    if (!fs.existsSync(base)) return (stagedOf[id] = null);
    (function walk(d) {
      for (const e of fs.readdirSync(d, { withFileTypes: true })) {
        const p = path.join(d, e.name);
        if (e.isDirectory()) walk(p); else out[path.relative(base, p).split(path.sep).join("/")] = p;
      }
    })(base);
    return (stagedOf[id] = out);
  }
  function answer(fn) {
    const r = {};
    setImmediate(() => { try { r.result = fn(); if (r.onsuccess) r.onsuccess(); } catch (e) { r.error = e; if (r.onerror) r.onerror(); } });
    return r;
  }
  global.IDBKeyRange = { bound: (lo, hi) => [lo, hi] };
  global.indexedDB = {
    open: (persist) => answer(() => {
      const files = staged(persist.replace(/^staged:/, "")) || {}, prefix = persist + "/game/";
      const store = {
        getAllKeys: () => answer(() => Object.keys(files).map((rel) => prefix + rel)),
        get: (key) => answer(() => { const f = files[key.slice(prefix.length)]; return f ? { contents: new Uint8Array(fs.readFileSync(f)) } : undefined; })
      };
      return { objectStoreNames: { contains: (n) => n === "FILE_DATA" }, transaction: () => ({ objectStore: () => store }), close() {} };
    })
  };
  global.ByopopStrings = { t: (k, vars) => k + (vars ? " " + JSON.stringify(vars) : "") };
  return staged;
};
