  // Bookworm Adventures Volume 2's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "bookwormadv2",
    shortName: "Bookworm Adventures Volume 2",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so it can be served from the same
    // site as the other games without sharing files or saves (games/bookwormadv2/src/main.cpp uses the same folder)
    persist: "/bwa2persist",
    lock: "bookworm-adventures-vol2-instance",
    // Most of the game is packed in main.pak, which the import unpacks into the game's folder (its folders are
    // these; the import keeps only what is in them); the cutscenes (images/), the songs (music/), the scripts and
    // properties are loose, and BookwormAdventuresVol2.exe holds the game's texts (it is never run). The trailer
    // in images/upsell is the buy screen's, which the port does not have.
    gameDirs: ["data", "fonts", "images", "music", "properties", "scripts", "sounds"],
    gameFiles: ["bookwormadventuresvol2.exe"],
    paks: ["main.pak"],
    skip: [/^images\/upsell\//i],
    markers: ["main.pak", "bookwormadventuresvol2.exe"],
    markerText: "main.pak and BookwormAdventuresVol2.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Bookworm Adventures Volume 2</span>; on Steam: <span class="mono">steamapps/common/Bookworm Adventures ' +
      'Volume 2</span>) or a .zip of it. Only <span class="mono">main.pak</span>, the <span class="mono">images</span>, ' +
      '<span class="mono">music</span>, <span class="mono">properties</span> and <span class="mono">scripts</span> folders ' +
      'and <span class="mono">BookwormAdventuresVol2.exe</span> (the game\'s texts are read from it; it is never run) ' +
      'are copied, with main.pak unpacked.',
    modsIntro: "Mods replace game files: images, sounds, scripts or data in the same folders the game uses (as " +
      "main.pak holds them, for example images/cutscenes/intro.swf), plus an optional mod.json.",
    seeds: false
  };
