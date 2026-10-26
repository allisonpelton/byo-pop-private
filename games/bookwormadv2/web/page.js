  // Bookworm Adventures Volume 2's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "bookwormadv2",
    shortName: "Bookworm Adventures",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so it can be served from the same
    // site as the other games without sharing files or saves (games/bookwormadv2/src/main.cpp uses the same folder)
    persist: "/bwa2persist",
    lock: "bookworm-adventures-deluxe-instance",
    // Most of the game is packed in main.pak, which the import unpacks into the game's folder (its folders are
    // these; the import keeps only what is in them); the cutscenes are loose in images/, and
    // BookwormAdventures.exe holds the game's texts (it is never run)
    gameDirs: ["data", "fonts", "images", "properties", "scripts", "sounds"],
    gameFiles: ["bookwormadventures.exe"],
    paks: ["main.pak"],
    markers: ["main.pak", "bookwormadventures.exe"],
    markerText: "main.pak and BookwormAdventures.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Bookworm Adventures Volume 2</span>) or a .zip of it. Only <span class="mono">main.pak</span>, the ' +
      '<span class="mono">images</span> and <span class="mono">properties</span> folders and ' +
      '<span class="mono">BookwormAdventures.exe</span> (the game\'s texts are read from it; it is never run) are ' +
      'copied, with main.pak unpacked.',
    modsIntro: "Mods replace game files: images, sounds, scripts or data in the same folders the game uses (as " +
      "main.pak holds them, for example images/board/grid_lowhealth.png), plus an optional mod.json.",
    seeds: false
  };
