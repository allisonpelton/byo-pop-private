  // Typer Shark! Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "typershark",
    shortName: "Typer Shark Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so the games can be served from
    // the same site without sharing files or saves
    persist: "/tspersist",
    lock: "typer-shark-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music", "properties"],
    // WinTS.exe (the game's texts are read from it; it is never run) sits at the top of the game folder
    gameFiles: ["wints.exe"],
    markers: ["data/twords.txt", "images/titlescreen.jpg", "wints.exe"],
    markerText: "data/twords.txt, images/titlescreen.jpg and WinTS.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Typer Shark Deluxe</span>) or a .zip of it. Only the <span class="mono">images, data, sounds, music, ' +
      'properties</span> folders and <span class="mono">WinTS.exe</span> (the game\'s texts are read from ' +
      'it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music, word lists or level files in the same folders the game uses " +
      "(for example data/twords.txt), plus an optional mod.json.",
    seeds: false
  };
