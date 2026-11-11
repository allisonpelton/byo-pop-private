  // Typer Shark! Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "typershark",
    shortName: "Typer Shark Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so the games can be served from
    // the same site without sharing files or saves
    persist: "/tspersist",
    lock: "typer-shark-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music", "properties"],
    // WinTS.exe (the Steam copy) or TyperShark.exe (the PopCap Game Collection's copy) sits at the top of the game folder: the
    // game's texts are read from it and it is never run. A marker cannot name two files, so neither exe is a marker.
    gameFiles: ["wints.exe", "typershark.exe"],
    markers: ["data/twords.txt", "images/titlescreen.jpg"],
    markerText: "data/twords.txt and images/titlescreen.jpg",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Typer Shark Deluxe</span>) or a .zip of it. Only the <span class="mono">images, data, sounds, music, ' +
      'properties</span> folders and <span class="mono">WinTS.exe</span> (<span class="mono">TyperShark.exe</span> in the ' +
      'Collection\'s copy; the game\'s texts are read from it, it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music, word lists or level files in the same folders the game uses " +
      "(for example data/twords.txt), plus an optional mod.json.",
    seeds: false
  };
