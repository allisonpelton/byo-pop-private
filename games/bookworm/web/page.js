  // BookWorm's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md), and its Replays
  // panel (page.html)
  var PAGE = {
    id: "bookworm",
    shortName: "BookWorm",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so BookWorm, Big Money and
    // Bejeweled can be served from the same site without sharing files or saves
    persist: "/bwpersist",
    lock: "bookworm-deluxe-instance",
    gameDirs: ["images", "fonts", "sounds", "music", "properties"],
    // The word files and Bookworm.exe (the game's texts are read from it; it is never run) sit at the top of the
    // game folder
    gameFiles: ["wordlist.txt", "bonus_word_dictionary.txt", "popdefs.txt", "bookworm.exe"],
    markers: ["music/bookworm1.mo3", "wordlist.txt", "bookworm.exe"],
    markerText: "wordlist.txt, music/Bookworm1.mo3 and Bookworm.exe",
    // the original's runtime sound cache
    skip: [/^sounds\/cached_/i],
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/BookWorm Deluxe</span>) or a .zip of it. Only the <span class="mono">images, fonts, sounds, music</span> and ' +
      '<span class="mono">properties</span> folders, the word files (<span class="mono">wordlist.txt, ' +
      'bonus_word_dictionary.txt, popdefs.txt</span>) and <span class="mono">Bookworm.exe</span> (the game\'s texts are ' +
      'read from it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/backdrop_board.gif, or images/backdrop_board@2x.png for an HD version, or a wordlist.txt with another " +
      "dictionary), plus an optional mod.json.",
    seeds: false,
    // games/bookworm/src/Replay.h
    replays: true,
    start: function (run) {
      var replay = pageReplay();
      if (!replay) return;
      run.args.push("-replay=" + REPLAYS + "/" + replay);
      run.status += " · watching a replay";
    },
    shown: function (id) { if (id === "panel-replays") renderReplays(); },
    init: function () {
      $("pick-replay").addEventListener("change", function (ev) { importReplays(ev.target.files); ev.target.value = ""; });
    },
    // The game writes players/<name>/classic.dat or action.dat (classic-<key>.dat... with mods that change play)
    // only for a game in progress
    hasSave: function () {
      var fs = web().fs;
      return fs.listDir(USER + "/players").some(function (p) {
        return fs.listDir(USER + "/players/" + p).some(function (n) { return /^(classic|action).*\.dat$/i.test(n); });
      });
    }
  };

  // ?replay=<name>: watch that replay from REPLAYS (passed on as -replay=). A plain file name only.
  function pageReplay() {
    var name = new URLSearchParams(location.search).get("replay") || "";
    return (/^[A-Za-z0-9._-]+\.bwr$/.test(name) && web().fs.exists(REPLAYS + "/" + name)) ? name : "";
  }

  // ---------- replays (as Big Money's page) ----------
  var MODE_NAMES = { classic: "Classic", action: "Action" };
  function renderReplays() {
    var list = $("replay-list");
    list.textContent = "";
    var names = web().fs.listDir(REPLAYS).filter(function (n) { return /\.bwr$/i.test(n); }).sort().reverse();
    if (!names.length) {
      var empty = document.createElement("li");
      empty.className = "empty";
      empty.textContent = "No replays yet.";
      list.appendChild(empty);
      return;
    }
    names.forEach(function (n) {
      // <yyyymmdd>-<hhmmss>-<mode>-<score>[-<n>][-keep].bwr
      var m = /^(\d{4})(\d{2})(\d{2})-(\d{2})(\d{2})\d{2}-([a-z]+)-(\d+)/.exec(n);
      var row = document.createElement("li");
      row.className = "replay";
      var meta = document.createElement("div");
      meta.className = "meta";
      meta.innerHTML = '<div class="name"></div><div class="sub"></div>';
      meta.querySelector(".name").textContent = m ? (MODE_NAMES[m[6]] || m[6]) + ": " + Number(m[7]).toLocaleString() +
        " points" : n;
      meta.querySelector(".sub").textContent = m ? m[1] + "-" + m[2] + "-" + m[3] + " " + m[4] + ":" + m[5] +
        (/-keep\.bwr$/i.test(n) ? " · kept" : "") : "";
      var actions = document.createElement("div");
      actions.className = "actions";
      var watch = document.createElement("button");
      watch.type = "button"; watch.textContent = "Watch"; watch.setAttribute("aria-label", "Watch " + n);
      watch.addEventListener("click", function () {
        var params = new URLSearchParams(location.search);
        params.set("replay", n);
        location.search = params.toString();
      });
      var save = document.createElement("button");
      save.type = "button"; save.textContent = "Download"; save.setAttribute("aria-label", "Download " + n);
      save.addEventListener("click", function () {
        var url = URL.createObjectURL(new Blob([Module.FS.readFile(REPLAYS + "/" + n)], { type: "application/octet-stream" }));
        var a = document.createElement("a");
        a.href = url; a.download = n;
        document.body.appendChild(a); a.click(); a.remove();
        setTimeout(function () { URL.revokeObjectURL(url); }, 1000);
      });
      var del = document.createElement("button");
      del.type = "button"; del.textContent = "Delete"; del.setAttribute("aria-label", "Delete " + n);
      del.addEventListener("click", function () {
        if (del.dataset.armed) { try { Module.FS.unlink(REPLAYS + "/" + n); } catch (e) {} web().sync().then(renderReplays); }
        else { del.dataset.armed = "1"; del.textContent = "Confirm"; del.setAttribute("aria-label", "Confirm: delete " + n); }
      });
      actions.appendChild(watch); actions.appendChild(save); actions.appendChild(del);
      row.appendChild(meta); row.appendChild(actions);
      list.appendChild(row);
    });
  }
  function importReplays(files) {
    var bad = [];
    return Array.prototype.slice.call(files).reduce(function (p, f) {
      return p.then(function () { return f.arrayBuffer(); }).then(function (buf) {
        var b = new Uint8Array(buf);
        var name = f.name.replace(/[^A-Za-z0-9._-]/g, "_");
        if (b.length < 8 || String.fromCharCode(b[0], b[1], b[2], b[3]) !== "BWRP" || !/\.bwr$/i.test(name)) { bad.push(f.name); return; }
        web().fs.writeFile(REPLAYS + "/" + name, b);
      });
    }, Promise.resolve()).then(function () { return web().sync(); }).then(function () {
      $("replay-msg").textContent = bad.length ? "Not a BookWorm replay: " + bad.join(", ") : "";
      renderReplays();
    });
  }
