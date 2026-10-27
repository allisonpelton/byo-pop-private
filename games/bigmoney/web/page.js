  // Big Money's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md), and its Replays
  // panel (page.html)
  var PAGE = {
    id: "bigmoney",
    shortName: "Big Money",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Big Money and Bejeweled can be
    // served from the same site without sharing files or saves
    persist: "/bmpersist",
    lock: "big-money-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music", "properties"],
    markers: ["images/mrmoneybags.*"],
    markerText: "images/mrmoneybags file",
    importHtml: 'Choose the game\'s install folder (for Steam: <span class="mono">steamapps/common/Big Money Deluxe</span>) or a ' +
      '.zip of it. Only the <span class="mono">images, data, sounds, music</span> and <span class="mono">properties</span> ' +
      'folders are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/coins.gif, or images/coins@2x.png for an HD version), plus an optional mod.json.",
    // ?seed=N: the seed of every new game until one is played on it (its first valid click or its game over;
    // games/bigmoney/src/ModRules.h)
    // the game stores no exe: the import reads its icon out of the install's exe (web/shell.html, importIcon)
    importIcon: true,
    seeds: true,
    // games/bigmoney/src/Replay.h
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
    }
  };

  // ?replay=<name>: watch that replay from REPLAYS (passed on as -replay=). A plain file name only.
  function pageReplay() {
    var name = new URLSearchParams(location.search).get("replay") || "";
    return (/^[A-Za-z0-9._-]+\.bmr$/.test(name) && web().fs.exists(REPLAYS + "/" + name)) ? name : "";
  }

  // ---------- replays ----------
  var MODE_NAMES = { action: "Action", strategy: "Strategy", puzzle: "Puzzle" };
  function renderReplays() {
    var list = $("replay-list");
    list.textContent = "";
    var names = web().fs.listDir(REPLAYS).filter(function (n) { return /\.bmr$/i.test(n); }).sort().reverse();
    if (!names.length) {
      var empty = document.createElement("div");
      empty.className = "empty";
      empty.textContent = "No replays yet.";
      list.appendChild(empty);
      return;
    }
    names.forEach(function (n) {
      // <yyyymmdd>-<hhmmss>-<mode>-<skill>-<score>[-<n>][-keep].bmr
      var m = /^(\d{4})(\d{2})(\d{2})-(\d{2})(\d{2})\d{2}-([a-z]+)-([a-z]+)-(\d+)/.exec(n);
      var row = document.createElement("div");
      row.className = "replay";
      var meta = document.createElement("div");
      meta.className = "meta";
      meta.innerHTML = '<div class="name"></div><div class="sub"></div>';
      meta.querySelector(".name").textContent = m ? (MODE_NAMES[m[6]] || m[6]) + ", " + m[7] + ": $" + m[8] : n;
      meta.querySelector(".sub").textContent = m ? m[1] + "-" + m[2] + "-" + m[3] + " " + m[4] + ":" + m[5] +
        (/-keep\.bmr$/i.test(n) ? " · kept" : "") : "";
      var actions = document.createElement("div");
      actions.className = "actions";
      var watch = document.createElement("button");
      watch.type = "button"; watch.textContent = "Watch"; watch.setAttribute("aria-label", "Watch " + n);
      watch.addEventListener("click", function () {
        var params = new URLSearchParams(location.search);
        params.set("replay", n);
        params.delete("seed");
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
        else { del.dataset.armed = "1"; del.textContent = "Confirm"; }
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
        if (b.length < 8 || String.fromCharCode(b[0], b[1], b[2], b[3]) !== "BMRP" || !/\.bmr$/i.test(name)) { bad.push(f.name); return; }
        web().fs.writeFile(REPLAYS + "/" + name, b);
      });
    }, Promise.resolve()).then(function () { return web().sync(); }).then(function () {
      $("replay-msg").textContent = bad.length ? "Not a Big Money replay: " + bad.join(", ") : "";
      renderReplays();
    });
  }
