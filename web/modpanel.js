// The web pages' shared code (docs/MOD_MENU_DESIGN.md 4.4-4.5): cmake/ByopopWeb.cmake appends this file to every
// game's JS with --post-js, so it runs as the game's script loads and leaves Module.byopop:
//   fs, zip, pak, sync   the page's file helpers (the browser's IndexedDB copy of /persist, game files and mods;
//                   a PopCap main.pak's files)
//   Session(game)   restart and autostart, the start that a mod broke, starting once without mods, the import
//                   card after the game closed
//   ModPanel(opts)  the Mods panel: it adds and removes mods, and offers the rescue when a mod stops the game
//                   from starting. Turning mods on and off, their order and their settings are in the game's own
//                   mod menu (F2, framework/ModMenu.h), which the panel opens over and closes back to.
// The panel reads the mods through the game's own model (platform/WebApi.cpp: byopop_mods_open, _state, _edit,
// _rescan; ModManager::GetStateJson), never by reading mod.json files itself: their order, tags and checks exist
// once, in C++. Every edit goes through the model, which writes mods.json by merging (unknown keys stay).
//
// A mod folder the running game uses is never changed under it (it reads data/diff.txt again at every new game):
// removing it turns it off and deletes the folder at the next start ("byopop-pending-remove:<game>" in
// localStorage), and adding a mod into its folder waits in the staging folder until the next start. Turning such
// a mod on again (in the game's mod menu) keeps it, as the panel's Keep does.
//
// The page sets Module.onModsChanged, onModPanel and onModMenuFocus through ModPanel; plain ES5, like the shells.

Module["byopop"] = (function () {
  "use strict";
  var M = Module;

  // ---------- files ----------
  function fsys() { return M["FS"]; }
  function exists(path) { try { fsys().stat(path); return true; } catch (e) { return false; } }
  function isDir(path) { try { return fsys().isDir(fsys().stat(path).mode); } catch (e) { return false; } }
  function listDir(path) {
    try { return fsys().readdir(path).filter(function (n) { return n !== "." && n !== ".."; }); } catch (e) { return []; }
  }
  function rmTree(path) {
    if (!exists(path)) return;
    if (isDir(path)) { listDir(path).forEach(function (n) { rmTree(path + "/" + n); }); fsys().rmdir(path); }
    else fsys().unlink(path);
  }
  function writeFile(path, bytes) {
    fsys().mkdirTree(path.substring(0, path.lastIndexOf("/")));
    fsys().writeFile(path, bytes);
  }
  function readText(path) { try { return fsys().readFile(path, { encoding: "utf8" }); } catch (e) { return null; } }
  function copyTree(from, to) {
    if (isDir(from)) {
      try { fsys().mkdirTree(to); } catch (e) {}
      listDir(from).forEach(function (n) { copyTree(from + "/" + n, to + "/" + n); });
    } else writeFile(to, fsys().readFile(from));
  }
  function moveTree(from, to) {
    rmTree(to);
    try { fsys().rename(from, to); } catch (e) { copyTree(from, to); rmTree(from); }
  }
  // A path inside an upload, made safe to put under a folder: forward slashes, no empty or "." parts, and null
  // for one that would leave the folder ("..", or a drive or root)
  function safeRel(path) {
    var parts = [], bad = false;
    String(path).replace(/\\/g, "/").split("/").forEach(function (p) {
      if (p === "..") bad = true;
      else if (p && p !== ".") parts.push(p);
    });
    if (bad || !parts.length || /:/.test(parts[0])) return null;
    return parts.join("/");
  }
  // A single folder name (a mod's): letters, digits, ".", "_" and "-", and not "." or ".."
  function safeName(name) { return typeof name === "string" && /^[A-Za-z0-9._-]+$/.test(name) && !/^\.+$/.test(name); }

  // Saves the IndexedDB copy, taking turns with the game's own syncs (platform/WebPersist.cpp:
  // Module.sexySyncBusy and sexySyncPending, which the page also waits for before it reloads)
  function sync() {
    return new Promise(function (resolve) {
      var run = function () {
        if (M["sexySyncBusy"]) { setTimeout(run, 50); return; }
        M["sexySyncBusy"] = true;
        M["sexySyncPending"] = false;
        fsys().syncfs(false, function (err) {
          M["sexySyncBusy"] = false;
          if (err) console.warn("Saving to the browser's storage failed:", err);
          if (M["sexySyncPending"]) { run(); return; }  // the game asked for one meanwhile
          resolve();
        });
      };
      run();
    });
  }

  // ---------- zip reading (stored and deflated entries, through DecompressionStream) ----------
  function readZip(buffer) {
    var view = new DataView(buffer), bytes = new Uint8Array(buffer);
    var eocd = -1;
    for (var i = bytes.length - 22; i >= Math.max(0, bytes.length - 65557); i--) {
      if (view.getUint32(i, true) === 0x06054b50) { eocd = i; break; }
    }
    if (eocd < 0) throw new Error(ByopopStrings.t("mods.zip.not_zip"));
    var count = view.getUint16(eocd + 10, true), off = view.getUint32(eocd + 16, true);
    var entries = [];
    for (var n = 0; n < count; n++) {
      if (view.getUint32(off, true) !== 0x02014b50) throw new Error(ByopopStrings.t("mods.zip.directory"));
      var method = view.getUint16(off + 10, true), csize = view.getUint32(off + 20, true);
      var nameLen = view.getUint16(off + 28, true), extraLen = view.getUint16(off + 30, true), commentLen = view.getUint16(off + 32, true);
      var local = view.getUint32(off + 42, true);
      var name = new TextDecoder().decode(bytes.subarray(off + 46, off + 46 + nameLen));
      entries.push({ name: name.replace(/\\/g, "/"), method: method, csize: csize, local: local });
      off += 46 + nameLen + extraLen + commentLen;
    }
    return entries.filter(function (e) { return !/\/$/.test(e.name); }).map(function (e) {
      return {
        path: e.name,
        read: function () {
          var lnl = view.getUint16(e.local + 26, true), lel = view.getUint16(e.local + 28, true);
          var data = bytes.subarray(e.local + 30 + lnl + lel, e.local + 30 + lnl + lel + e.csize);
          if (e.method === 0) return Promise.resolve(data.slice());
          if (e.method !== 8) return Promise.reject(new Error(ByopopStrings.t("mods.zip.compression", {name: e.name})));
          var ds = new DecompressionStream("deflate-raw");
          return new Response(new Blob([data]).stream().pipeThrough(ds)).arrayBuffer().then(function (b) { return new Uint8Array(b); });
        }
      };
    });
  }
  // The files of an <input type=file> (a folder: webkitdirectory) as {path, read()}
  function filesFromInput(fileList) {
    return Array.prototype.map.call(fileList, function (f) {
      return { path: (f.webkitRelativePath || f.name).replace(/\\/g, "/"),
               read: function () { return f.arrayBuffer().then(function (b) { return new Uint8Array(b); }); } };
    });
  }
  function filesFromZip(file) { return file.arrayBuffer().then(readZip); }

  // ---------- PopCap paks (PakInterface's main.pak; tools/popcap_pak.py) ----------
  // The whole file is XORed with one byte (0xF7 in the shipped games): a magic 0xBAC04AC0 and a version, then a
  // record per file (a flags byte, the name's length and name, an int32 size and a FILETIME) until a flags byte
  // with 0x80, then the files' data in record order. Returns the files as {path, read()}, like readZip's.
  function readPak(bytes, key) {
    key = (key === undefined) ? 0xf7 : key;
    function byte(i) {
      if (i >= bytes.length) throw new Error(ByopopStrings.t("mods.pak.directory"));
      return bytes[i] ^ key;
    }
    function u32(i) { return (byte(i) | (byte(i + 1) << 8) | (byte(i + 2) << 16) | (byte(i + 3) << 24)) >>> 0; }
    if (bytes.length < 9 || u32(0) !== 0xbac04ac0) throw new Error(ByopopStrings.t("mods.pak.not_pak"));
    var pos = 8, records = [];
    for (;;) {
      var flags = byte(pos++);
      if (flags & 0x80) break;
      var len = byte(pos), name = "";
      for (var i = 0; i < len; i++) name += String.fromCharCode(byte(pos + 1 + i));
      records.push({ path: name.replace(/\\/g, "/"), size: u32(pos + 1 + len) });
      pos += 1 + len + 12;
    }
    return records.map(function (r) {
      var start = pos;
      pos += r.size;
      if (pos > bytes.length) throw new Error(ByopopStrings.t("mods.pak.short"));
      return {
        path: r.path,
        read: function () {
          var data = bytes.slice(start, start + r.size);
          for (var j = 0; j < data.length; j++) data[j] ^= key;
          return Promise.resolve(data);
        }
      };
    });
  }

  // ---------- this tab's and this browser's notes ----------
  function storage(kind) { try { return window[kind] || null; } catch (e) { return null; } }
  function getItem(kind, key) { try { var s = storage(kind); return s ? s.getItem(key) : null; } catch (e) { return null; } }
  function setItem(kind, key, value) {
    try {
      var s = storage(kind);
      if (s && value === null) s.removeItem(key);
      else if (s) s.setItem(key, value);
    } catch (e) {}
  }

  // ---------- Session: restarts and starts that did not finish ----------
  // game: a short name ("bejeweled") that keeps each game's notes apart on one site.
  function Session(game) {
    this.autostartKey = "byopop-autostart:" + game;  // sessionStorage: start at once after this reload
    this.noModsKey = "byopop-nomods:" + game;        // sessionStorage: ... and without mods (-nomods)
    this.importKey = "byopop-import:" + game;        // sessionStorage: show the import card after this reload
    this.startingKey = "byopop-starting:" + game;    // localStorage: a start that has not finished loading
  }
  // The game's own restart (Module.onGameExit({restart, dropped})): the page reloads, since the game cannot run
  // twice in one page, and starts it at once, without the Play overlay. Sound comes back with the first click or
  // key. A restart drops some of the game's arguments, and the page parameter that passed one goes with it:
  // -gameseed takes ?seed=, so a seed that was played is not played again, and -replay (Big Money's page) takes
  // ?replay=, so the replay that was being watched is not watched again; -nomods is not passed again.
  var DROPPED_PARAMS = { "-gameseed": "seed", "-replay": "replay" };
  Session.prototype.restartPage = function (info) {
    setItem("sessionStorage", this.autostartKey, "1");
    this.reload(info);
  };
  // The Game files panel's "Import again" once the game has closed (it runs once in a page): the new page shows
  // the import card
  Session.prototype.reloadToImport = function (info) {
    setItem("sessionStorage", this.importKey, "1");
    this.reload(info);
  };
  // A reload after the game closed; info: Module.onGameExit's, whose dropped arguments take their page parameters
  Session.prototype.reload = function (info) {
    var dropped = (info && info.dropped) || [];
    var url = new URL(location.href), changed = false;
    Object.keys(DROPPED_PARAMS).forEach(function (arg) {
      if ((dropped.indexOf(arg) >= 0) && url.searchParams.has(DROPPED_PARAMS[arg])) {
        url.searchParams.delete(DROPPED_PARAMS[arg]);
        changed = true;
      }
    });
    if (changed) history.replaceState(null, "", url.toString());
    location.reload();
  };
  // After the game stopped: reload and start once without mods
  Session.prototype.reloadWithoutMods = function () {
    setItem("sessionStorage", this.autostartKey, "1");
    setItem("sessionStorage", this.noModsKey, "1");
    location.reload();
  };
  Session.prototype.takeAutostart = function () {
    var on = !!getItem("sessionStorage", this.autostartKey);
    setItem("sessionStorage", this.autostartKey, null);
    return on;
  };
  Session.prototype.takeNoMods = function () {
    var on = !!getItem("sessionStorage", this.noModsKey);
    setItem("sessionStorage", this.noModsKey, null);
    return on;
  };
  Session.prototype.takeImport = function () {
    var on = !!getItem("sessionStorage", this.importKey);
    setItem("sessionStorage", this.importKey, null);
    return on;
  };
  // Set as callMain starts the game, cleared when it has loaded (Module.onGameLoaded) or quit: one still set at
  // the next boot means the game hung or crashed while it loaded, maybe because of a mod.
  Session.prototype.markStarting = function () { setItem("localStorage", this.startingKey, String(Date.now())); };
  Session.prototype.markFinished = function () { setItem("localStorage", this.startingKey, null); };
  Session.prototype.lastStartFailed = function () { return !!getItem("localStorage", this.startingKey); };

  // ---------- ModPanel ----------
  var STYLE =
    ".mp-sr{position:absolute;width:1px;height:1px;margin:-1px;padding:0;overflow:hidden;clip:rect(0 0 0 0);white-space:nowrap;border:0}" +
    ".mp-panel{display:grid;gap:14px}.mp-panel:focus{outline:none}.mp-panel p{margin:0}" +
    ".mp-list{display:grid;gap:8px;margin:0;padding:0;list-style:none}" +
    ".mp-mod{display:grid;grid-template-columns:1fr auto;gap:4px 12px;align-items:start;padding:10px 12px;" +
      "border:1px solid var(--line);border-radius:8px;background:var(--panel-2)}" +
    ".mp-meta{min-width:0}" +
    ".mp-name{font-weight:600;overflow-wrap:anywhere}" +
    ".mp-sub{color:var(--muted);font-size:12.5px;overflow-wrap:anywhere}" +
    ".mp-tags{display:flex;flex-wrap:wrap;gap:2px 10px;margin-top:2px}" +
    ".mp-tag{font-size:11px;letter-spacing:0.04em;text-transform:uppercase;color:var(--muted)}" +
    ".mp-tag.on{color:var(--ok)}.mp-tag.scores{color:var(--warn)}.mp-tag.features,.mp-tag.pending{color:var(--accent)}" +
    ".mp-msg{font-size:12px;overflow-wrap:anywhere;color:var(--muted)}" +
    ".mp-msg.error{color:var(--gem-ruby)}.mp-msg.warn{color:var(--warn)}" +
    ".mp-actions{display:flex;gap:4px}.mp-actions button{padding:2px 8px}" +
    ".mp-empty{color:var(--muted);border:1px dashed var(--line);border-radius:8px;padding:14px}" +
    ".mp-box{display:grid;gap:8px;padding:10px 12px;border:1px solid var(--line);border-radius:8px}" +
    ".mp-box p{margin:0}" +
    ".mp-original{display:flex;gap:6px;align-items:center}.mp-original[hidden]{display:none}.mp-original input{margin:0}";

  function el(tag, cls, text) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text !== undefined) e.textContent = text;
    return e;
  }
  function button(text, onClick, cls) {
    var b = el("button", cls, text);
    b.type = "button";
    b.addEventListener("click", onClick);
    return b;
  }

  // options:
  //   game         the Session name ("bejeweled"): the removal queue's localStorage key
  //   body         the element the panel fills (inside the page's slide-over)
  //   userDir, modsDir, stagingDir   the game's -userdir and -modsdir, and where mods added over a mod in use wait
  //   gameDirs     the game's data folders ("images", ...): a .zip of them is a mod without a folder of its own
  //   gameFiles    the files at the top of the game's folder that a mod may replace, in lower case (BookWorm's word
  //                lists): a folder holding one is a mod too; optional
  //   intro        the first paragraph (what a mod is for this game)
  //   isOpen()     whether the slide-over is showing
  //   show()       show it (the in-game menu's "Add or remove mods")
  //   canPlay()    whether the game can be started now (the rescue's "Play without mods")
  //   playWithoutMods()
  function ModPanel(options) {
    var self = this;
    this.o = options;
    this.state = null;      // the model (byopop_mods_state)
    this.dead = false;      // the game's runtime stopped (an abort): no more calls into it
    this.message = "";
    this.question = null;   // the restart's question, while it is asked
    this.armed = null;      // the folder whose Remove asks for its confirmation
    this.stale = true;

    if (!document.getElementById("mp-style")) {
      var style = el("style");
      style.id = "mp-style";
      style.textContent = STYLE;
      document.head.appendChild(style);
    }
    // The in-game menu's focus for screen readers (Module.onModMenuFocus): a polite live region, the page's own (options.live,
    // there from the start, which is how a live region is announced reliably) or one made here
    this.live = options.live;
    if (!this.live) {
      this.live = el("div", "mp-sr");
      document.body.appendChild(this.live);
    }
    this.live.setAttribute("role", "status");
    this.live.setAttribute("aria-live", "polite");
    this.live.setAttribute("aria-atomic", "true");
    this.liveText = "";

    var root = this.root = el("div", "mp-panel");
    root.tabIndex = -1;  // keeps the focus (and so the keys) in the panel when a control it was on is redrawn
    if (options.intro) root.appendChild(el("p", "note", options.intro));
    root.appendChild(el("p", "note", ByopopStrings.t("mods.intro")));
    var pickers = el("div", "row");
    this.pickFolder = el("input");
    this.pickFolder.type = "file";
    this.pickFolder.setAttribute("webkitdirectory", "");
    this.pickFolder.multiple = true;
    this.pickZip = el("input");
    this.pickZip.type = "file";
    this.pickZip.accept = ".zip,application/zip";
    this.pickZip.multiple = true;
    [this.pickFolder, this.pickZip].forEach(function (input) { input.hidden = true; input.tabIndex = -1; });
    this.addFolderButton = button(ByopopStrings.t("mods.add_folder"), function () { self.pickFolder.click(); });
    this.addFolderButton.title = ByopopStrings.t("mods.add_folder_title");
    this.addZipButton = button(ByopopStrings.t("mods.add_zip"), function () { self.pickZip.click(); });
    this.addZipButton.title = ByopopStrings.t("mods.add_zip_title");
    pickers.appendChild(this.addFolderButton);
    pickers.appendChild(this.addZipButton);
    pickers.appendChild(this.pickFolder);
    pickers.appendChild(this.pickZip);
    root.appendChild(pickers);
    // The Original switch (docs/WEB_PLAYER.md, "The fixes mod on the site"): the site's fixes and lite mods off at once
    this.originalRow = el("label", "mp-original");
    this.originalBox = el("input");
    this.originalBox.type = "checkbox";
    this.originalRow.appendChild(this.originalBox);
    this.originalRow.appendChild(document.createTextNode(ByopopStrings.t("mods.original")));
    this.originalBox.addEventListener("change", function () { self.setOriginal(self.originalBox.checked); });
    root.appendChild(this.originalRow);
    this.list = el("ul", "mp-list");
    this.list.setAttribute("aria-label", ByopopStrings.t("mods.list"));
    root.appendChild(this.list);
    this.rescue = el("div", "mp-box");
    this.rescue.appendChild(el("p", "note", ByopopStrings.t("mods.rescue")));
    var rescueRow = el("div", "row");
    rescueRow.appendChild(button(ByopopStrings.t("mods.play_without"), function () { if (self.o.playWithoutMods) self.o.playWithoutMods(); }));
    this.rescue.appendChild(rescueRow);
    root.appendChild(this.rescue);
    this.restartBox = el("div", "mp-box");
    root.appendChild(this.restartBox);
    this.msg = el("p", "note");
    this.msg.setAttribute("role", "status");
    this.msg.setAttribute("aria-live", "polite");
    root.appendChild(this.msg);
    options.body.textContent = "";
    options.body.appendChild(root);

    this.pickFolder.addEventListener("change", function (ev) {
      var entries = filesFromInput(ev.target.files);
      ev.target.value = "";
      self.add(function () { return self.importMods(entries, ""); });
    });
    this.pickZip.addEventListener("change", function (ev) {
      var files = Array.prototype.slice.call(ev.target.files);
      ev.target.value = "";
      self.add(function () {
        return files.reduce(function (p, f) {
          return p.then(function (done) {
            return filesFromZip(f).then(function (e) { return self.importMods(e, f.name.replace(/\.zip$/i, "")); })
              .then(function (r) { return done.concat(r); });
          });
        }, Promise.resolve([]));
      });
    });
  }

  // A call into the game (ccall); null once its runtime has stopped
  ModPanel.prototype.call = function (name, ret, types, args) {
    if (this.dead) return null;
    try { return M["ccall"](name, ret, types, args); }
    catch (e) {
      console.warn(name + " failed:", e);
      this.dead = true;
      this.say(ByopopStrings.t("mods.stopped"));
      return null;
    }
  };
  // One edit of the model ({op, mod, ...}); "" or why nothing changed
  ModPanel.prototype.edit = function (op) {
    var r = this.call("byopop_mods_edit", "string", ["string"], [JSON.stringify(op)]);
    return r === null ? ByopopStrings.t("mods.err.stopped") : r;
  };
  ModPanel.prototype.say = function (text) { this.message = text; this.msg.textContent = text; };
  // What the in-game menu's focus is on, to the live region. The same words twice in a row (a setting changed back to
  // where it was) are told apart by a no-break space, as a region that gets the same text again announces nothing.
  ModPanel.prototype.announce = function (text) {
    text = String(text);
    if (text === this.liveText) text += "\u00a0";
    this.liveText = text;
    this.live.textContent = text;
  };

  // The removal queue: [{dir, id, wasEnabled}] of folders the running game uses, deleted at the next start
  ModPanel.prototype.queueKey = function () { return "byopop-pending-remove:" + this.o.game; };
  ModPanel.prototype.queue = function () {
    try {
      var q = JSON.parse(getItem("localStorage", this.queueKey()) || "[]");
      return Array.isArray(q) ? q.filter(function (e) { return e && safeName(e.dir); }) : [];
    } catch (e) { return []; }
  };
  ModPanel.prototype.setQueue = function (q) { setItem("localStorage", this.queueKey(), q.length ? JSON.stringify(q) : null); };
  ModPanel.prototype.queued = function (dir) { return this.queue().filter(function (e) { return e.dir === dir; })[0] || null; };
  // Takes dir out of the queue; its entry, or null
  ModPanel.prototype.unqueue = function (dir) {
    var hit = this.queued(dir);
    if (hit) this.setQueue(this.queue().filter(function (e) { return e.dir !== dir; }));
    return hit;
  };

  // At every boot, after byopop_userdata_restore and before the game starts: the removals and replacements that
  // waited for this start, then the model
  ModPanel.prototype.boot = function () {
    var o = this.o, self = this, removed = [];
    this.call("byopop_mods_open", null, ["string", "string"], [o.userDir, o.modsDir]);
    // A queued mod that is on in mods.json was turned on again after its Remove: it stays
    this.readState();
    this.queue().forEach(function (e) { rmTree(o.modsDir + "/" + e.dir); removed.push(e.id); });
    this.setQueue([]);
    var staged = listDir(o.stagingDir).filter(safeName);
    staged.forEach(function (dir) { moveTree(o.stagingDir + "/" + dir, o.modsDir + "/" + dir); });
    if (removed.length || staged.length) this.call("byopop_mods_rescan", null, [], []);
    // A removed mod's place in the order and its on/off state go with it (not if another folder has its id)
    removed.forEach(function (id) { if (typeof id === "string" && id) self.edit({ op: "forget", mod: id }); });
    // Every change of the model (the in-game menu's too): read again, so the queue follows it even while the
    // panel is closed
    M["onModsChanged"] = function () { self.stale = true; self.refresh(); };
    M["onModPanel"] = function () { if (o.show) o.show(); };
    M["onModMenuFocus"] = function (text) { self.announce(text); };
    this.refresh();
    if (removed.length || staged.length) sync();
  };

  ModPanel.prototype.refresh = function () {
    this.readState();
    this.render();
  };
  // The model again, then the removal queue made to agree with it
  ModPanel.prototype.readState = function () {
    var text = this.call("byopop_mods_state", "string", [], []);
    if (text !== null) {
      try { this.state = JSON.parse(text); this.stale = false; } catch (e) { console.warn("mods state", e); }
    }
    this.reconcile();
    this.keepOriginal();
  };
  // A queued removal whose mod is on again was taken back in the game's mod menu (Remove turned it off): it is
  // kept, as with Keep, instead of being deleted at the next start while the menu shows it on
  ModPanel.prototype.reconcile = function () {
    var self = this, q = this.queue();
    if (!q.length || !this.state) return;
    var kept = q.filter(function (e) {
      var m = self.mods().filter(function (x) { return x.dir === e.dir; })[0];
      return !(m && m.enabled);
    });
    if (kept.length !== q.length) this.setQueue(kept);
  };
  ModPanel.prototype.mods = function () { return (this.state && this.state.mods) || []; };

  // ---- The Original switch (the user, 2026-11-12: "Yes, flip defaults"; docs/MOD_ROADMAP.md, "What a player gets") ----
  // A player gets the game with the site's fixes and lite mods on; the switch turns both off to play the original as it
  // was. Its state is in the game's user dir beside the fixes mod's note (site-fixes.json): "original" holds each of
  // those mods' on/off state from before the switch went on, and turning it off puts them back exactly. Their own
  // settings (mod-settings.json) are never touched, so every fix's toggle holds too. A mod the player removed stays
  // removed: the switch only turns on what is there.
  var ORIGINAL_DIRS = ["fixes", "lite"];
  ModPanel.prototype.noteFile = function () { return this.o.userDir + "/site-fixes.json"; };
  ModPanel.prototype.note = function () {
    try { return JSON.parse(readText(this.noteFile())) || {}; } catch (e) { return {}; }
  };
  ModPanel.prototype.saveNote = function (note) {
    writeFile(this.noteFile(), new TextEncoder().encode(JSON.stringify(note)));
    sync();
  };
  ModPanel.prototype.originalMods = function () {
    return this.mods().filter(function (m) { return ORIGINAL_DIRS.indexOf(m.dir) >= 0; });
  };
  ModPanel.prototype.setOriginal = function (on) {
    var self = this, note = this.note();
    if (!!note.original === on) return;
    this.switching = true;
    try {
      if (on) {
        var kept = {};
        this.originalMods().forEach(function (m) {
          kept[m.dir] = !!m.enabled;
          if (m.enabled) self.edit({ op: "enable", mod: m.id, on: false });
        });
        note.original = kept;
      } else {
        var was = note.original || {};
        this.originalMods().forEach(function (m) {
          if (was[m.dir] !== undefined && !!was[m.dir] !== !!m.enabled) self.edit({ op: "enable", mod: m.id, on: !!was[m.dir] });
        });
        delete note.original;
      }
      this.saveNote(note);
    } finally { this.switching = false; }
    this.refresh();
  };
  // While the switch is on: a fixes or lite mod that arrives (the site's first install of it) is turned off and
  // remembered as on, as it would have started; one the player turned on again (in the game's mod menu) means the
  // player left the original, so the switch goes off and their mods stay as they are now
  ModPanel.prototype.keepOriginal = function () {
    if (this.switching || !this.state) return;
    var self = this, note = this.note(), kept = note.original;
    if (!kept) return;
    var mods = this.originalMods(), changed = false;
    if (mods.some(function (m) { return kept[m.dir] !== undefined && m.enabled; })) {
      delete note.original;
      this.saveNote(note);
      return;
    }
    this.switching = true;
    try {
      mods.forEach(function (m) {
        if (kept[m.dir] !== undefined) return;
        kept[m.dir] = !!m.enabled;
        changed = true;
        if (m.enabled) self.edit({ op: "enable", mod: m.id, on: false });
      });
    } finally { this.switching = false; }
    if (changed) {
      this.saveNote(note);
      var text = this.call("byopop_mods_state", "string", [], []);
      if (text !== null) { try { this.state = JSON.parse(text); } catch (e) { /* kept */ } }
    }
  };
  // Mods turned on in mods.json (what the next start applies)
  ModPanel.prototype.enabledCount = function () { return this.mods().filter(function (m) { return m.enabled; }).length; };
  ModPanel.prototype.running = function () { return !!(this.state && this.state.running) && !this.dead; };
  // byopop_menu: 1 open, 0 closed, -1 the game does not run
  ModPanel.prototype.menu = function (on) {
    var r = this.call("byopop_menu", "number", ["number"], [on]);
    return r === null ? -1 : r;
  };

  // The page shows the panel: a fresh look at the model, without the last visit's message or question
  ModPanel.prototype.shown = function () {
    this.message = "";
    this.question = null;
    this.armed = null;
    this.refresh();
    if (this.o.body.scrollTo) this.o.body.scrollTo(0, 0);
  };

  ModPanel.prototype.render = function () {
    var self = this, st = this.state || { mods: [], pending: { restart: [] } }, o = this.o;
    var running = this.running();
    var hadFocus = this.root.contains(document.activeElement);
    this.list.textContent = "";
    this.addFolderButton.disabled = this.addZipButton.disabled = this.dead;
    var original = !!this.note().original;
    this.originalRow.hidden = !(this.originalMods().length || original);
    this.originalBox.checked = original;
    this.originalBox.disabled = this.dead;
    if (!st.mods.length) {
      var empty = el("li", "mp-empty", ByopopStrings.t("mods.empty"));
      this.list.appendChild(empty);
    }
    st.mods.forEach(function (m) {
      var queued = self.queued(m.dir);
      var staged = exists(o.stagingDir + "/" + m.dir);
      var row = el("li", "mp-mod");
      var meta = el("div", "mp-meta");
      meta.appendChild(el("div", "mp-name", m.name + (m.version ? " " + m.version : "")));
      meta.appendChild(el("div", "mp-sub", [m.author && ByopopStrings.t("mods.by", {author: m.author}), m.description].filter(Boolean).join(" · ") || m.dir));
      var tags = el("div", "mp-tags");
      var tag = function (text, cls) { tags.appendChild(el("span", "mp-tag" + (cls ? " " + cls : ""), text)); };
      if (queued) tag(ByopopStrings.t("mods.tag.removed"), "pending");
      else {
        tag(ByopopStrings.t(m.enabled ? "mods.tag.on" : "mods.tag.off"), m.enabled ? "on" : "");
        if (staged) tag(ByopopStrings.t("mods.tag.replaced"), "pending");
        else if (running && m.pending === "restart") tag(ByopopStrings.t("mods.tag.restart"), "pending");
        else if (running && m.pending === "nextGame") tag(ByopopStrings.t("mods.tag.next_game"), "pending");
        if (m.scores && m.enabled) tag(ByopopStrings.t("mods.tag.scores"), "scores");
        if (m.features) tag(ByopopStrings.t("mods.tag.features"), "features");
        if (m.hasSettings) tag(ByopopStrings.t("mods.tag.settings"));
      }
      meta.appendChild(tags);
      (m.errors || []).slice(0, 3).forEach(function (t) { meta.appendChild(el("div", "mp-msg error", t)); });
      (m.warnings || []).slice(0, 2).forEach(function (t) { meta.appendChild(el("div", "mp-msg warn", t)); });
      var actions = el("div", "mp-actions");
      var label = m.name + (m.version ? " " + m.version : "");
      if (queued) {
        var undo = button(ByopopStrings.t("mods.keep"), function () { self.keep(m); });
        undo.setAttribute("aria-label", ByopopStrings.t("mods.keep_label", {label: label}));
        undo.disabled = self.dead;
        actions.appendChild(undo);
      } else {
        var armed = self.armed === m.dir;
        var del = button(ByopopStrings.t(armed ? "mods.remove_confirm" : "mods.remove"), function () {
          if (self.armed === m.dir) { self.armed = null; self.remove(m); }
          else { self.armed = m.dir; self.render(); self.focusRemove(m.dir); }
        });
        del.setAttribute("aria-label", ByopopStrings.t(armed ? "mods.remove_confirm_label" : "mods.remove_label", {label: label}));
        del.setAttribute("data-dir", m.dir);
        del.disabled = self.dead;
        actions.appendChild(del);
      }
      row.appendChild(meta);
      row.appendChild(actions);
      self.list.appendChild(row);
    });

    // The rescue, before the game starts: once without mods
    this.rescue.hidden = !(!running && !this.dead && o.canPlay && o.canPlay() && this.enabledCount() > 0);

    // While the game runs: what waits for a restart, and the restart
    var box = this.restartBox, waiting = ((st.pending && st.pending.restart) || []).slice();
    st.mods.forEach(function (m) { if (exists(o.stagingDir + "/" + m.dir)) waiting.push(ByopopStrings.t("mods.replaced", {name: m.name})); });
    box.textContent = "";
    var nomods = running && st.nomods;
    box.hidden = !(running && (waiting.length || nomods || this.question));
    if (!box.hidden) {
      var row2 = el("div", "row");
      if (this.question) {
        box.appendChild(el("p", "", this.question));
        row2.appendChild(button(ByopopStrings.t("mods.restart_yes"), function () { self.restart(true); }, "primary"));
        row2.appendChild(button(ByopopStrings.t("mods.restart_no"), function () { self.question = null; self.render(); }));
      } else {
        box.appendChild(el("p", "note", nomods ? ByopopStrings.t("mods.off_run") :
          ByopopStrings.t("mods.restart_to_apply", {names: waiting.join(", ")})));
        row2.appendChild(button(ByopopStrings.t("mods.restart_game"), function () { self.restart(false); }));
      }
      box.appendChild(row2);
    }
    this.msg.textContent = this.message;
    if (hadFocus && !this.root.contains(document.activeElement)) this.root.focus();
  };
  ModPanel.prototype.focusRemove = function (dir) {
    var buttons = this.list.querySelectorAll("button[data-dir]");
    for (var i = 0; i < buttons.length; i++) if (buttons[i].getAttribute("data-dir") === dir) buttons[i].focus();
  };

  ModPanel.prototype.restart = function (confirmed) {
    var text = this.call("byopop_restart", "string", ["number"], [confirmed ? 1 : 0]);
    var r = {};
    try { r = JSON.parse(text || "{}"); } catch (e) {}
    this.question = null;
    if (r.result === "confirm") {
      this.question = r.message || ByopopStrings.t("mods.restart_now");
      this.render();
      var yes = this.restartBox.querySelector("button.primary");
      if (yes) yes.focus();
      return;
    }
    if (r.result === "blocked") this.say(r.message || ByopopStrings.t("mods.cannot_restart"));
    else if (r.result === "started") this.say(ByopopStrings.t("mods.restarting"));
    else this.say(ByopopStrings.t("mods.not_running"));
    this.render();
  };

  // Runs an import (a function returning a Promise of [{dir, inUse, update} or {note}]), then saves and shows it
  ModPanel.prototype.add = function (work) {
    var self = this;
    if (this.dead) return;
    this.addFolderButton.disabled = this.addZipButton.disabled = true;
    Promise.resolve().then(work).then(function (results) {
      results = [].concat(results || []).filter(Boolean);
      self.call("byopop_mods_rescan", null, [], []);
      self.refresh();
      var running = self.running();
      var names = function (list) {
        return list.map(function (r) {
          var m = self.mods().filter(function (x) { return x.dir === r.dir; })[0];
          return m ? m.name : r.dir;
        }).join(", ");
      };
      var pick = function (inUse, update) {
        return results.filter(function (r) { return (r.inUse === inUse) && (inUse || (r.update === update)); });
      };
      var added = pick(false, false), updated = pick(false, true), waiting = pick(true);
      var parts = [];
      if (added.length) parts.push(ByopopStrings.t("mods.added", {names: names(added)}));
      if (updated.length) parts.push(ByopopStrings.t("mods.updated", {names: names(updated)}));
      if (running && (added.length || updated.length)) parts.push(ByopopStrings.t("mods.restart_to_use", {n: added.length + updated.length}));
      if (waiting.length) parts.push(ByopopStrings.t("mods.waiting", {names: names(waiting)}));
      if (!parts.length) parts.push(ByopopStrings.t("mods.nothing_added"));
      results.forEach(function (r) { if (r.note) parts.push(r.note); });
      self.say(parts.join(" "));
      return sync();
    }).catch(function (e) {
      self.say(e && e.message ? e.message : String(e));
    }).then(function () {
      self.addFolderButton.disabled = self.addZipButton.disabled = self.dead;
    });
  };

  // The mods in an upload (a folder's files or a .zip's entries): every folder that holds a mod.json, one of the
  // game's folders (images, sounds...: a mod of files alone) or one of its gameFiles, the outer one where one holds
  // another, so a folder of mods adds each of them. With none anywhere, the one top folder is the mod, or everything
  // is. Each mod's folder is named after its root. Resolves to [{dir, inUse, update}], and a {note} naming what was left out:
  // files outside every mod's folder.
  ModPanel.prototype.importMods = function (entries, fallbackName) {
    var self = this, o = this.o;
    if (!entries.length) return Promise.resolve([]);
    var gameDirs = (o.gameDirs || []).map(function (d) { return d.toLowerCase(); });
    var gameFiles = o.gameFiles || [];
    var roots = [];
    entries.forEach(function (e) {
      var parts = e.path.split("/");
      for (var i = 0; i < parts.length; i++) {
        var isManifest = (i === parts.length - 1) && /^mod\.json$/i.test(parts[i]);
        var isGameDir = (i < parts.length - 1) && (gameDirs.indexOf(parts[i].toLowerCase()) >= 0);
        var isGameFile = (i === parts.length - 1) && (gameFiles.indexOf(parts[i].toLowerCase()) >= 0);
        if (isManifest || isGameDir || isGameFile) {
          var root = parts.slice(0, i).join("/") + (i ? "/" : "");
          if (roots.indexOf(root) < 0) roots.push(root);
          break;
        }
      }
    });
    roots.sort(function (a, b) { return a.length - b.length; });
    roots = roots.filter(function (r, i) {
      return !roots.slice(0, i).some(function (q) { return r.substring(0, q.length) === q; });
    }).sort();
    var found = roots.length > 0;
    if (!found) {
      var tops = {};
      entries.forEach(function (e) { tops[e.path.split("/")[0]] = 1; });
      var keys = Object.keys(tops);
      roots = [keys.length === 1 ? keys[0] + "/" : ""];
    }
    var dirs = {};
    var mods = roots.map(function (root) {
      var dir = (root.replace(/\/$/, "").split("/").pop() || fallbackName || "mod").replace(/[^A-Za-z0-9._-]+/g, "-");
      if (!safeName(dir)) dir = "mod";
      // two mods of one upload in folders of the same name (in different places) keep apart
      for (var n = 2, base = dir; dirs[dir]; n++) dir = base + "-" + n;
      dirs[dir] = 1;
      return { root: root, dir: dir };
    });
    var notes = [];
    if (found) {
      // named from the folder that holds the mods (their roots' common parent), as one would see it opened
      var base = roots.map(function (r) { return r.replace(/[^\/]*\/$/, ""); }).reduce(function (a, b) {
        var i = 0;
        while ((i < a.length) && (i < b.length) && (a[i] === b[i])) i++;
        return a.substring(0, a.lastIndexOf("/", i - 1) + 1);
      });
      var left = {};
      entries.forEach(function (e) {
        if (roots.some(function (r) { return e.path.substring(0, r.length) === r; })) return;
        var rel = (e.path.substring(0, base.length) === base) ? e.path.substring(base.length) : e.path;
        left[rel.split("/")[0]] = 1;
      });
      var names = Object.keys(left);
      if (names.length) notes.push({ note: ByopopStrings.t(gameFiles.length ? "mods.left_out_files" : "mods.left_out", {names: names.join(", ")}) });
    }
    return mods.reduce(function (p, m) {
      return p.then(function (done) {
        return self.importMod(entries, m.root, m.dir).then(function (r) { return done.concat(r ? [r] : []); });
      });
    }, Promise.resolve([])).then(function (done) { return done.concat(notes); });
  };

  // One mod of an upload: the entries under root, into the mods folder dir. Resolves to {dir, inUse, update} (null:
  // nothing to add): inUse, it waits for the next start; update, it replaced a mod of that folder.
  ModPanel.prototype.importMod = function (entries, root, dir) {
    var self = this, o = this.o;
    var files = [];
    entries.forEach(function (e) {
      if (e.path.substring(0, root.length) !== root) return;
      var rel = safeRel(e.path.substring(root.length));
      if (rel) files.push({ rel: rel, read: e.read });
    });
    if (!files.length) return Promise.resolve(null);
    // A folder the running game uses is not changed under it: the new version waits for the next start
    var used = this.mods().filter(function (m) { return m.dir === dir; })[0];
    var isUpdate = !!used || exists(o.modsDir + "/" + dir);
    var inUse = !!(this.running() && used && used.applied);
    var target = (inUse ? o.stagingDir : o.modsDir) + "/" + dir;
    rmTree(target);
    if (!inUse) rmTree(o.stagingDir + "/" + dir);
    this.say(ByopopStrings.t("mods.copying", {count: files.length, dir: dir}));
    return files.reduce(function (p, f) {
      return p.then(function () { return f.read().then(function (d) { writeFile(target + "/" + f.rel, d); }); });
    }, Promise.resolve()).then(function () {
      // Added again: a removal of this folder that waited for the next start is off
      var q = self.unqueue(dir);
      if (q && q.wasEnabled) self.edit({ op: "enable", mod: q.id, on: true });
      return { dir: dir, inUse: inUse, update: isUpdate };
    });
  };

  ModPanel.prototype.remove = function (m) {
    var o = this.o;
    rmTree(o.stagingDir + "/" + m.dir);
    if (this.running() && m.applied) {
      // The running game reads this folder: off from the next start on, and deleted then
      var q = this.queue().filter(function (e) { return e.dir !== m.dir; });
      q.push({ dir: m.dir, id: m.id, wasEnabled: !!m.enabled });
      this.setQueue(q);
      var error = m.enabled ? this.edit({ op: "enable", mod: m.id, on: false }) : "";
      if (error) {
        // Still on: it would not be removed (a queued mod that is on is kept)
        this.unqueue(m.dir);
        this.say(ByopopStrings.t("mods.remove_failed", {name: m.name, error: error}));
        this.refresh();
        return;
      }
      this.say(ByopopStrings.t("mods.removed_later", {name: m.name}));
      this.refresh();
      return;
    }
    rmTree(o.modsDir + "/" + m.dir);
    this.unqueue(m.dir);
    this.call("byopop_mods_rescan", null, [], []);
    // Its place in the order and its on/off state go too (its settings' values stay, should it come back)
    this.edit({ op: "forget", mod: m.id });
    this.say(ByopopStrings.t("mods.removed", {name: m.name}));
    this.refresh();
    sync();
  };
  // A removal waiting for the next start, taken back
  ModPanel.prototype.keep = function (m) {
    var q = this.unqueue(m.dir);
    if (q && q.wasEnabled) this.edit({ op: "enable", mod: q.id, on: true });
    this.say(ByopopStrings.t("mods.kept", {name: m.name}));
    this.refresh();
  };

  return {
    fs: { exists: exists, isDir: isDir, listDir: listDir, rmTree: rmTree, writeFile: writeFile, readText: readText, safeRel: safeRel },
    zip: { read: readZip, fromInput: filesFromInput, fromFile: filesFromZip },
    pak: { read: readPak },
    sync: sync,
    Session: Session,
    ModPanel: ModPanel
  };
})();
